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

# Optional <numberFocusToTrace> must default to 0 so existing scenarios parse unchanged.
add_executable(trace_tests TraceParametersTest.cpp)
target_link_libraries(trace_tests transm_core gtest_main)
add_test(NAME trace_tests COMMAND trace_tests)

# Partner draws must pick the same people, in the same order, as the linear scan did.
add_executable(mixing_tests BucketSexualMixingTest.cpp)
target_link_libraries(mixing_tests transm_core gtest_main)
add_test(NAME mixing_tests COMMAND mixing_tests)

# The progress bar must not abort a run when the run-time estimate leaves [0, 1].
add_executable(runtime_tests RunTimePredictorTest.cpp)
target_link_libraries(runtime_tests gtest_main)
add_test(NAME runtime_tests COMMAND runtime_tests)

# The count index behind partner draws must agree with brute-force sums and walks.
add_executable(countindex_tests CountIndexTest.cpp)
target_link_libraries(countindex_tests gtest_main)
add_test(NAME countindex_tests COMMAND countindex_tests)

# Force of infection: each protection (condom, microbicide or circumcision, PrEP at
# its adherence level) must lower the risk on its own.
add_executable(infectionrisk_tests InfectionRiskTest.cpp)
target_link_libraries(infectionrisk_tests gtest_main)
add_test(NAME infectionrisk_tests COMMAND infectionrisk_tests)

# New people must start with every member set, e.g. how often they were chosen.
add_executable(entityinit_tests EntityInitTest.cpp)
target_link_libraries(entityinit_tests transm_core gtest_main)
add_test(NAME entityinit_tests COMMAND entityinit_tests)

# Optional: the same scenario through a reference build and this build must give
# identical TRANSM outputs. Configure with
#   -DTRANSM_REGRESSION_REFERENCE=<reference transm binary>
#   -DTRANSM_REGRESSION_SCENARIO=<scenario .xml>
#   -DTRANSM_REGRESSION_CEPAC=<CEPAC input directory>
# and optionally TRANSM_REGRESSION_SIZE / TRANSM_REGRESSION_DURATION to shrink it.
if (TRANSM_REGRESSION_REFERENCE AND TRANSM_REGRESSION_SCENARIO AND TRANSM_REGRESSION_CEPAC)
    add_test(NAME regression_outputs
             COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/regression/compare_binaries.sh
                     ${TRANSM_REGRESSION_REFERENCE} $<TARGET_FILE:transm>
                     ${TRANSM_REGRESSION_SCENARIO} ${TRANSM_REGRESSION_CEPAC}
                     ${TRANSM_REGRESSION_SIZE} ${TRANSM_REGRESSION_DURATION})
    # "PASS WITH WARNING" (CEPAC summaries or an exit-time crash) also passes.
    set_tests_properties(regression_outputs PROPERTIES PASS_REGULAR_EXPRESSION "\nPASS")
endif()

# Model checks: small runs of the Miami scenario (tests/model/scenarios) judged by
# bookkeeping rules, known answers and reference bands, not by identical outputs.
# Twelve 2,000-person runs, about 3 minutes on 4 cores. Run with: ctest -L model
find_package(Python3 COMPONENTS Interpreter)
if (Python3_Interpreter_FOUND)
    add_test(NAME model_checks
             COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/model/model_checks.py
                     --transm $<TARGET_FILE:transm>)
    set_tests_properties(model_checks PROPERTIES LABELS model TIMEOUT 3600)
endif()

# Fast tests of single classes and functions. Run with: ctest -L unit
set_tests_properties(tests memory_tests trace_tests mixing_tests runtime_tests countindex_tests
                     infectionrisk_tests entityinit_tests
                     PROPERTIES LABELS unit)
if (TEST regression_outputs)
    set_tests_properties(regression_outputs PROPERTIES LABELS regression)
endif()
