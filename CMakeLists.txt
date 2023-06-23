cmake_minimum_required(VERSION 3.16)

project(SimplePTN
        VERSION 1.0.0
        DESCRIPTION "Lightweight Petri Net template library"
        LANGUAGES CXX)

# If this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    # use c++ instead of g++
    set(CMAKE_CXX_EXTENSIONS OFF)

endif()

# main library
add_library(SimplePTN INTERFACE)
target_include_directories(SimplePTN INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(SimplePTN INTERFACE cxx_std_17)

## Install part ################################################################
# gather install targets
install(TARGETS SimplePTN EXPORT SimplePTNTargets)
install(DIRECTORY include/SimplePTN DESTINATION include)
# make targets available
install(EXPORT SimplePTNTargets
  FILE SimplePTNTargets.cmake
  DESTINATION lib/cmake/SimplePTN
)
# generate the config file that is includes the exports
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/SimplePTNConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/SimplePTN"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
# generate the version matching file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SimplePTNConfigVersion.cmake"
  VERSION "${SimplePTN_VERSION_MAJOR}.${SimplePTN_VERSION_MINOR}"
  COMPATIBILITY AnyNewerVersion
)
# install helper files
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/SimplePTNConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/SimplePTNConfigVersion.cmake
  DESTINATION lib/cmake/SimplePTN
  )
# make targets available, if this is a subdirectory
export(EXPORT SimplePTNTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/SimplePTNTargets.cmake"
)
################################################################################

# include tests
find_package(Catch2)
if(Catch2_FOUND)
    include(CTest)
    include(Catch)
    add_subdirectory(test)
else()
    message(WARNING "Catch2 not found, can NOT test the library!")
endif()

# examples
#add_subdirectory(examples)

# only use code coverage with clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Include to enable code coverage for SonarQube
  include(code-coverage.cmake)
  set(CODE_COVERAGE ON)
  target_code_coverage(simpleptn_test) 
endif()
