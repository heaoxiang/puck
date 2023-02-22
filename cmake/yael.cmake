# Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

INCLUDE(ExternalProject)

SET(YAEL_PREFIX_DIR  ${THIRD_PARTY_PATH}/yael)
SET(YAEL_INSTALL_DIR ${THIRD_PARTY_PATH}/install/yael)
SET(YAEL_INCLUDE_DIR ${YAEL_INSTALL_DIR} CACHE PATH "yael include directory." FORCE)
SET(YAEL_REPOSITORY ${GIT_URL}/nk2014yj/yael_v438.git)
SET(YAEL_TAG        v438)
SET(YAEL_CONFIGURE   sh configure.sh --msse4 --yael=${YAEL_INSTALL_DIR})  # 指定配置指令（注意此处修改了安装目录，否则默认情况下回安装到系统目录）

IF(WIN32)
  SET(YAEL_LIBRARIES "${YAEL_INSTALL_DIR}/lib/yael.lib" CACHE FILEPATH "yael library." FORCE)
  SET(YAEL_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4530")
  add_definitions("/DGOOGLE_YAEL_DLL_DECL=")
ELSE(WIN32)
  SET(YAEL_LIBRARIES "${YAEL_INSTALL_DIR}/libyael.a" CACHE FILEPATH "yael library." FORCE)
  SET(YAEL_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
ENDIF(WIN32)

SET(YAEL_INSTALL_COMMAND rm -rf ${YAEL_INSTALL_DIR} && mkdir -p ${YAEL_INSTALL_DIR}/lib && cp yael/libyael.a ${YAEL_LIBRARIES}  )

INCLUDE_DIRECTORIES(${YAEL_INCLUDE_DIR})
ExternalProject_Add(
    extern_yael
    ${EXTERNAL_PROJECT_LOG_ARGS}
    ${SHALLOW_CLONE}
    GIT_REPOSITORY  ${YAEL_REPOSITORY}
    GIT_TAG         ${YAEL_TAG}
    PREFIX          ${YAEL_PREFIX_DIR}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${YAEL_CONFIGURE}
    UPDATE_COMMAND  ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory yael ${YAEL_INSTALL_DIR}
    CMAKE_ARGS      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                    -DCMAKE_CXX_FLAGS=${YAEL_CMAKE_CXX_FLAGS}
                    -DCMAKE_CXX_FLAGS_RELEASE=${CMAKE_CXX_FLAGS_RELEASE}
                    -DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}
                    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
                    -DCMAKE_C_FLAGS_DEBUG=${CMAKE_C_FLAGS_DEBUG}
                    -DCMAKE_C_FLAGS_RELEASE=${CMAKE_C_FLAGS_RELEASE}
                    -DCMAKE_INSTALL_PREFIX=${YAEL_INSTALL_DIR}
                    -DCMAKE_INSTALL_LIBDIR=${YAEL_INSTALL_DIR}/lib
                    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                    -DWITH_GFLAGS=OFF
                    -DBUILD_TESTING=OFF
                    -DCMAKE_BUILD_TYPE=${THIRD_PARTY_BUILD_TYPE}
                    ${EXTERNAL_OPTIONAL_ARGS}
    CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${YAEL_INSTALL_DIR}
                     -DCMAKE_INSTALL_LIBDIR:PATH=${YAEL_INSTALL_DIR}/lib
                     -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
                     -DCMAKE_BUILD_TYPE:STRING=${THIRD_PARTY_BUILD_TYPE}
    BUILD_BYPRODUCTS ${YAEL_LIBRARIES}
)

ADD_LIBRARY(yael STATIC IMPORTED GLOBAL)
SET_PROPERTY(TARGET yael PROPERTY IMPORTED_LOCATION ${YAEL_LIBRARIES})
ADD_DEPENDENCIES(yael extern_yael)
LINK_LIBRARIES(yael)
file(
    GLOB YAEL_HEAD_FILES
    ${YAEL_PREFIX_DIR}/src/extern_yael/yael/*.h
)