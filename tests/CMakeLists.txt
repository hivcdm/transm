# Assuming GoogleTest has already been made available in the main CMakeLists.txt

# Create a test executable target from tests.cpp
add_executable(tests main-test.cpp TimeTest.cpp)

# Link GoogleTest libraries to the new target
target_link_libraries(tests gtest_main)

# Optionally, if you have additional libraries your tests need to link against, include them here
# target_link_libraries(SimpleCloth_test gtest_main your_additional_libraries)

# Register the executable with CTest
add_test(NAME tests COMMAND tests)

# Memory changes must preserve configuration values and individual interventions.
add_executable(memory_tests MemoryManagementTest.cpp)
target_link_libraries(memory_tests transm_core gtest_main)
add_test(NAME memory_tests COMMAND memory_tests)
