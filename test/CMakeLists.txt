find_package(Threads REQUIRED)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

add_executable(simpleptn_test
  ${PROJECT_SOURCE_DIR}/test/SimplePTN/petri_net.cpp
  ${PROJECT_SOURCE_DIR}/test/SimplePTN/place.cpp
  ${PROJECT_SOURCE_DIR}/test/SimplePTN/transition.cpp
  ${PROJECT_SOURCE_DIR}/test/main.cpp
)
target_link_libraries(simpleptn_test SimplePTN Catch2::Catch2 Threads::Threads)

# Let CTest discover the Catch2 test cases
catch_discover_tests(simpleptn_test)
