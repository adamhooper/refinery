INCLUDE(FindPkgConfig)

option(LICENSE_GPL "Build GPL components" OFF)

project(refinery)
cmake_minimum_required(VERSION 2.8.2)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-Wall")

  if (CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  endif()
endif()

# Includes
include_directories(${refinery_SOURCE_DIR}/include)
if (LICENSE_GPL)
  include_directories(${refinery_SOURCE_DIR}/include/licensed/gpl)
endif()

# Shared library
file(GLOB sources src/*.cc)
if (LICENSE_GPL)
  pkg_check_modules(EXIV2 REQUIRED exiv2>=0.20)
  include_directories(SYSTEM ${EXIV2_INCLUDE_DIRS})

  file(GLOB gpl_sources src/licensed/gpl/*.cc)
  set(sources ${sources} ${gpl_sources})
endif()

add_library(refinery-0.1 SHARED ${sources})
set_target_properties(refinery-0.1 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

if (LICENSE_GPL)
  target_link_libraries(refinery-0.1 exiv2)
endif()

# Binaries
if (LICENSE_GPL)
  add_executable(bin/raw2ppm util/licensed/gpl/raw2ppm.cc)
  add_dependencies(bin/raw2ppm refinery-0.1)
  target_link_libraries(bin/raw2ppm exiv2 refinery-0.1)
endif()

# Testing
enable_testing()
find_package(GTest REQUIRED)

file(GLOB gtest_sources test/*.cc)
if (LICENSE_GPL)
  file(GLOB gtest_sources_gpl test/licensed/gpl/*.cc)
  set(gtest_sources ${gtest_sources} ${gtest_sources_gpl})
endif()

add_executable(test/gtest_main ${gtest_sources})
add_dependencies(test/gtest_main refinery-0.1)
include_directories(${GTEST_INCLUDE_DIRS})
target_link_libraries(test/gtest_main
    pthread ${GTEST_BOTH_LIBRARIES} refinery-0.1)
if (LICENSE_GPL)
  target_link_libraries(test/gtest_main exiv2)
endif()
add_test(AllTestsInGtestMain test/gtest_main)

add_subdirectory(bindings)

# Installation
install(TARGETS refinery-0.1
        LIBRARY DESTINATION lib)
install(DIRECTORY include/refinery
        DESTINATION include/refinery-0.1
        FILES_MATCHING PATTERN "*.h")
if (LICENSE_GPL)
  install(TARGETS bin/raw2ppm
          RUNTIME DESTINATION bin)
  install(DIRECTORY include/licensed/gpl/refinery
          DESTINATION include/refinery-0.1)
endif()
