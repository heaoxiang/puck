include_directories(../)
file(
    GLOB PUCK_CPP_FILES
    "../puck/base/*.cpp"
    "../puck/gflags/*.cpp"
    "../puck/hierarchical_cluster/*.cpp"
    "../puck/puck/*.cpp"
    "../puck/tinker/*.cpp"
    "../puck/tinker/*/*.cc"
    "../puck/*.cpp"
    "../puck/base/yael/*.c"
)

add_executable(train)
target_sources(train PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/train.cpp)
target_sources(train PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(train PUBLIC)

target_link_libraries(train gflags)
target_link_libraries(train glog)

find_package(OpenMP REQUIRED)
target_link_libraries(train  OpenMP::OpenMP_CXX)

find_package(MKL REQUIRED)
if(MKL_FOUND)
target_link_libraries(train  ${MKL_LIBRARIES})

find_package(BLAS REQUIRED)
target_link_libraries(train  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(train  ${LAPACK_LIBRARIES})
else()
MESSAGE(FATAL_ERROR "Could not find MKL!")
endif()

add_executable(build)
target_sources(build PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/build.cpp)
target_sources(build PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(build PUBLIC)
find_package(OpenMP REQUIRED)
target_link_libraries(build  OpenMP::OpenMP_CXX)

target_link_libraries(build gflags)
target_link_libraries(build glog)

find_package(MKL REQUIRED)
if(MKL_FOUND)
target_link_libraries(build  ${MKL_LIBRARIES})

find_package(BLAS REQUIRED)
target_link_libraries(build  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(build  ${LAPACK_LIBRARIES})
else()
MESSAGE(FATAL_ERROR "Could not find MKL!")
endif()       

#set output dir
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/build_tools/bin)                                                                                  
#copy file
file(GLOB scriptFile  "script")
file(COPY ${scriptFile} DESTINATION ${PROJECT_SOURCE_DIR}/output/build_tools/)

file(GLOB demoConfFile  "demo/conf")
file(COPY ${demoConfFile} DESTINATION ${PROJECT_SOURCE_DIR}/output/build_tools/)    
