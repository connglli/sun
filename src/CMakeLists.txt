# Utility library
add_library(sunutil
    util/logging.cpp
)

# IR library (shared between suni and suntv)
add_library(sunir
    ir/opcode.cpp
    ir/node.cpp
    ir/graph.cpp
    ir/types.cpp
)
target_link_libraries(sunir PUBLIC sunutil)

# IGV parser library (shared between suni and suntv)
add_library(sunigv
    igv/parser.cpp
    igv/canonicalizer.cpp
    igv/igv_util.cpp
    igv/java2igv.cpp
)
target_link_libraries(sunigv PUBLIC sunir sunutil pugixml::pugixml)

# Interpreter library (for suni)
add_library(suninterp
    interp/value.cpp
    interp/heap.cpp
    interp/outcome.cpp
    interp/interpreter.cpp
    interp/evaluator.cpp
)
target_link_libraries(suninterp PUBLIC sunir sunutil)
