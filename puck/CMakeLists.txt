set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-std=c++11 -Wunused-variable -Wno-format -g0 -O3")
set(CMAKE_C_FLAGS "-Wno-missing-field-initializers -Wno-unused-but-set-variable -Wunused-variable -Wno-sign-compare")
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
add_definitions(-DFINTEGER=int)

add_compile_options(-fopenmp -lpthread -lcrypto -lrt -lgfortran -ldl -lz -fPIC -rdynamic)
add_compile_options(-O2 -g -pipe -W -Wall -fPIC -Wno-unused-but-set-variable -march=native -Wno-deprecated-declarations -Wno-unused-parameter -Wunused-variable -Wno-logical-not-parentheses)

file(
    GLOB PUCK_CPP_FILES
    "./base/*.cpp"
    "./gflags/*.cpp"
    "./hierarchical_cluster/*.cpp"
    "./puck/*.cpp"
    "./tinker/*.cpp"
    "./tinker/*/*.cc"
    "./*.cpp"
    "./base/yael/*.c"
)
file(
    GLOB PUCK_HEAD_FILES
    "./base/*.h"
    "./gflags/*.h"
    "./hierarchical_cluster/*.h"
    "./puck/*.h"
    "./tinker/*.h"
    "./tinker/*/*.h"
    "./*.h"
    "./base/yael/*.h"
)

file(
    GLOB YAEL_SRC_FILES
    "./base/yael/binheap.c"
    "./base/yael/kmeans.c"
    "./base/yael/machinedeps.c"
    "./base/yael/matrix.c"
    "./base/yael/nn.c"
    "./base/yael/sorting.c"
    "./base/yael/vector.c"
)

set(PUCK_SRC
    ${PUCK_CPP_FILES}
)

set(PUCK_HEADERS
    ${PUCK_HEAD_FILES}
)

set(YAEL_SRC
    ${YAEL_SRC_FILES}
)

include_directories(../ ./base/yael)
add_library(yael ${YAEL_SRC})
target_link_libraries(yael PUBLIC ${BLAS_LIBRARIES})
target_link_libraries(yael PUBLIC ${LAPACK_LIBRARIES})
add_library(puck ${PUCK_SRC})

# Handle `#include <faiss/foo.h>`.
target_include_directories(puck PUBLIC
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

set_target_properties(puck PROPERTIES
POSITION_INDEPENDENT_CODE ON
WINDOWS_EXPORT_ALL_SYMBOLS ON
)
target_compile_definitions(puck PUBLIC FINTEGER=int)

target_link_libraries(puck PUBLIC gtest gflags)
target_link_libraries(puck PUBLIC ${GLOG_LIBRARIES})
target_link_libraries(puck PUBLIC ${GFLAGS_LIBRARIES})
    
find_package(OpenMP REQUIRED)
target_link_libraries(puck PUBLIC OpenMP::OpenMP_CXX)

find_package(MKL REQUIRED)
if(MKL_FOUND)
target_link_libraries(puck PUBLIC ${MKL_LIBRARIES})

find_package(BLAS REQUIRED)
target_link_libraries(puck PUBLIC ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(puck PUBLIC ${LAPACK_LIBRARIES})
else()
MESSAGE(FATAL_ERROR "Could not find MKL!")
endif()


target_link_libraries(puck PUBLIC gflags)

target_link_libraries(puck PUBLIC glog)

target_link_libraries(puck PUBLIC yael)

install(TARGETS puck
EXPORT puck-targets
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

foreach(header ${PUCK_HEADERS})
get_filename_component(dir ${header} DIRECTORY )
install(FILES ${header}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/puck/${dir}
)
endforeach()

#install(EXPORT puck-targets
#DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/puck
#)

# set output dir
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/lib)

file(GLOB PUCK_FOLDER "./*.h")
file(GLOB BASE_FOLDER "./base/*.h")
file(GLOB YAEL_FOLDER "./base/yael/*.h")
file(GLOB GFLAGS_FOLDER "./gflags/*.h")
file(GLOB PUCK_PUCK_FOLDER "./puck/*.h")
file(GLOB TINKER_FOLDER "./tinker/*.h")
file(GLOB METHOD_FOLDER "./tinker/method/*.h")
file(GLOB SPACE_FOLDER "./tinker/space/*.h")
file(GLOB HIERARCHICAL_FOLDER "./hierachical/*.h")

file(COPY ${PUCK_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include)  
file(COPY ${BASE_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/base)  
file(COPY ${YAEL_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/base/yael)  
file(COPY ${GFLAGS_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/gflags)  
file(COPY ${PUCK_PUCK_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/puck)  
file(COPY ${TINKER_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/tinker)  
file(COPY ${METHOD_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/tinker/method)  
file(COPY ${SPACE_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/tinker/space)  
file(COPY ${HIERARCHICAL_FOLDER}  DESTINATION ${PROJECT_SOURCE_DIR}/output/include/hierachical)  
