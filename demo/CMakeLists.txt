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


add_executable(puck_client)
target_sources(puck_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/puck_client.cpp)
target_sources(puck_client PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(puck_client PUBLIC)

find_package(OpenMP REQUIRED)
target_link_libraries(puck_client  OpenMP::OpenMP_CXX)

find_package(MKL)
if(MKL_FOUND)
target_link_libraries(puck_client  ${MKL_LIBRARIES})
else()
find_package(BLAS REQUIRED)
target_link_libraries(puck_client  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(puck_client  ${LAPACK_LIBRARIES})
endif()

add_executable(tinker_client)
target_sources(tinker_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinker_client.cpp)
target_sources(tinker_client PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(tinker_client PUBLIC)
find_package(OpenMP REQUIRED)
target_link_libraries(tinker_client  OpenMP::OpenMP_CXX)

find_package(MKL)
if(MKL_FOUND)
target_link_libraries(tinker_client  ${MKL_LIBRARIES})
else()
find_package(BLAS REQUIRED)
target_link_libraries(tinker_client  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(tinker_client  ${LAPACK_LIBRARIES})
endif()

add_executable(insert_demo)
target_sources(insert_demo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/insert_demo.cpp)
target_sources(insert_demo PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(insert_demo PUBLIC)
find_package(OpenMP REQUIRED)
target_link_libraries(tinker_client  OpenMP::OpenMP_CXX)

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
