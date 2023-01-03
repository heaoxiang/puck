#!/bin/bash 

source ./config.propertie

function puck_buildindex()
{
    if [ $# -ne 3 ]; then
        error "puck_buildindex param error"
        exit -1
    fi

    local _input="${1}"
    local _output="${2}"
    local _jobname="${3}"

    ${HADOOP_BIN} fs -test -e ${_output}
    if [ $? -eq 0 ];then
        ${HADOOP_BIN} fs -rmr ${_output}
    fi

    $HADOOP_BIN ustreaming \
        -D mapred.job.priority="NORMAL" \
        -D mapred.job.name=${_jobname} \
        -D mapred.map.tasks=10000 \
        -D mapred.reduce.tasks=1000 \
        -D mapred.job.reduce.capacity=1000 \
        -D mapred.job.map.capacity=10000 \
        -D abaci.split.optimize.enable=false \
        -D stream.memory.limit=1500 \
        -input "${_input}" \
        -output "${_output}" \
        -reducer "${BUILD_CMD}" \
        -cacheArchive "${HADOOP_TOOL_LIB}/${TOOLS_LOCAL_LIB_NAME}.tar.gz" \
        -mapper "cat"
}

_input=${RECALL_FEA_LIB}
_output=${RECALL_FEA_BUILD_OUTPUT}
_jobname=${JOB_TAG}"_build"
echo ${_input}  ${_output}  ${_jobname}
puck_buildindex ${_input}  ${_output}  ${_jobname}

exit $?

