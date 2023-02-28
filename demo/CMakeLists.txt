include_directories(../puck .. /opt/intel/oneapi/mkl/latest/include /usr/local/include)
file(
    GLOB PUCK_CPP_FILES
    "../puck/base/*.cpp"
    "../puck/gflags/*.cpp"
    "../puck/hierarchical_cluster/*.cpp"
    "../puck/puck/*.cpp"
    "../puck/tinker/*.cpp"
    "../puck/tinker/*/*.cc"
    "../puck/*.cpp"
)

add_executable(search_client)
target_sources(search_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/search_client.cpp)
target_sources(search_client PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(search_client PUBLIC)

find_package(OpenMP REQUIRED)
target_link_libraries(search_client  OpenMP::OpenMP_CXX)

find_package(MKL)
if(MKL_FOUND)
target_link_libraries(search_client  ${MKL_LIBRARIES})
else()
find_package(BLAS REQUIRED)
target_link_libraries(search_client  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(search_client  ${LAPACK_LIBRARIES})
endif()

add_executable(insert_demo)
target_sources(insert_demo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/insert_demo.cpp)
target_sources(insert_demo PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(insert_demo PUBLIC)
find_package(OpenMP REQUIRED)
target_link_libraries(insert_demo  OpenMP::OpenMP_CXX)

find_package(MKL)
if(MKL_FOUND)
target_link_libraries(insert_demo  ${MKL_LIBRARIES})
else()
find_package(BLAS REQUIRED)
target_link_libraries(insert_demo  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(insert_demo  ${LAPACK_LIBRARIES})
endif()   

#set output dir
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/bin)  

#copy file
file(GLOB conFile  "conf")
file(COPY ${conFile} DESTINATION ${PROJECT_SOURCE_DIR}/output)                                                                                
