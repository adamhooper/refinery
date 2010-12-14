INCLUDE(FindPkgConfig)

project(refinery)
cmake_minimum_required(VERSION 2.8.2)

if(CMAKE_COMPILER_IS_GNUCXX)
#set(CMAKE_CXX_FLAGS "-fopenmp")
endif()

include_directories(${refinery_SOURCE_DIR}/include)

pkg_check_modules(EXIV2 REQUIRED exiv2>=0.20)
include_directories(SYSTEM ${EXIV2_INCLUDE_DIRS})

add_library(refinery-0.1 SHARED src/camera.cc src/filters.cc src/image.cc src/input.cc src/interpolate.cc src/output.cc src/unpack.cc)
set_target_properties(refinery-0.1 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
target_link_libraries(refinery-0.1 exiv2)

add_executable(bin/raw2ppm util/raw2ppm.cc)
add_dependencies(bin/raw2ppm refinery-0.1)
target_link_libraries(bin/raw2ppm exiv2 refinery-0.1)

file(GLOB gtest_sources test/*.cc)
enable_testing()
find_package(GTest REQUIRED)
add_executable(test/gtest_main ${gtest_sources})
add_dependencies(test/gtest_main refinery-0.1)
include_directories(${GTEST_INCLUDE_DIRS})
target_link_libraries(test/gtest_main
    pthread ${GTEST_BOTH_LIBRARIES} exiv2 refinery-0.1)
add_test(AllTestsInGtestMain test/gtest_main)

install(TARGETS refinery-0.1 bin/raw2ppm
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(DIRECTORY include/refinery
        DESTINATION include/refinery-0.1
        FILES_MATCHING PATTERN "*.h")
