# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Unit tests
add_executable(sun_unit_tests
    unit/ir/test_opcode.cpp
    unit/ir/test_node.cpp
    unit/ir/test_graph.cpp
    unit/igv/test_parser.cpp
    unit/interp/test_value.cpp
    unit/interp/test_heap.cpp
    unit/interp/test_interpreter.cpp
)
target_link_libraries(sun_unit_tests
    PRIVATE
    sunigv
    suninterp
    sunir
    sunutil
    GTest::gtest_main
)

# Integration tests
add_executable(sun_integration_tests
    integration/test_simple_add.cpp
    integration/test_conditionals.cpp
    integration/test_memory.cpp
)
target_link_libraries(sun_integration_tests
    PRIVATE
    sunigv
    suninterp
    sunir
    sunutil
    GTest::gtest_main
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(sun_unit_tests)
gtest_discover_tests(sun_integration_tests)
