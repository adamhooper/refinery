find_package(PythonLibs)

if (PYTHON_INCLUDE_DIR)
  execute_process(COMMAND python -c "import site;print(site.getsitepackages()[0])"
      OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
      OUTPUT_STRIP_TRAILING_WHITESPACE)

  include_directories(${PYTHON_INCLUDE_PATH})
  set_source_files_properties(refinery.i
      PROPERTIES CPLUSPLUS ON
      SWIG_FLAGS "-Wextra")

  swig_add_module(refinery python refinery.i)
  swig_link_libraries(refinery refinery-0.1 ${PYTHON_LIBRARIES})

  install(TARGETS ${SWIG_MODULE_refinery_REAL_NAME}
      LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
  install(FILES refinery.py
      DESTINATION ${PYTHON_SITE_PACKAGES})
else()
  message(STATUS "Python 2.7 headers not found. Install python-dev package to compile Python bindings.")
endif()
