#!/bin/bash
#########################################################################
# Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
# @file	swig_run.sh
# @author	yinjie06(yinjie06@baidu.com)
# @date	2021-08-18 14:52
# @brief	
#########################################################################

#!/bin/bash
export SWIG_LIB='../../../baidu/third-party/swig/Lib'
mkdir py_puck_api
../../../baidu/third-party/swig/swig -c++ -python -outdir py_puck_api -o ./py_puck_api/py_puck_api.cpp ./pyapi_wrapper/py_puck_api.i
