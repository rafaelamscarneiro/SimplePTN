find_package(Threads REQUIRED)

add_executable(simpleptn_examples_harbor_terminal
  ${PROJECT_SOURCE_DIR}/examples/harbor_terminal.cpp
)
target_link_libraries(simpleptn_examples_harbor_terminal SimplePTN Threads::Threads)
