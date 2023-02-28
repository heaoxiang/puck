
file(
    GLOB PUCK_CPP_FILES
    "./base/*.cpp"
    "./gflags/*.cpp"
    "./hierarchical_cluster/*.cpp"
    "./puck/*.cpp"
    "./tinker/*.cpp"
    "./tinker/*/*.cc"
    "./*.cpp"
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
)

set(PUCK_SRC
    ${PUCK_CPP_FILES}
)

set(PUCK_HEADERS
    ${PUCK_HEAD_FILES}
)
include_directories(../)
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

find_package(MKL)
if(MKL_FOUND)
target_link_libraries(puck PUBLIC ${MKL_LIBRARIES})
else()
find_package(BLAS REQUIRED)
target_link_libraries(puck PUBLIC ${BLAS_LIBRARIES})

find_package(LAPACK REQUIRED)
target_link_libraries(puck PUBLIC ${LAPACK_LIBRARIES})
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

install(EXPORT puck-targets
DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/puck
)

# set output dir
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../output/lib)

#copy file
file(
    GLOB OUTPUT_HEAD_FILES
    "./base"
    "./gflags"
    "./hierarchical_cluster"
    "./puck"
    "./tinker"
    "./tinker/*/*.h"
    "./*.h"
    “../tools/*.h”
)
file(COPY ${OUTPUT_HEAD_FILES} DESTINATION ${PROJECT_SOURCE_DIR}/output/include)  
