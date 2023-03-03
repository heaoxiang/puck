include_directories(../puck ..)
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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-std=c++11 -Wunused-variable -Wno-sign-compare -Wno-format -g0 -O3")
set(CMAKE_C_FLAGS "-Wno-missing-field-initializers -Wno-unused-but-set-variable -Wunused-variable -Wno-sign-compare")
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

add_compile_options(-fopenmp -lpthread -lcrypto -lrt -lgfortran -ldl -lz -fPIC -rdynamic)
add_compile_options(-O2 -g -pipe -W -Wall -fPIC -Wno-unused-but-set-variable -march=native -Wno-deprecated-declarations -Wno-unused-parameter -Wno-logical-not-parentheses)

add_executable(search_client)
target_sources(search_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/search_client.cpp)
target_sources(search_client PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(search_client PUBLIC)

target_link_libraries(search_client gflags)
target_link_libraries(search_client glog)

find_package(OpenMP REQUIRED)
target_link_libraries(search_client  OpenMP::OpenMP_CXX)

find_package(MKL REQUIRED)
if(MKL_FOUND)
target_link_libraries(search_client  ${MKL_LIBRARIES})

find_package(BLAS REQUIRED)
target_link_libraries(search_client  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(search_client  ${LAPACK_LIBRARIES})
else()
MESSAGE(FATAL_ERROR "Could not find MKL!")
endif()

add_executable(insert_demo)
target_sources(insert_demo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/insert_demo.cpp)
target_sources(insert_demo PUBLIC ${PUCK_CPP_FILES})
target_compile_definitions(insert_demo PUBLIC)
find_package(OpenMP REQUIRED)
target_link_libraries(insert_demo  OpenMP::OpenMP_CXX)

target_link_libraries(insert_demo gflags)
target_link_libraries(insert_demo glog)

find_package(MKL REQUIRED)
if(MKL_FOUND)
target_link_libraries(insert_demo  ${MKL_LIBRARIES})

find_package(BLAS REQUIRED)
target_link_libraries(insert_demo  ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(insert_demo  ${LAPACK_LIBRARIES})
else()
MESSAGE(FATAL_ERROR "Could not find MKL!")
endif()   

#set output dir
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/bin)  

#copy file
file(GLOB conFile  "conf")
file(COPY ${conFile} DESTINATION ${PROJECT_SOURCE_DIR}/output)                                                                                
