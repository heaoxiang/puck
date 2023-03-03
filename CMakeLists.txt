cmake_minimum_required(VERSION 3.21)

project(puck)
set(CMAKE_CXX_STANDARD 14)

# minimum requirements
set(PYTHON_MINIMUM_VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# find python3
find_package(Python3 ${PYTHON_MINIMUM_VERSION} REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Found Python: ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}.${Python3_VERSION_PATCH}")

set(THIRD_PARTY_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_libs)

# config GIT_URL with github mirrors to speed up dependent repos clone
option(GIT_URL "Git URL to clone dependent repos" ${GIT_URL})
if(NOT GIT_URL)
    set(GIT_URL "https://github.com")
endif()

if (NOT Python3_SITELIB)
    message(FATAL_ERROR "site-packages not found. ")
else ()
    message(STATUS "site-packages: ${Python3_SITELIB}")
endif ()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-std=c++11 -Wunused-variable -Wno-format -g0 -O3")
set(CMAKE_C_FLAGS "-Wno-missing-field-initializers -Wunused-variable -Wno-sign-compare")
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
add_definitions(-DFINTEGER=int)

add_compile_options(-fopenmp -lpthread -lcrypto -lrt -lgfortran -ldl -lz -fPIC -rdynamic)
add_compile_options(-O2 -g -pipe -W -Wall -fPIC -march=native  -Wno-deprecated-declarations -Wno-unused-parameter -Wno-logical-not-parentheses)

include(${PROJECT_SOURCE_DIR}/cmake/gflags.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/glog.cmake)
#include(${PROJECT_SOURCE_DIR}/cmake/yael.cmake)
add_subdirectory(puck)
# Add block directories
add_subdirectory(demo)
add_subdirectory(tools)

#copy file
file(GLOB demoConfFile  "py_puck_api/py_puck_api.py")
file(COPY ${demoConfFile} DESTINATION ${PROJECT_SOURCE_DIR}/output/lib/)    
