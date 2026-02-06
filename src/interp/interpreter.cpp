#include "suntv/interp/interpreter.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <queue>
#include <set>

#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"
#include "suntv/util/logging.hpp"

namespace sun {

static bool IsNonDataTypeString(const std::string& type) {
  // C2 uses many non-data kinds that appear as "type" in IGV dumps.
  // For our concrete interpreter, treat these as non-evaluable as Values.
  return type == "control" || type == "memory" || type == "abIO" ||
         type == "return_address" || type == "bottom";
}

static bool IsDataTypeString(const std::string& type) {
  // Heuristic for scalar data values used in tests/fixtures:
  // int:, long:, ptr:, etc. have trailing ':'; exclude known non-data kinds.
  if (type.empty()) return false;
  if (IsNonDataTypeString(type)) return false;
  return type.back() == ':';
}

static bool IsDataPhiNode(const Node* n) {
  if (!n || n->opcode() != Opcode::kPhi) return false;
  // Some IGV phases/dumps can reference nodes that are absent from the parsed
  // node list, leaving dangling input pointers. Be defensive here to avoid
  // crashing while classifying Phis.
  if (!n->has_prop("type")) {
    // Manually-constructed unit tests often omit type; treat as data.
    return true;
  }
  const std::string type = std::get<std::string>(n->prop("type"));
  if (type == "memory" || type == "control" || type == "abIO" ||
      type == "return_address") {
    return false;
  }
  return IsDataTypeString(type);
}

void Interpreter::UpdateRegionPhis(const Node* region, bool is_back_edge) {
  if (!region || region->opcode() != Opcode::kRegion) {
    return;
  }

  auto prune_cache_keep_seeds = [&]() {
    // After (re)seeding Phis, cached derived computations can become stale once
    // Phi values change. Keep only constants/parameters and Phis.
    auto it = value_cache_.begin();
    while (it != value_cache_.end()) {
      const Opcode vop = it->first->opcode();
      const bool keep =
          (vop == Opcode::kConI || vop == Opcode::kConL ||
           vop == Opcode::kConP || vop == Opcode::kParm || vop == Opcode::kPhi);
      if (keep) {
        ++it;
      } else {
        it = value_cache_.erase(it);
      }
    }
  };

  // Snapshot old Phi values (from previous iteration) so that back-edge updates
  // don't recursively depend on the new values being computed.
  if (is_back_edge) {
    phi_old_values_.clear();
    in_phi_update_ = true;
    updating_region_ = region;
    updating_phi_ = nullptr;
    for (Node* n : graph_.nodes()) {
      if (n->opcode() != Opcode::kPhi) continue;
      if (n->region_input() != region) continue;
      if (!IsDataPhiNode(n)) continue;
      auto it = value_cache_.find(n);
      if (it != value_cache_.end()) {
        phi_old_values_[n] = it->second;
      }
    }
  }

  // For back-edges, clear cached computed values so future evaluation uses the
  // updated Phi seeds. We keep constants/parameters.
  if (is_back_edge) {
    auto it = value_cache_.begin();
    while (it != value_cache_.end()) {
      Opcode vop = it->first->opcode();
      if (vop == Opcode::kConI || vop == Opcode::kConL ||
          vop == Opcode::kConP || vop == Opcode::kParm) {
        ++it;
      } else {
        it = value_cache_.erase(it);
      }
    }
  }

  // Collect data Phi nodes for this Region.
  std::vector<const Node*> phis;
  for (Node* n : graph_.nodes()) {
    if (n->opcode() != Opcode::kPhi) continue;
    if (n->region_input() != region) continue;
    if (!IsDataPhiNode(n)) continue;
    phis.push_back(n);
  }

  // Recompute Phi values without letting intermediate cached computations leak
  // out of the update.
  std::map<const Node*, Value> new_phi_values;
  Logger::Info("  UpdateRegionPhis: evaluating " + std::to_string(phis.size()) +
               " Phis");
  for (const Node* phi : phis) {
    updating_phi_ = phi;
    Logger::Info("    Evaluating Phi node " + std::to_string(phi->id()));
    new_phi_values[phi] = EvalPhi(phi);
    Logger::Info("    Phi node " + std::to_string(phi->id()) + " evaluated");
  }
  updating_phi_ = nullptr;
  Logger::Info("  UpdateRegionPhis: all Phis evaluated");

  // Install new Phi seeds.
  for (const auto& [phi, v] : new_phi_values) {
    value_cache_[phi] = v;
  }

  // Ensure no stale derived values remain cached.
  prune_cache_keep_seeds();

  if (is_back_edge) {
    in_phi_update_ = false;
    updating_region_ = nullptr;
    phi_old_values_.clear();
    phi_update_active_.clear();
  }
}

// Helper functions to categorize opcodes
static bool IsArithmetic(Opcode op) {
  return op == Opcode::kAddI || op == Opcode::kSubI || op == Opcode::kMulI ||
         op == Opcode::kDivI || op == Opcode::kModI || op == Opcode::kAbsI ||
         op == Opcode::kAddL || op == Opcode::kSubL || op == Opcode::kMulL ||
         op == Opcode::kDivL || op == Opcode::kModL || op == Opcode::kAbsL ||
         op == Opcode::kConvI2L || op == Opcode::kConvL2I;
}

static bool IsBitwise(Opcode op) {
  return op == Opcode::kAndI || op == Opcode::kOrI || op == Opcode::kXorI ||
         op == Opcode::kLShiftI || op == Opcode::kRShiftI ||
         op == Opcode::kURShiftI || op == Opcode::kAndL || op == Opcode::kOrL ||
         op == Opcode::kXorL || op == Opcode::kLShiftL ||
         op == Opcode::kRShiftL || op == Opcode::kURShiftL;
}

static bool IsComparison(Opcode op) {
  return op == Opcode::kCmpI || op == Opcode::kCmpL || op == Opcode::kCmpP ||
         op == Opcode::kCmpU || op == Opcode::kCmpUL;
}

Interpreter::Interpreter(const Graph& g) : graph_(g) {}

void Interpreter::BuildControlSuccessors() {
  control_successors_.clear();

  // Build adjacency from inputs: for each node n, if it is a control node,
  // record that it is a successor of its control input.
  for (Node* n : graph_.nodes()) {
    if (!n) continue;
    const Opcode op = n->opcode();

    const bool is_control_like =
        (op == Opcode::kIf || op == Opcode::kIfTrue || op == Opcode::kIfFalse ||
         op == Opcode::kGoto || op == Opcode::kReturn || op == Opcode::kHalt ||
         op == Opcode::kSafePoint || op == Opcode::kParsePredicate ||
         op == Opcode::kCallStaticJava || op == Opcode::kRegion ||
         op == Opcode::kProj || op == Opcode::kParm ||
         op == Opcode::kRangeCheck);
    if (!is_control_like) continue;

    // Region control predecessors can be at any index.
    if (op == Opcode::kRegion) {
      for (size_t i = 0; i < n->num_inputs(); ++i) {
        const Node* pred = n->input(i);
        if (!pred) continue;
        // C2 Regions often have a self-edge placeholder at input[0] for loops.
        // It is not a real control predecessor for our concrete traversal.
        if (pred == n) continue;
        control_successors_[pred].push_back(n);
      }
      continue;
    }

    // All other control-ish nodes use input[0] as control.
    if (n->num_inputs() == 0) continue;
    const Node* pred = n->input(0);
    if (!pred) continue;
    control_successors_[pred].push_back(n);
  }

  // Determinize iteration order.
  for (auto& [pred, succs] : control_successors_) {
    std::sort(succs.begin(), succs.end(),
              [](const Node* a, const Node* b) { return a->id() < b->id(); });
    succs.erase(std::unique(succs.begin(), succs.end()), succs.end());
  }
}

Outcome Interpreter::Execute(const std::vector<Value>& inputs) {
  return ExecuteWithHeap(inputs, ConcreteHeap());
}

Outcome Interpreter::ExecuteWithHeap(const std::vector<Value>& inputs,
                                     const ConcreteHeap& initial_heap) {
  Logger::Info("ExecuteWithHeap: starting");
  value_cache_.clear();
  region_predecessor_.clear();
  loop_iterations_.clear();
  heap_ = initial_heap;  // Use provided heap instead of resetting
  eval_active_.clear();
  phi_eval_stack_.clear();
  phi_update_active_.clear();
  phi_old_values_.clear();
  memory_chain_visited_.clear();
  in_phi_update_ = false;
  updating_region_ = nullptr;
  updating_phi_ = nullptr;

  Logger::Info("ExecuteWithHeap: calling BuildControlSuccessors");
  BuildControlSuccessors();
  Logger::Info("ExecuteWithHeap: BuildControlSuccessors done");

  // Cache parameter values first
  // Get all parameter nodes and filter to data parameters only
  Logger::Info("ExecuteWithHeap: getting parameter nodes");
  std::vector<Node*> all_params = graph_.GetParameterNodes();
  Logger::Info("ExecuteWithHeap: got " + std::to_string(all_params.size()) +
               " parameter nodes");
  std::vector<Node*> params;

  for (Node* p : all_params) {
    // Only include data parameters (skip control, memory, I/O, etc.)
    // Data parameters have types like "int:", "long:", etc.
    if (p->has_prop("type")) {
      std::string type = std::get<std::string>(p->prop("type"));
      // Skip non-data types
      if (type == "control" || type == "memory" || type == "return_address" ||
          type == "rawptr:" || type == "abIO") {
        continue;
      }
      // Only include types that end with ':' (int:, long:, etc.)
      if (type.empty() || type.back() != ':') {
        continue;
      }
    }
    params.push_back(p);
  }

  // Sort parameters by their index
  std::sort(params.begin(), params.end(), [](Node* a, Node* b) {
    // Try to get index from property first
    if (a->has_prop("index") && b->has_prop("index")) {
      return std::get<int32_t>(a->prop("index")) <
             std::get<int32_t>(b->prop("index"));
    }

    // Try to extract from dump_spec (e.g., "Parm0: int")
    auto get_index = [](Node* n) -> int32_t {
      if (n->has_prop("dump_spec")) {
        std::string spec = std::get<std::string>(n->prop("dump_spec"));
        size_t parm_pos = spec.find("Parm");
        if (parm_pos != std::string::npos) {
          size_t colon_pos = spec.find(':', parm_pos);
          if (colon_pos != std::string::npos) {
            std::string num_str =
                spec.substr(parm_pos + 4, colon_pos - parm_pos - 4);
            try {
              return std::stoi(num_str);
            } catch (...) {
              return 999999;  // Invalid index
            }
          }
        }
      }
      return 999999;  // No index found
    };

    return get_index(a) < get_index(b);
  });

  for (Node* parm_node : params) {
    Value parm_val = EvalParm(parm_node, inputs);
    value_cache_[parm_node] = parm_val;
  }
  Logger::Info("ExecuteWithHeap: parameter caching done");

  // Start control flow traversal from Start node
  Node* start = graph_.start();
  if (!start) {
    throw std::runtime_error("No Start node found in graph");
  }
  Logger::Info("ExecuteWithHeap: starting control flow traversal");

  // Traverse control flow until we reach Return
  const Node* current_control = start;
  int step_count = 0;
  constexpr int kMaxControlSteps = 100000;
  while (current_control && current_control->opcode() != Opcode::kReturn) {
    if (step_count++ > kMaxControlSteps) {
      throw std::runtime_error("Control flow exceeded maximum steps (" +
                               std::to_string(kMaxControlSteps) + ")");
    }
    if (step_count % 100 == 0) {
      Logger::Debug("Control flow step " + std::to_string(step_count) +
                    ": node " + std::to_string(current_control->id()));
    }
    current_control = StepControl(current_control);
    if (!current_control) {
      break;
    }
  }

  if (!current_control) {
    throw std::runtime_error("Control flow terminated without reaching Return");
  }

  // Evaluate the return value (if any)
  Outcome outcome;
  outcome.kind = Outcome::Kind::kReturn;

  // C2 Return node structure:
  // input[0] = control
  // input[1..n-1] = various (memory, frame pointer, etc.)
  // Last input = return value (if method returns non-void)
  // Find the return value: it's typically the last input that's not a Parm
  Node* value_node = nullptr;
  for (int i = current_control->num_inputs() - 1; i >= 1; --i) {
    Node* inp = current_control->input(i);
    if (inp && inp->opcode() != Opcode::kParm) {
      value_node = inp;
      break;
    }
  }

  if (value_node) {
    try {
      Value result = EvalNode(value_node);
      outcome.return_value = result;
    } catch (const EvalException& e) {
      // Convert evaluation exception to Throw outcome
      outcome.kind = Outcome::Kind::kThrow;
      outcome.exception_kind = e.what();
    }
  }

  outcome.heap = heap_;
  return outcome;
}

const Node* Interpreter::StepControl(const Node* ctrl) {
  if (!ctrl) {
    throw std::runtime_error("StepControl called with null control node");
  }

  Opcode op = ctrl->opcode();
  Logger::Info("StepControl: node " + std::to_string(ctrl->id()) + " (" +
               OpcodeToString(op) + ")");

  switch (op) {
    case Opcode::kStart:
    case Opcode::kGoto:
    case Opcode::kIfTrue:
    case Opcode::kIfFalse:
    case Opcode::kParm:       // Parm nodes act as control projections in C2
    case Opcode::kSafePoint:  // SafePoint is a control pass-through
    case Opcode::kProj:       // Proj can also be a control projection in C2
    case Opcode::kCallStaticJava:  // Often uncommon_trap; assume no deopt
      // Simple pass-through: find successor
      return FindControlSuccessor(ctrl);

    case Opcode::kIf:
    case Opcode::kParsePredicate: {
      // If and ParsePredicate: evaluate condition and choose branch
      // Use schema-aware accessor to get value inputs
      auto value_inputs = ctrl->value_inputs();
      if (value_inputs.empty()) {
        throw std::runtime_error(std::string(OpcodeToString(ctrl->opcode())) +
                                 " node needs condition value input");
      }

      // Evaluate condition (this may recursively evaluate data subgraph)
      Value cond = EvalNode(value_inputs[0]);

      bool branch_taken = false;
      if (cond.is_bool()) {
        branch_taken = cond.as_bool();
      } else if (cond.is_i32()) {
        // C2 sometimes uses int as bool (non-zero = true)
        branch_taken = (cond.as_i32() != 0);
      } else {
        throw std::runtime_error("If condition must be boolean or int");
      }

      Logger::Trace("  If condition evaluated to: " +
                    std::string(branch_taken ? "true" : "false"));

      // Find the corresponding IfTrue or IfFalse successor (use precomputed
      // adjacency to avoid scanning and to match our traversal).
      auto it_succs = control_successors_.find(ctrl);
      if (it_succs != control_successors_.end()) {
        for (const Node* s : it_succs->second) {
          if (!s) continue;
          if (branch_taken && s->opcode() == Opcode::kIfTrue) {
            return s;
          }
          if (!branch_taken && s->opcode() == Opcode::kIfFalse) {
            return s;
          }
        }
      }

      throw std::runtime_error("If node has no IfTrue/IfFalse successors");
    }

    case Opcode::kRangeCheck: {
      // RangeCheck: Array bounds check that branches on success/failure
      // Inputs: [0] control, [1] Bool node (comparison result)
      // Outputs: IfTrue (bounds OK), IfFalse (out of bounds)
      // Similar to If node, but specifically for array bounds checking

      Logger::Info("StepControl: handling RangeCheck node " +
                   std::to_string(ctrl->id()));

      auto value_inputs = ctrl->value_inputs();
      Logger::Info("  Got " + std::to_string(value_inputs.size()) +
                   " value inputs");
      if (value_inputs.empty()) {
        throw std::runtime_error("RangeCheck node needs Bool condition input");
      }

      // Evaluate the condition (Bool node that computes the bounds check)
      Logger::Info("  About to evaluate Bool condition node " +
                   std::to_string(value_inputs[0]->id()));
      Value cond = EvalNode(value_inputs[0]);
      Logger::Info("  Condition evaluated");

      bool bounds_ok = false;
      if (cond.is_bool()) {
        bounds_ok = cond.as_bool();
      } else if (cond.is_i32()) {
        // C2 sometimes uses int as bool (non-zero = true)
        bounds_ok = (cond.as_i32() != 0);
      } else {
        throw std::runtime_error("RangeCheck condition must be boolean or int");
      }

      Logger::Info("  Bounds check result: " +
                   std::string(bounds_ok ? "OK" : "FAIL"));
      Logger::Trace(
          "  RangeCheck condition evaluated to: " +
          std::string(bounds_ok ? "true (OK)" : "false (OUT_OF_BOUNDS)"));

      // Find IfTrue or IfFalse successor based on bounds check result
      Logger::Info("  Looking for successors");
      auto it_succs = control_successors_.find(ctrl);
      if (it_succs != control_successors_.end()) {
        Logger::Info("  Found " + std::to_string(it_succs->second.size()) +
                     " successors");
        for (const Node* s : it_succs->second) {
          if (!s) continue;
          Logger::Info("    Checking successor node " +
                       std::to_string(s->id()) + " (" +
                       OpcodeToString(s->opcode()) + ")");
          if (bounds_ok && s->opcode() == Opcode::kIfTrue) {
            Logger::Info("    Taking IfTrue branch");
            return s;
          }
          if (!bounds_ok && s->opcode() == Opcode::kIfFalse) {
            Logger::Info("    Taking IfFalse branch");
            return s;
          }
        }
      }

      throw std::runtime_error(
          "RangeCheck node has no IfTrue/IfFalse successors");
    }

    case Opcode::kRegion: {
      // Control merge point
      // We cannot reliably identify loop headers structurally from a raw C2 IGV
      // dump without CFG analysis. Instead, treat any Region that is revisited
      // during execution as part of a loop and update its data Phis on
      // subsequent visits.
      bool has_data_phi = false;
      for (Node* n : graph_.nodes()) {
        if (!IsDataPhiNode(n)) continue;
        if (n->region_input() == ctrl) {
          has_data_phi = true;
          break;
        }
      }

      if (has_data_phi) {
        auto it = loop_iterations_.find(ctrl);
        if (it == loop_iterations_.end()) {
          // First time we enter this Region: seed Phi caches for the entry
          // predecessor.
          Logger::Info("  Region first visit, seeding Phis");
          loop_iterations_[ctrl] = 0;
          UpdateRegionPhis(ctrl, /*is_back_edge=*/false);
          Logger::Info("  Region Phi seeding complete");
        } else {
          const int iter_count = it->second;
          Logger::Info("  Region revisit, iteration " +
                       std::to_string(iter_count));
          if (iter_count >= kMaxLoopIterations) {
            throw std::runtime_error("Loop exceeded maximum iterations (" +
                                     std::to_string(kMaxLoopIterations) + ")");
          }
          it->second = iter_count + 1;
          Logger::Info("  Updating Region Phis for back-edge");
          UpdateRegionPhis(ctrl, /*is_back_edge=*/true);
          Logger::Info("  Region Phi update complete");
        }
      }

      // Continue to successor
      return FindControlSuccessor(ctrl);
    }

    case Opcode::kHalt: {
      // Halt nodes appear in IGV dumps for uncommon traps and deopt paths.
      // In this concrete interpreter prototype, reaching Halt means we've
      // stepped onto an invalid/unsupported control path.
      throw std::runtime_error(
          "Reached Halt control node (likely uncommon trap): node " +
          std::to_string(ctrl->id()));
    }

    default:
      throw std::runtime_error("Unexpected control opcode in StepControl: " +
                               OpcodeToString(op));
  }
}

static bool PropIsTrue(const Node* n, const std::string& key) {
  if (!n || !n->has_prop(key)) return false;
  const Property p = n->prop(key);
  if (std::holds_alternative<bool>(p)) return std::get<bool>(p);
  if (std::holds_alternative<int32_t>(p)) return std::get<int32_t>(p) != 0;
  if (std::holds_alternative<int64_t>(p)) return std::get<int64_t>(p) != 0;
  if (std::holds_alternative<std::string>(p)) {
    const std::string& s = std::get<std::string>(p);
    return s == "true" || s == "True" || s == "1";
  }
  return false;
}

static std::optional<int64_t> PropAsI64(const Node* n, const std::string& key) {
  if (!n || !n->has_prop(key)) return std::nullopt;
  const Property p = n->prop(key);
  if (std::holds_alternative<int32_t>(p)) return std::get<int32_t>(p);
  if (std::holds_alternative<int64_t>(p)) return std::get<int64_t>(p);
  if (std::holds_alternative<std::string>(p)) {
    const std::string& s = std::get<std::string>(p);
    char* end = nullptr;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if (end != s.c_str() && *end == '\0') return static_cast<int64_t>(v);
  }
  return std::nullopt;
}

const Node* Interpreter::FindControlSuccessor(const Node* ctrl) {
  if (!ctrl) return nullptr;

  auto it = control_successors_.find(ctrl);
  if (it == control_successors_.end()) {
    Logger::Warn("FindControlSuccessor: node " + std::to_string(ctrl->id()) +
                 " (" + OpcodeToString(ctrl->opcode()) + ") has no successors");
    return nullptr;
  }

  const auto& succs = it->second;

  auto is_candidate = [](const Node* s) -> bool {
    if (!s) return false;
    const Opcode op = s->opcode();
    if (op == Opcode::kRegion || op == Opcode::kIf || op == Opcode::kIfTrue ||
        op == Opcode::kIfFalse || op == Opcode::kGoto ||
        op == Opcode::kReturn || op == Opcode::kHalt ||
        op == Opcode::kSafePoint || op == Opcode::kParsePredicate ||
        op == Opcode::kCallStaticJava || op == Opcode::kProj ||
        op == Opcode::kRangeCheck) {
      return true;
    }
    if (op == Opcode::kParm && s->has_prop("type")) {
      const Property p = s->prop("type");
      if (std::holds_alternative<std::string>(p)) {
        return std::get<std::string>(p) == "control";
      }
    }
    return false;
  };

  std::vector<const Node*> candidates;
  candidates.reserve(succs.size());
  for (const Node* s : succs) {
    if (is_candidate(s)) candidates.push_back(s);
  }
  if (candidates.empty()) {
    Logger::Warn("FindControlSuccessor: node " + std::to_string(ctrl->id()) +
                 " has " + std::to_string(succs.size()) +
                 " successors but none are control candidates");
    for (const Node* s : succs) {
      Logger::Warn("  - successor node " + std::to_string(s->id()) + " (" +
                   OpcodeToString(s->opcode()) + ")");
    }
    return nullptr;
  }
  if (candidates.size() == 1) {
    const Node* chosen = candidates.front();
    if (chosen->opcode() == Opcode::kRegion) {
      region_predecessor_[chosen] = ctrl;
    }
    return chosen;
  }

  const auto ctrl_idx = PropAsI64(ctrl, "idx");
  const auto ctrl_bci = PropAsI64(ctrl, "bci");

  auto priority = [](Opcode op) -> int {
    switch (op) {
      case Opcode::kReturn:
        return 0;
      case Opcode::kHalt:
        return 1000;
      case Opcode::kIf:
      case Opcode::kParsePredicate:
      case Opcode::kRangeCheck:
        return 2;
      case Opcode::kIfTrue:
      case Opcode::kIfFalse:
        return 3;
      case Opcode::kGoto:
        return 4;
      case Opcode::kRegion:
        return 5;
      case Opcode::kSafePoint:
      case Opcode::kCallStaticJava:
      case Opcode::kProj:
        return 6;
      case Opcode::kParm:
        return 7;
      default:
        return 100;
    }
  };

  auto score = [&](const Node* s) {
    const int prio = priority(s->opcode());
    const bool is_block_start = PropIsTrue(s, "is_block_start");
    const bool is_block_proj = PropIsTrue(s, "is_block_proj");
    const auto s_idx = PropAsI64(s, "idx");
    const auto s_bci = PropAsI64(s, "bci");

    int64_t idx_delta = 0;
    if (ctrl_idx && s_idx) {
      // Prefer forward progress in schedule order.
      idx_delta = (*s_idx >= *ctrl_idx)
                      ? (*s_idx - *ctrl_idx)
                      : (INT64_C(1) << 60) + (*ctrl_idx - *s_idx);
    } else {
      idx_delta = (INT64_C(1) << 60);
    }

    int64_t bci_delta = 0;
    if (ctrl_bci && s_bci) {
      bci_delta = (*s_bci >= *ctrl_bci)
                      ? (*s_bci - *ctrl_bci)
                      : (INT64_C(1) << 50) + (*ctrl_bci - *s_bci);
    } else {
      bci_delta = (INT64_C(1) << 50);
    }

    // Tuple ordering: smaller is better.
    return std::tuple<int, int, int, int64_t, int64_t, int32_t>(
        prio, is_block_start ? 0 : 1, is_block_proj ? 0 : 1, bci_delta,
        idx_delta, s->id());
  };

  const Node* chosen = *std::min_element(
      candidates.begin(), candidates.end(),
      [&](const Node* a, const Node* b) { return score(a) < score(b); });

  if (chosen && chosen->opcode() == Opcode::kRegion) {
    // CRITICAL: record predecessor only for the chosen Region successor.
    region_predecessor_[chosen] = ctrl;
  }
  return chosen;
}

bool Interpreter::IsLoopHeader(const Node* region) const {
  if (!region || region->opcode() != Opcode::kRegion) {
    return false;
  }

  // HotSpot C2 uses cyclic Phis for loop headers (induction variables).
  // This is a robust discriminator for our prototype: non-loop merge Regions
  // can still have a Region self-input placeholder, but they won't contain
  // a Phi that references itself.
  for (Node* n : graph_.nodes()) {
    if (!IsDataPhiNode(n)) continue;
    if (n->region_input() != region) continue;
    for (Node* v : n->phi_values()) {
      if (v == n) return true;
    }
  }
  return false;
}

const Node* Interpreter::SelectPhiInputNode(const Node* phi,
                                            const Node* active_pred,
                                            bool allow_self) const {
  if (!phi || phi->opcode() != Opcode::kPhi) return nullptr;
  const Node* region = phi->region_input();
  if (!region || region->opcode() != Opcode::kRegion) return nullptr;
  if (!active_pred) return nullptr;

  // Find the Region predecessor index i such that Region input[i] ==
  // active_pred.
  size_t pred_index = static_cast<size_t>(-1);
  for (size_t i = 0; i < region->num_inputs(); ++i) {
    const Node* rin = region->input(i);
    if (rin == nullptr) continue;
    if (rin == region) continue;
    if (rin == active_pred) {
      pred_index = i;
      break;
    }
  }
  if (pred_index == static_cast<size_t>(-1)) return nullptr;

  // Candidate phi input indices. Real C2 IGV dumps are inconsistent:
  // - often Phi input[0] is Region, and value for pred i is at input[i+1]
  // - sometimes Phi has the same arity as Region and uses input[i] for i>=1
  // - dumps can have holes (nullptr) at some indices
  std::vector<size_t> candidates;
  const size_t phi_n = phi->num_inputs();
  const size_t region_n = region->num_inputs();
  if (phi_n == region_n + 1) {
    candidates.push_back(pred_index + 1);
  }
  if (phi_n == region_n) {
    candidates.push_back((pred_index == 0) ? 1 : pred_index);
  }
  // Always try these two fallbacks as well (in case arities don't match).
  candidates.push_back(pred_index + 1);
  candidates.push_back(pred_index);

  auto accept = [&](const Node* v) -> bool {
    if (!v) return false;
    if (!allow_self && v == phi) return false;
    return true;
  };

  for (size_t idx : candidates) {
    if (idx >= phi->num_inputs()) continue;
    const Node* v = phi->input(idx);
    if (accept(v)) return v;
  }

  // As a last resort, use compacted predecessor counting to align the k-th
  // non-self Region input with the k-th Phi value (input[1+k]).
  size_t k = 0;
  for (size_t i = 0; i < region->num_inputs(); ++i) {
    const Node* rin = region->input(i);
    if (rin == nullptr || rin == region) continue;
    if (rin == active_pred) {
      const size_t idx = 1 + k;
      if (idx < phi->num_inputs() && accept(phi->input(idx))) {
        return phi->input(idx);
      }
      break;
    }
    ++k;
  }
  return nullptr;
}

Value Interpreter::EvalNode(const Node* n) {
  if (!n) {
    throw std::runtime_error("EvalNode called with null node");
  }

  static int eval_count = 0;
  if (++eval_count % 1000 == 0) {
    Logger::Debug("EvalNode call #" + std::to_string(eval_count) + ": node " +
                  std::to_string(n->id()) + " (" + OpcodeToString(n->opcode()) +
                  ")");
  }

  // During loop back-edge Phi updates, force all loop Phi reads to use the
  // previous-iteration value, regardless of what may already be in the cache.
  // This gives simultaneous-update semantics for mutually dependent Phis.
  if (in_phi_update_ && updating_region_ != nullptr &&
      n->opcode() == Opcode::kPhi && n->region_input() == updating_region_) {
    auto it_old = phi_old_values_.find(n);
    if (it_old != phi_old_values_.end()) {
      if (n != updating_phi_) {
        return it_old->second;
      }
      if (phi_update_active_.count(n) > 0) {
        return it_old->second;
      }
    }
  }

  // General cycle detection: loops in SoN value graphs should only be via Phi
  // with well-defined predecessor selection. If we see a cycle here, our
  // traversal/predecessor tracking is broken; fail fast instead of segfaulting.
  if (eval_active_.count(n) > 0) {
    throw std::runtime_error(
        "Cyclic value evaluation detected (node=" + std::to_string(n->id()) +
        ", op=" + OpcodeToString(n->opcode()) + ")");
  }
  struct EvalActiveGuard {
    Interpreter* interp;
    const Node* node;
    ~EvalActiveGuard() { interp->eval_active_.erase(node); }
  } active_guard{this, n};
  eval_active_.insert(n);

  struct EvalDepthGuard {
    Interpreter* interp;
    explicit EvalDepthGuard(Interpreter* i) : interp(i) {
      ++interp->eval_depth_;
      if (interp->eval_depth_ > Interpreter::kMaxEvalDepth) {
        throw std::runtime_error("Value evaluation exceeded max depth (" +
                                 std::to_string(Interpreter::kMaxEvalDepth) +
                                 ")");
      }
    }
    ~EvalDepthGuard() { --interp->eval_depth_; }
  } depth_guard(this);

  // CRITICAL: Control nodes should never be evaluated as data
  NodeSchema s = n->schema();
  if (s == NodeSchema::kS1_Control || s == NodeSchema::kS7_Start) {
    throw std::runtime_error("Attempted to evaluate control node as data: " +
                             OpcodeToString(n->opcode()) + " (node " +
                             std::to_string(n->id()) + ")");
  }

  // Check cache first
  auto it = value_cache_.find(n);
  if (it != value_cache_.end()) {
    return it->second;
  }

  Value result;
  Opcode op = n->opcode();

  // Dispatch based on opcode
  if (op == Opcode::kConI || op == Opcode::kConL || op == Opcode::kConP) {
    result = EvalConst(n);
  } else if (op == Opcode::kParm) {
    // Parm nodes should have been cached during Execute()
    // If we reach here, it means we're evaluating a Parm that wasn't cached
    // (e.g., a non-data Parm like control or memory)
    // Return a dummy value
    return Value::MakeI32(0);
  } else if (op == Opcode::kBool) {
    result = EvalBool(n);
  } else if (op == Opcode::kConv2B) {
    result = EvalConv2B(n);
  } else if (op == Opcode::kCastII || op == Opcode::kCastLL ||
             op == Opcode::kCastPP || op == Opcode::kCastX2P ||
             op == Opcode::kCastP2X) {
    // Cast operations: pass through the value
    // These are type system assertions that don't change the actual value
    const Node* input_node = n->input(1);
    if (!input_node) {
      throw std::runtime_error("Cast operation missing input");
    }
    result = EvalNode(input_node);
  } else if (op == Opcode::kCMoveI || op == Opcode::kCMoveL ||
             op == Opcode::kCMoveP) {
    result = EvalCMove(n);
  } else if (op == Opcode::kPhi) {
    // Memory/control Phis exist in real C2 graphs and can be cyclic (self).
    // We do not model memory as a first-class Value in this interpreter.
    if (!IsDataPhiNode(n)) {
      result = Value::MakeI32(0);
    } else {
      result = EvalPhi(n);
    }
  } else if (op == Opcode::kSafePoint || op == Opcode::kOpaque1 ||
             op == Opcode::kParsePredicate) {
    result = EvalNoOp(n);
  } else if (op == Opcode::kProj) {
    // Proj projects one output from a multi-output node. For this concrete
    // interpreter prototype, treat it as a pass-through of its first value
    // input (similar to Opaque1/SafePoint behavior).
    result = EvalNoOp(n);
  } else if (op == Opcode::kThreadLocal) {
    result = EvalThreadLocal(n);
  } else if (op == Opcode::kCallStaticJava) {
    result = EvalCallStaticJava(n);
  } else if (op == Opcode::kHalt) {
    result = EvalHalt(n);
  } else if (op == Opcode::kAllocate) {
    result = EvalAllocate(n);
  } else if (op == Opcode::kAllocateArray) {
    result = EvalAllocateArray(n);
  } else if (op == Opcode::kLoadRange) {
    // LoadRange: Get array length
    // Typical inputs: input[0] = control/memory, input[1] = memory, input[2] =
    // array reference Find the array reference input (should be a data input,
    // not memory/control)
    const Node* arr_node = nullptr;
    for (size_t i = 1; i < static_cast<size_t>(n->num_inputs()); ++i) {
      const Node* inp = n->input(i);
      if (inp && inp->opcode() != Opcode::kParm) {
        // Check if this could be the array reference
        // Try to evaluate it - if it's a reference, it's likely the array
        try {
          Value val = EvalNode(inp);
          if (val.is_ref()) {
            arr_node = inp;
            break;
          }
        } catch (...) {
          // Skip this input if evaluation fails
          continue;
        }
      }
    }

    if (!arr_node) {
      // If no reference found, try input[2] (typical C2 pattern)
      if (n->num_inputs() > 2) {
        arr_node = n->input(2);
      }
    }

    if (!arr_node) {
      throw std::runtime_error("LoadRange: could not find array input");
    }

    Value arr_val = EvalNode(arr_node);
    if (!arr_val.is_ref()) {
      throw std::runtime_error("LoadRange: array input is not a reference");
    }
    int32_t length = heap_.ArrayLength(arr_val.as_ref());
    result = Value::MakeI32(length);
  } else if (op == Opcode::kRangeCheck) {
    // RangeCheck: Verify index is within [0, length)
    // Inputs: length, index (or vice versa, need to check C2 convention)
    // For now, treat as a pass-through of the index (assuming bounds are valid)
    // In a real implementation, this would throw if out of bounds
    const Node* length_node = n->input(1);
    const Node* index_node = n->input(2);
    if (!length_node || !index_node) {
      throw std::runtime_error("RangeCheck: missing length or index input");
    }
    Value length_val = EvalNode(length_node);
    Value index_val = EvalNode(index_node);

    int32_t length = length_val.as_i32();
    int32_t index = index_val.as_i32();

    // Perform the bounds check
    if (index < 0 || index >= length) {
      throw std::runtime_error(
          "Array index out of bounds: index=" + std::to_string(index) +
          ", length=" + std::to_string(length));
    }

    // Return the index (pass-through)
    result = index_val;
  } else if (op == Opcode::kAddP) {
    // AddP: Pointer/address arithmetic (used for array element addressing)
    // For the concrete interpreter, we don't actually compute addresses
    // Instead, we treat this as a pass-through or extract the offset
    // Inputs are typically: base, offset
    // We can just pass through one of the inputs (typically the offset for
    // array indexing)

    // For array access patterns, AddP is used to compute element addresses
    // We don't need actual address calculation in our abstract interpreter
    // Just pass through the first non-base input (usually the index)
    const Node* input_node = nullptr;
    for (size_t i = 1; i < static_cast<size_t>(n->num_inputs()); ++i) {
      const Node* inp = n->input(i);
      if (inp && inp->opcode() != Opcode::kParm) {
        input_node = inp;
        break;
      }
    }

    if (!input_node && n->num_inputs() > 1) {
      input_node = n->input(1);
    }

    if (input_node) {
      result = EvalNode(input_node);
    } else {
      // If no suitable input, return a dummy value
      result = Value::MakeI32(0);
    }
  } else if (op == Opcode::kLoadB || op == Opcode::kLoadUB ||
             op == Opcode::kLoadS || op == Opcode::kLoadUS ||
             op == Opcode::kLoadI || op == Opcode::kLoadL ||
             op == Opcode::kLoadP || op == Opcode::kLoadN) {
    result = EvalLoad(n);
  } else if (IsArithmetic(op) || IsBitwise(op)) {
    result = EvalArithOp(n);
  } else if (IsComparison(op)) {
    result = EvalCmpOp(n);
  } else {
    throw std::runtime_error("Unsupported opcode: " +
                             OpcodeToString(n->opcode()));
  }

  // Cache the result
  value_cache_[n] = result;
  return result;
}

Value Interpreter::EvalConst(const Node* n) {
  Opcode op = n->opcode();

  if (op == Opcode::kConI) {
    // Try 'value' property first (manually constructed graphs)
    if (n->has_prop("value")) {
      int32_t val = std::get<int32_t>(n->prop("value"));
      return Value::MakeI32(val);
    }
    // C2 graphs encode value in dump_spec: " #int:42" or " #int:-5"
    if (n->has_prop("dump_spec")) {
      std::string spec = std::get<std::string>(n->prop("dump_spec"));
      // Format: " #int:<value>"
      size_t colon = spec.find(':');
      if (colon != std::string::npos) {
        std::string val_str = spec.substr(colon + 1);
        int32_t val = std::stoi(val_str);
        return Value::MakeI32(val);
      }
    }
    throw std::runtime_error(
        "ConI node missing 'value' or parseable 'dump_spec' property");
  } else if (op == Opcode::kConL) {
    // Try 'value' property first (manually constructed graphs)
    if (n->has_prop("value")) {
      int64_t val = std::get<int64_t>(n->prop("value"));
      return Value::MakeI64(val);
    }
    // C2 graphs encode value in dump_spec: " #long:42" or " #long:-5"
    if (n->has_prop("dump_spec")) {
      std::string spec = std::get<std::string>(n->prop("dump_spec"));
      // Format: " #long:<value>"
      size_t colon = spec.find(':');
      if (colon != std::string::npos) {
        std::string val_str = spec.substr(colon + 1);
        int64_t val = std::stoll(val_str);
        return Value::MakeI64(val);
      }
    }
    throw std::runtime_error(
        "ConL node missing 'value' or parseable 'dump_spec' property");
  } else if (op == Opcode::kConP) {
    // Null pointer constant
    return Value::MakeNull();
  }

  throw std::runtime_error("Unknown constant opcode");
}

Value Interpreter::EvalParm(const Node* n, const std::vector<Value>& inputs) {
  // Try to get index from property (for manually constructed graphs)
  if (n->has_prop("index")) {
    int32_t index = std::get<int32_t>(n->prop("index"));
    if (index < 0 || index >= static_cast<int32_t>(inputs.size())) {
      throw std::runtime_error("Parm index out of range");
    }
    return inputs[index];
  }

  // For C2 graphs: Extract index from dump_spec (e.g., "Parm0: int")
  if (n->has_prop("dump_spec")) {
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    // Trim leading/trailing whitespace
    size_t start = spec.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
      spec = spec.substr(start);
    }

    // Look for "Parm<N>:" pattern
    size_t parm_pos = spec.find("Parm");
    if (parm_pos != std::string::npos) {
      size_t colon_pos = spec.find(':', parm_pos);
      if (colon_pos != std::string::npos) {
        std::string num_str =
            spec.substr(parm_pos + 4, colon_pos - parm_pos - 4);
        // Trim the extracted number string as well
        size_t num_start = num_str.find_first_not_of(" \t\n\r");
        size_t num_end = num_str.find_last_not_of(" \t\n\r");
        if (num_start != std::string::npos && num_end != std::string::npos) {
          num_str = num_str.substr(num_start, num_end - num_start + 1);
        }

        try {
          int32_t index = std::stoi(num_str);
          if (index < 0 || index >= static_cast<int32_t>(inputs.size())) {
            // For array/object parameters that weren't provided,
            // return a null reference - this allows testing compilation
            // even without proper input setup
            Logger::Warn(
                "Parm index " + std::to_string(index) +
                " out of range (inputs size: " + std::to_string(inputs.size()) +
                "), returning null reference");
            return Value::MakeNull();
          }
          return inputs[index];
        } catch (const std::exception& e) {
          throw std::runtime_error(
              "Failed to parse Parm index from dump_spec: " + spec +
              " (extracted: '" + num_str + "')");
        }
      }
    }
  }

  // Fallback: non-data Parm nodes (control, memory, etc.) return dummy value
  // Check if this is a data parameter by looking at the 'type' property
  if (n->has_prop("type")) {
    std::string type = std::get<std::string>(n->prop("type"));
    if (type == "control" || type == "memory" || type == "return_address") {
      // Not a data parameter, return dummy
      return Value::MakeI32(0);
    }
  }

  throw std::runtime_error("Parm node missing 'index' or 'dump_spec' property");
}

Value Interpreter::EvalArithOp(const Node* n) {
  Opcode op = n->opcode();

  auto widen_i32_to_i64 = [](Value v) -> Value {
    if (v.is_i32()) return Value::MakeI64(v.as_i32());
    return v;
  };

  // Handle unary operations (AbsI, AbsL, conversions)
  if (op == Opcode::kAbsI || op == Opcode::kAbsL || op == Opcode::kConvI2L ||
      op == Opcode::kConvL2I) {
    // C2 format check: if input[0] is null, try input[1]
    const Node* operand = nullptr;
    if (n->num_inputs() >= 2 && n->input(0) == nullptr) {
      operand = n->input(1);
    } else if (n->num_inputs() >= 1) {
      operand = n->input(0);
    } else {
      throw std::runtime_error("Unary op needs at least 1 input");
    }

    Value a = EvalNode(operand);

    switch (op) {
      case Opcode::kAbsI:
        return Evaluator::EvalAbsI(a);
      case Opcode::kAbsL:
        return Evaluator::EvalAbsL(a);
      case Opcode::kConvI2L:
        return Evaluator::EvalConvI2L(a);
      case Opcode::kConvL2I:
        return Evaluator::EvalConvL2I(a);
      default:
        throw std::runtime_error("Unsupported unary opcode");
    }
  }

  // Binary operations - use schema-aware accessor
  auto value_inputs = n->value_inputs();
  if (value_inputs.size() < 2) {
    throw std::runtime_error("Binary op needs at least 2 value inputs");
  }

  Value a = EvalNode(value_inputs[0]);
  Value b = EvalNode(value_inputs[1]);

  // C2 sometimes keeps constants as int even when the operation is long-typed.
  // Be permissive and widen i32 to i64 for long ops.
  const bool is_long_op =
      (op == Opcode::kAddL || op == Opcode::kSubL || op == Opcode::kMulL ||
       op == Opcode::kDivL || op == Opcode::kModL || op == Opcode::kAndL ||
       op == Opcode::kOrL || op == Opcode::kXorL || op == Opcode::kLShiftL ||
       op == Opcode::kRShiftL || op == Opcode::kURShiftL);
  if (is_long_op) {
    a = widen_i32_to_i64(a);
    b = widen_i32_to_i64(b);
  }

  switch (op) {
    // Int32 arithmetic
    case Opcode::kAddI:
      return Evaluator::EvalAddI(a, b);
    case Opcode::kSubI:
      return Evaluator::EvalSubI(a, b);
    case Opcode::kMulI:
      return Evaluator::EvalMulI(a, b);
    case Opcode::kDivI:
      return Evaluator::EvalDivI(a, b);
    case Opcode::kModI:
      return Evaluator::EvalModI(a, b);

    // Int64 arithmetic
    case Opcode::kAddL:
      return Evaluator::EvalAddL(a, b);
    case Opcode::kSubL:
      return Evaluator::EvalSubL(a, b);
    case Opcode::kMulL:
      return Evaluator::EvalMulL(a, b);
    case Opcode::kDivL:
      return Evaluator::EvalDivL(a, b);
    case Opcode::kModL:
      return Evaluator::EvalModL(a, b);

    // Int32 bitwise
    case Opcode::kAndI:
      return Evaluator::EvalAndI(a, b);
    case Opcode::kOrI:
      return Evaluator::EvalOrI(a, b);
    case Opcode::kXorI:
      return Evaluator::EvalXorI(a, b);
    case Opcode::kLShiftI:
      return Evaluator::EvalLShiftI(a, b);
    case Opcode::kRShiftI:
      return Evaluator::EvalRShiftI(a, b);
    case Opcode::kURShiftI:
      return Evaluator::EvalURShiftI(a, b);

    // Int64 bitwise
    case Opcode::kAndL:
      return Evaluator::EvalAndL(a, b);
    case Opcode::kOrL:
      return Evaluator::EvalOrL(a, b);
    case Opcode::kXorL:
      return Evaluator::EvalXorL(a, b);
    case Opcode::kLShiftL:
      return Evaluator::EvalLShiftL(a, b);
    case Opcode::kRShiftL:
      return Evaluator::EvalRShiftL(a, b);
    case Opcode::kURShiftL:
      return Evaluator::EvalURShiftL(a, b);

    default:
      throw std::runtime_error("Unsupported arithmetic opcode");
  }
}

Value Interpreter::EvalCmpOp(const Node* n) {
  // Comparison operations: CmpI, CmpL, CmpP
  // Use schema-aware accessor to get value inputs
  auto value_inputs = n->value_inputs();

  if (value_inputs.size() < 2) {
    throw std::runtime_error("Comparison op needs at least 2 value inputs");
  }

  Value a = EvalNode(value_inputs[0]);
  Value b = EvalNode(value_inputs[1]);

  Opcode op = n->opcode();

  // CmpI/CmpL/CmpP return tri-state: -1 (less), 0 (equal), 1 (greater)
  switch (op) {
    case Opcode::kCmpI: {
      int32_t av = a.as_i32();
      int32_t bv = b.as_i32();
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    case Opcode::kCmpL: {
      if (a.is_i32()) a = Value::MakeI64(a.as_i32());
      if (b.is_i32()) b = Value::MakeI64(b.as_i32());
      int64_t av = a.as_i64();
      int64_t bv = b.as_i64();
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    case Opcode::kCmpP: {
      // For pointers, compare refs
      if (!a.is_ref() && !a.is_null()) {
        throw std::runtime_error("CmpP expects ref or null for first operand");
      }
      if (!b.is_ref() && !b.is_null()) {
        throw std::runtime_error("CmpP expects ref or null for second operand");
      }
      // Compare: null < ref, refs by numeric value
      int32_t av = a.is_null() ? 0 : a.as_ref();
      int32_t bv = b.is_null() ? 0 : b.as_ref();
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    case Opcode::kCmpU: {
      // Unsigned int32 comparison
      uint32_t av = static_cast<uint32_t>(a.as_i32());
      uint32_t bv = static_cast<uint32_t>(b.as_i32());
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    case Opcode::kCmpUL: {
      // Unsigned int64 comparison
      if (a.is_i32()) a = Value::MakeI64(a.as_i32());
      if (b.is_i32()) b = Value::MakeI64(b.as_i32());
      uint64_t av = static_cast<uint64_t>(a.as_i64());
      uint64_t bv = static_cast<uint64_t>(b.as_i64());
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    default:
      throw std::runtime_error("Unsupported comparison opcode");
  }
}

Value Interpreter::EvalPhi(const Node* n) {
  Logger::Info("      EvalPhi: Phi node " + std::to_string(n->id()));
  // Outside of explicit back-edge update mode, Phi cycles are a bug in our
  // predecessor tracking / input selection and should be reported, not crash.
  const bool in_update_for_this_region = in_phi_update_ &&
                                         updating_region_ != nullptr &&
                                         n->region_input() == updating_region_;
  Logger::Info("        in_update_for_this_region=" +
               std::string(in_update_for_this_region ? "true" : "false"));
  if (!in_update_for_this_region) {
    if (phi_eval_stack_.count(n) > 0) {
      throw std::runtime_error("Cyclic Phi evaluation detected (phi=" +
                               std::to_string(n->id()) + ")");
    }
    struct PhiStackGuard {
      Interpreter* interp;
      const Node* phi;
      ~PhiStackGuard() { interp->phi_eval_stack_.erase(phi); }
    } stack_guard{this, n};
    phi_eval_stack_.insert(n);

    // Continue with normal Phi logic.
  }

  // During loop back-edge Phi updates, we conceptually compute "next" Phi
  // values by substituting all loop Phis with their previous-iteration values
  // on the RHS. That means:
  // - any Phi in the loop (other than the one we're currently updating) should
  //   read as the old value
  // - recursive self-references while computing a Phi should also read as old
  if (in_phi_update_ && updating_region_ != nullptr &&
      n->region_input() == updating_region_) {
    auto it_old = phi_old_values_.find(n);
    if (it_old != phi_old_values_.end()) {
      if (n != updating_phi_ || phi_update_active_.count(n) > 0) {
        return it_old->second;
      }
    }
  }

  // Phi selects value based on which control predecessor was taken
  // Use schema-aware accessors
  Node* region = n->region_input();
  if (!region || region->opcode() != Opcode::kRegion) {
    // Simplified case: no region, just take first value
    auto values = n->phi_values();
    if (values.empty()) {
      throw std::runtime_error("Phi node has no value inputs");
    }
    return EvalNode(values[0]);
  }

  // Determine which control predecessor was taken
  auto it = region_predecessor_.find(region);
  if (it == region_predecessor_.end()) {
    // No predecessor recorded - this shouldn't happen in proper execution
    // Fall back to first value
    Logger::Warn("Phi node " + std::to_string(n->id()) +
                 ": no predecessor recorded for Region " +
                 std::to_string(region->id()) + ", using first value");
    auto values = n->phi_values();
    if (values.empty()) {
      throw std::runtime_error("Phi node has no value inputs");
    }
    return EvalNode(values[0]);
  }

  const Node* active_pred = it->second;
  Logger::Trace("EvalPhi: Phi " + std::to_string(n->id()) + " in Region " +
                std::to_string(region->id()) +
                " active predecessor = " + std::to_string(active_pred->id()) +
                " (region_inputs=" + std::to_string(region->num_inputs()) +
                ", phi_inputs=" + std::to_string(n->num_inputs()) + ")");

  // IMPORTANT: Phi inputs are positionally aligned with Region inputs.
  // Region:   input[i]   is the i-th control predecessor.
  // Phi:      input[0]   is the Region; input[i+1] is the value for pred i.
  // Both Region and Phi may contain a Region self-edge placeholder for loops;
  // if we skip that predecessor, we must skip the corresponding Phi value at
  // the same index too. Using compacted vectors breaks this alignment and can
  // select the self-referential Phi input on the entry path (infinite
  // recursion).
  const bool allow_self = in_phi_update_ && updating_region_ != nullptr &&
                          n->region_input() == updating_region_;
  Logger::Info("        Selecting Phi input, allow_self=" +
               std::string(allow_self ? "true" : "false"));
  const Node* selected =
      SelectPhiInputNode(n, active_pred, /*allow_self=*/allow_self);
  Logger::Info("        Selected node " +
               (selected ? std::to_string(selected->id()) : "NULL"));
  if (selected == nullptr) {
    throw std::runtime_error(
        "Phi node: could not select input (phi=" + std::to_string(n->id()) +
        ", region=" + std::to_string(region->id()) +
        ", active_pred=" + std::to_string(active_pred->id()) + ")");
  }
  if (!allow_self && selected == n) {
    throw std::runtime_error(
        "Phi node: selected self reference outside update mode (phi=" +
        std::to_string(n->id()) + ")");
  }

  struct ActivePhiGuard {
    Interpreter* interp;
    const Node* phi;
    bool active;
    ~ActivePhiGuard() {
      if (active) {
        interp->phi_update_active_.erase(phi);
      }
    }
  } guard{this, n,
          in_phi_update_ && updating_region_ != nullptr &&
              n->region_input() == updating_region_ && n == updating_phi_};
  if (guard.active) {
    phi_update_active_.insert(n);
  }

  Logger::Info("        About to EvalNode on selected=" +
               std::to_string(selected->id()) + " (" +
               OpcodeToString(selected->opcode()) + ")");
  Value result = EvalNode(selected);
  Logger::Info("        EvalNode complete");
  return result;
}

Value Interpreter::EvalBool(const Node* n) {
  // Bool node converts comparison result to boolean
  // Use schema-aware accessor
  auto value_inputs = n->value_inputs();

  if (value_inputs.empty()) {
    throw std::runtime_error("Bool node needs comparison value input");
  }

  Node* cmp_node = value_inputs[0];
  if (!cmp_node) {
    throw std::runtime_error("Bool node comparison input is null");
  }

  Value cmp_result = EvalNode(cmp_node);
  if (!cmp_result.is_i32()) {
    throw std::runtime_error("Bool node expects i32 comparison result");
  }

  int32_t cmp_val = cmp_result.as_i32();

  // Get mask property (condition code)
  int32_t mask = 0;
  if (n->has_prop("mask")) {
    mask = std::get<int32_t>(n->prop("mask"));
  } else if (n->has_prop("dump_spec")) {
    // Parse dump_spec for condition (e.g., "[le]")
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    // Map dump_spec to mask
    // le = LT|EQ = 1|2 = 3, gt = 4, ge = GT|EQ = 4|2 = 6, etc.
    if (spec.find("le") != std::string::npos) {
      mask = 3;  // LT | EQ
    } else if (spec.find("lt") != std::string::npos) {
      mask = 1;  // LT
    } else if (spec.find("ge") != std::string::npos) {
      mask = 6;  // GT | EQ
    } else if (spec.find("gt") != std::string::npos) {
      mask = 4;  // GT
    } else if (spec.find("eq") != std::string::npos) {
      mask = 2;  // EQ
    } else if (spec.find("ne") != std::string::npos) {
      mask = 5;  // LT | GT
    }
  }

  // HotSpot condition codes (bit encoding):
  // Bit 0 (value 1): LT (less than)
  // Bit 1 (value 2): EQ (equal)
  // Bit 2 (value 4): GT (greater than)
  // Examples: NE = LT|GT = 1|4 = 5, LE = LT|EQ = 1|2 = 3, GE = GT|EQ = 4|2 = 6
  // Comparison returns: -1 (less), 0 (equal), 1 (greater)
  bool result = false;
  if (cmp_val < 0 && (mask & 1))
    result = true;  // LT
  else if (cmp_val == 0 && (mask & 2))
    result = true;  // EQ
  else if (cmp_val > 0 && (mask & 4))
    result = true;  // GT

  return Value::MakeBool(result);
}

Value Interpreter::EvalCMove(const Node* n) {
  // CMoveI/L/P: conditional move
  // Use schema-aware accessor
  auto value_inputs = n->value_inputs();
  if (value_inputs.size() < 3) {
    throw std::runtime_error("CMove needs 3 value inputs");
  }

  Value cond = EvalNode(value_inputs[0]);
  if (!cond.is_bool()) {
    throw std::runtime_error("CMove condition must be boolean");
  }

  if (cond.as_bool()) {
    return EvalNode(value_inputs[1]);
  } else {
    return EvalNode(value_inputs[2]);
  }
}

Value Interpreter::EvalConv2B(const Node* n) {
  // Conv2B: Convert any value to boolean (0 -> 0, non-zero -> 1)
  // Use schema-aware accessor
  auto value_inputs = n->value_inputs();
  if (value_inputs.empty()) {
    throw std::runtime_error("Conv2B needs value input");
  }

  Value input = EvalNode(value_inputs[0]);

  // Convert to boolean: 0 -> 0, non-zero -> 1
  if (input.is_i32()) {
    return Value::MakeI32(input.as_i32() != 0 ? 1 : 0);
  } else if (input.is_i64()) {
    return Value::MakeI32(input.as_i64() != 0 ? 1 : 0);
  } else if (input.is_ref()) {
    return Value::MakeI32(input.as_ref() != 0 ? 1 : 0);
  } else if (input.is_null()) {
    return Value::MakeI32(0);
  } else if (input.is_bool()) {
    return Value::MakeI32(input.as_bool() ? 1 : 0);
  } else {
    throw std::runtime_error("Conv2B: unsupported input type");
  }
}

Value Interpreter::EvalNoOp(const Node* n) {
  // Opaque1: optimization marker, pass through value
  // Use value_inputs() which respects schema and skips non-value inputs
  auto vals = n->value_inputs();
  if (!vals.empty()) {
    // Pass through the first value input
    return EvalNode(vals[0]);
  }
  // If no value inputs, return a dummy value (shouldn't happen in practice)
  return Value::MakeI32(0);
}

Value Interpreter::EvalThreadLocal(const Node* /*n*/) {
  // ThreadLocal: thread-local variable access
  // For prototype: return a dummy reference (null)
  // In real implementation, would need to track thread-local storage
  return Value::MakeNull();
}

Value Interpreter::EvalCallStaticJava(const Node* n) {
  // CallStaticJava: static method call
  // Most of these in C2 graphs are uncommon_trap (deopt guards)
  // Check if it's an uncommon_trap and skip it
  if (n->has_prop("dump_spec")) {
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    if (spec.find("uncommon_trap") != std::string::npos) {
      // Uncommon trap - assume it doesn't fire, return dummy value
      return Value::MakeI32(0);
    }
  }

  // Real method call - not supported in prototype
  throw std::runtime_error(
      "CallStaticJava: real method calls not supported in prototype");
}

Value Interpreter::EvalHalt(const Node* /*n*/) {
  // Halt: abnormal termination (e.g., unhandled exception)
  throw std::runtime_error("Program reached Halt node (abnormal termination)");
}

Value Interpreter::EvalAllocate(const Node* /*n*/) {
  // Allocate object: input(0) = control
  // Returns a fresh reference
  // Note: Control dependency is implicit; concrete execution doesn't check it
  Ref new_ref = heap_.AllocateObject();
  return Value::MakeRef(new_ref);
}

Value Interpreter::EvalAllocateArray(const Node* n) {
  // AllocateArray: input(0) = control, input(1) = length
  if (n->num_inputs() < 2) {
    throw std::runtime_error("AllocateArray needs length input");
  }

  Value len_val = EvalNode(n->input(1));
  if (!len_val.is_i32()) {
    throw std::runtime_error("Array length must be i32");
  }

  int32_t length = len_val.as_i32();
  if (length < 0) {
    throw EvalException("Negative array length");
  }

  Ref arr_ref = heap_.AllocateArray(length);
  return Value::MakeRef(arr_ref);
}

Value Interpreter::EvalLoad(const Node* n) {
  // Load: input(0) = control, input(1) = memory, input(2) = base,
  //       [optional input(3) = index for arrays]
  if (n->num_inputs() < 3) {
    throw std::runtime_error("Load needs at least control, memory, and base");
  }

  // Clear memory chain visited set before processing this load's memory chain
  memory_chain_visited_.clear();

  // Process memory chain (execute any Store nodes in the chain)
  Node* mem = n->input(1);
  ProcessMemoryChain(mem);

  // Get base object/array
  Value base_val = EvalNode(n->input(2));
  if (!base_val.is_ref()) {
    throw std::runtime_error("Load base must be a reference");
  }
  Ref base = base_val.as_ref();

  // Check if this is array access
  // C2 can indicate arrays in multiple ways:
  // 1. Explicit 'array' property
  // 2. dump_spec contains "@type[...]" (array type signature)
  // 3. Address is computed via AddP (pointer arithmetic for array indexing)
  bool is_array = false;
  if (n->has_prop("array")) {
    is_array = std::get<bool>(n->prop("array"));
  } else if (n->has_prop("dump_spec")) {
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    is_array = (spec.find('[') != std::string::npos);  // Array type signature
  } else if (n->num_inputs() >= 3) {
    // Check if address input is AddP (array address arithmetic)
    const Node* addr = n->input(2);
    is_array = (addr && addr->opcode() == Opcode::kAddP);
  }

  if (is_array) {
    // Array access via AddP: the address node encodes both base and index
    // We need to extract the actual array base and index from AddP inputs
    if (n->num_inputs() >= 4) {
      // Traditional array load: input[2]=base, input[3]=index
      Value idx_val = EvalNode(n->input(3));
      if (!idx_val.is_i32()) {
        throw std::runtime_error("Array index must be i32");
      }
      int32_t index = idx_val.as_i32();
      Value elem = heap_.ReadArray(base, index);
      return elem;
    } else if (n->num_inputs() == 3 && n->input(2)->opcode() == Opcode::kAddP) {
      // Optimized array load via AddP
      // AddP structure: input[1]=base_array, input[2]=offset/index,
      // input[3]=scale
      const Node* addp = n->input(2);
      if (addp->num_inputs() < 3) {
        throw std::runtime_error(
            "AddP for array access needs at least 3 inputs");
      }

      // Extract base array from AddP input[1]
      Value actual_base = EvalNode(addp->input(1));
      if (!actual_base.is_ref()) {
        throw std::runtime_error("AddP base must be array reference");
      }

      // Extract index from AddP computation by recursively searching for the
      // index Pattern: LShiftL(ConvI2L(index), scale) or similar
      std::function<bool(const Node*, int32_t&)> extract_index;
      extract_index = [&](const Node* node, int32_t& out_index) -> bool {
        if (!node) return false;

        Opcode op = node->opcode();

        // If this is a shift operation, check its first input
        if (op == Opcode::kLShiftL || op == Opcode::kLShiftI) {
          if (node->num_inputs() >= 2) {
            const Node* val = node->input(1);
            if (val && val->opcode() == Opcode::kConvI2L &&
                val->num_inputs() >= 2) {
              Value idx = EvalNode(val->input(1));
              if (idx.is_i32()) {
                out_index = idx.as_i32();
                return true;
              }
            }
            // Try evaluating directly
            Value idx = EvalNode(val);
            if (idx.is_i32()) {
              out_index = idx.as_i32();
              return true;
            }
          }
        }

        // If this is AddP, recursively check its inputs
        if (op == Opcode::kAddP) {
          for (size_t i = 1; i < node->num_inputs(); ++i) {
            if (extract_index(node->input(i), out_index)) {
              return true;
            }
          }
        }

        // Try evaluating this node directly
        Value val = EvalNode(node);
        if (val.is_i32()) {
          out_index = val.as_i32();
          return true;
        }

        return false;
      };

      int32_t index = -1;
      if (!extract_index(addp, index)) {
        throw std::runtime_error(
            "Could not extract i32 array index from AddP address computation");
      }

      // Successfully extracted index, now read from array
      Value elem = heap_.ReadArray(actual_base.as_ref(), index);
      return elem;
    } else {
      throw std::runtime_error("Array load structure not recognized");
    }
  } else {
    // Field access
    if (!n->has_prop("field")) {
      throw std::runtime_error("Load needs field property");
    }
    std::string field = std::get<std::string>(n->prop("field"));

    Value field_val = heap_.ReadField(base, field);
    return field_val;
  }
}

void Interpreter::ProcessMemoryChain(const Node* mem) {
  // Process Store nodes in memory chain
  if (!mem) return;

  // Cycle detection: memory chains can have cycles through memory Phis
  if (memory_chain_visited_.count(mem) > 0) {
    return;  // Already processed this node in this chain walk
  }
  memory_chain_visited_.insert(mem);

  Opcode op = mem->opcode();

  // If this is a Store, execute it
  if (op == Opcode::kStoreB || op == Opcode::kStoreC || op == Opcode::kStoreI ||
      op == Opcode::kStoreL || op == Opcode::kStoreP || op == Opcode::kStoreN) {
    EvalStore(mem);
  }
  // If Store has a memory input, process that first (in reverse order)
  if (mem->num_inputs() >= 2) {
    ProcessMemoryChain(mem->input(1));
  }
}

void Interpreter::EvalStore(const Node* n) {
  // Store: input(0) = control, input(1) = memory, input(2) = base,
  //        input(3) = value (field) or input(3) = index, input(4) = value
  //        (array)

  if (n->num_inputs() < 4) {
    throw std::runtime_error(
        "Store needs at least control, memory, base, value");
  }

  // Get base object/array
  Value base_val = EvalNode(n->input(2));
  if (!base_val.is_ref()) {
    throw std::runtime_error("Store base must be a reference");
  }
  Ref base = base_val.as_ref();

  // Check if this is array access
  bool is_array = n->has_prop("array") && std::get<bool>(n->prop("array"));

  if (is_array) {
    // Array access: input(3) = index, input(4) = value
    if (n->num_inputs() < 5) {
      throw std::runtime_error("Array store needs index and value");
    }

    Value idx_val = EvalNode(n->input(3));
    if (!idx_val.is_i32()) {
      throw std::runtime_error("Array index must be i32");
    }
    int32_t index = idx_val.as_i32();

    Value value = EvalNode(n->input(4));
    heap_.WriteArray(base, index, value);
  } else {
    // Field access: input(3) = value
    if (!n->has_prop("field")) {
      throw std::runtime_error("Store needs field property");
    }
    std::string field = std::get<std::string>(n->prop("field"));

    Value value = EvalNode(n->input(3));
    heap_.WriteField(base, field, value);
  }
}

}  // namespace sun
