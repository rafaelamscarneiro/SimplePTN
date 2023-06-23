find_package(Threads REQUIRED)

add_executable(simpleptn_test
  ${PROJECT_SOURCE_DIR}/test/SimplePTN/petri_net.cpp
  ${PROJECT_SOURCE_DIR}/test/SimplePTN/place.cpp
  ${PROJECT_SOURCE_DIR}/test/SimplePTN/transition.cpp
  ${PROJECT_SOURCE_DIR}/test/main.cpp
)
target_link_libraries(simpleptn_test SimplePTN Catch2::Catch2 Threads::Threads)

# Let CTest discover the Catch2 test cases
catch_discover_tests(simpleptn_test)
