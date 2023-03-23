# SWIG compile for Python(3)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package(PythonLibs "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" EXACT REQUIRED)
message(STATUS "Found PYTHON_INCLUDE_DIR: ${PYTHON_INCLUDE_DIR}")
message(STATUS "Found PYTHON_LIBRARY: ${PYTHON_LIBRARY}")

include_directories(${PYTHON_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_SWIG_FLAGS "")

set(SWIG_INTERFACE "./py_puck_api.i")
file(GLOB SWIG_USED_CPP
        "../puck/*.cpp"
        "../puck/base/*.cpp"
        "../puck/base/yael/*.c"
        "../puck/gflags/*.cpp"
        "../puck/hierarchical_cluster/*.cpp"
        "../puck/puck/puck_index.cpp"
        "../puck/puck/quantization.cpp"
        "../puck/puck/utils_simd.cpp"

        "../puck/tinker/method/hnsw.cc"
        "../puck/tinker/method/hnsw_distfunc_opt.cc"
        "../puck/tinker/tinker_index.cpp"
        "./py_api_wrapper.cpp")

set_source_files_properties(${SWIG_INTERFACE} PROPERTIES CPLUSPLUS ON)
set_source_files_properties(${SWIG_INTERFACE} PROPERTIES SWIG_FLAGS "-includeall")

include_directories(./
                    ../
                    ${MKL_ROOT}
                    ${PYTHON_INCLUDE_DIR})

# add     py_puck_api.cpp ->  py_puck_api.py _PyPuck.so    in a row
swig_add_library(   PyPuck
                    LANGUAGE python
                    SOURCES ${SWIG_INTERFACE} ${SWIG_USED_CPP}
                    OUTFILE_DIR ./py_puck_api_cpp
                    OUTPUT_DIR ./)

# link all the libraries
swig_link_libraries(PyPuck ${PYTHON_LIBRARY} )
swig_link_libraries(PyPuck  OpenMP::OpenMP_CXX)
swig_link_libraries(PyPuck  ${MKL_LIBRARIES})

#copy file
file(GLOB py_puck_lib  
"${PROJECT_BINARY_DIR}/pyapi_wrapper/_PyPuck.so"
"${PROJECT_BINARY_DIR}/pyapi_wrapper/py_puck_api.py")

file(COPY ${py_puck_lib} DESTINATION ${PROJECT_BINARY_DIR}/output/lib/)    



set(PYPUCKLIB_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/puck)
install(FILES "${PROJECT_BINARY_DIR}/pyapi_wrapper/py_puck_api.py"
    DESTINATION ${PYPUCKLIB_INSTALL_LIBDIR})

install(TARGETS PyPuck
    ARCHIVE DESTINATION ${PYPUCKLIB_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${PYPUCKLIB_INSTALL_LIBDIR}
    )
