set(SWIG_EXECUTABLE swig2.0)
find_package(SWIG)

if (SWIG_FOUND)
  include(${SWIG_USE_FILE})
  add_subdirectory(python)
else()
  message(STATUS "Swig not found. Install Swig to compile language bindings.")
endif()
