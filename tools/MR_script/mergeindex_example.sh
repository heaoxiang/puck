#!/bin/bash
source ./config.propertie

function gnoimi_mergeindex()
{
    local _input="${1}"
    local _output="${2}"
    local _jobname="${3}"

    ${HADOOP_BIN} fs -test -e ${_output}
    if [ $? -eq 0 ];then
        ${HADOOP_BIN} fs -rmr ${_output}
    fi
   	echo ${MERGE_TASK_NUMBER} 
    ${HADOOP_BIN} streaming \
        -D mapred.job.priority="NORMAL" \
        -D mapred.job.name=${_jobname} \
        -D mapred.map.tasks=20 \
        -D mapred.reduce.tasks=${MERGE_TASK_NUMBER} \
        -D mapred.job.map.capacity=20 \
        -D mapred.job.reduce.capacity=${MERGE_TASK_NUMBER} \
        -D abaci.split.optimize.enable=false \
        -D abaci.split.remote=true \
        -partitioner "com.baidu.sos.mapred.lib.IntHashPartitioner" \
        -input "${_input}" \
        -output "${_output}" \
        -mapper "${SPLIT_BS_CMD}" \
        -reducer "${MERGE_CMD}" \
        -file "${SPLIT_BS_SCRIPT}" \
        -cacheArchive "${HADOOP_TOOL_LIB}/${TOOLS_LOCAL_LIB_NAME}.tar.gz"
}

_input=${RECALL_FEA_BUILD_OUTPUT}
_output=${RECALL_FEA_MERGE_OUTPUT}
_jobname=${JOB_TAG}"_merge"
echo ${_input} ${_output} ${_jobname}
gnoimi_mergeindex ${_input} ${_output} ${_jobname}
exit $?

