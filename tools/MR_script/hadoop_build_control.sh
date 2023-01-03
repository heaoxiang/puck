#!/bin/bash

function _usage(){
    usage_str="sh ${0}[-i build_path] [-p push puck_tools.tar.gz] [-b build] [-m merge] [-d download] [-h help]
    \n\t\t
    \n\toptions:
    \n\t\t-i codebooks train lib  init ${TOOLS_LOCAL_LIB_NAME}(index files & conf file)
    \n\t\t-p  push puck_tools.tar.gz
    \n\t\t-b  use for build method
    \n\t\t-m  use for merge method
    \n\t\t-d  download index
    \n\t\t-h  use for help" 
    echo -e ${usage_str}
}

if [ ! -n "$1" ] ;then
    _usage
    exit 1
fi

source ./config.propertie
INDEX_FILES_ARRAY=(`echo ${INDEX_FILES_LIST} | tr ',' ' '`)
INDEX_CODEBOOKS_ARRAY=(`echo ${INDEX_CODEBOOKS_LIST}| tr ',' ' '`)

is_prepare=0
puck_train_path=""
is_push_puck_tools=0

is_build=0
is_merge=0
is_download=0
while getopts "i:pbmdh" opt
do
    case $opt in
        i) 
            is_prepare=1
            puck_train_path=$OPTARG;;
        p) 
            is_push_puck_tools=1;;
        b) 
            is_build=1;;
        m) 
            is_merge=1;;
        d) 
            is_download=1;;
        h)
            _usage
            exit 0;;
        *)
            _usage
            exit 1;;
    esac
done

if [ ${is_prepare} -eq 1 ]; then
    #根据输入产生（codebooks的训练目录），初始化建库工具
    cd ${PROJECT_PATH}
    echo ${puck_train_path}
    rm -rf ${TOOLS_LOCAL_LIB_NAME} 
    mkdir -p ${TOOLS_LOCAL_LIB_NAME}/${INDEX_PATH_GFLAG}/
    mkdir -p ${TOOLS_LOCAL_LIB_NAME}/conf
    for file in ${INDEX_CODEBOOKS_ARRAY[@]}
    do 
        cp ${puck_train_path}/${INDEX_PATH_GFLAG}/${file}  ${TOOLS_LOCAL_LIB_NAME}/${INDEX_PATH_GFLAG}/${file}
    done
    cp -a ${puck_train_path}/conf ${TOOLS_LOCAL_LIB_NAME}
    cp -a ${PROJECT_PATH}/bin ${TOOLS_LOCAL_LIB_NAME} 
    hadoop_ugi_str=$(echo "--hadoop_ugi="${HADOOP_UGI})
    echo ${hadoop_ugi_str} >> ${TOOLS_LOCAL_LIB_NAME}/conf/${CONF_FILE_NAME}
fi

if [ ${is_push_puck_tools} -eq 1 ]; then
    cd ${PROJECT_PATH}
    rm -f ${TOOLS_LOCAL_LIB_NAME}.tar.gz
    tar zcvf ${TOOLS_LOCAL_LIB_NAME}.tar.gz ${TOOLS_LOCAL_LIB_NAME}/*
    
    ${HADOOP_BIN} fs -test -e ${HADOOP_TOOL_LIB}/${TOOLS_LOCAL_LIB_NAME}.tar.gz
    if [ $? -eq 0 ];then
        ${HADOOP_BIN} fs -rm ${HADOOP_TOOL_LIB}/${TOOLS_LOCAL_LIB_NAME}.tar.gz
    fi
    
    ${HADOOP_BIN} fs -put ${TOOLS_LOCAL_LIB_NAME}.tar.gz  ${HADOOP_TOOL_LIB}
    retcode=$? 
    [ $retcode -ne 0 ] && exit $retcode	
    echo "tar ${TOOLS_LOCAL_LIB_NAME} lib and put it to hadoop path ${HADOOP_TOOL_LIB} suc";
fi

if [ ${is_build} -eq 1 ]; then
    echo "\nstart build puck index"
    ./buildindex_example.sh    
    retcode=$? 
    [ $retcode -ne 0 ] && exit $retcode	
    echo "build puck index suc"
fi

if [ ${is_merge} -eq 1 ]; then
    echo "\nstart merge puck index"
    ./mergeindex_example.sh    
    retcode=$? 
    [ $retcode -ne 0 ] && exit $retcode	
    echo "merge puck index suc"
fi

function merge_index_file_by_order()
{
    local file_name="${1}"
    local _input="${2}"
    local _output="${3}"

    local _bs_id="${4}"
    local _shard_num="${5}"

    local shard_id=0
    local merge_file_list=""
    while(( ${shard_id} < ${_shard_num} ))
    do
        cur_shard_id=$((${shard_id} + ${_bs_id} * ${_shard_num}))
        echo "cur_shard_id="${cur_shard_id}
        # 1 => part-00001
        part=`printf "%05d" ${shard_id}`
        echo "TEST "${shard_id}" "${cur_shard_id}" "${part}
        input_file="${_input}/${cur_shard_id}/${INDEX_PATH_GFLAG}/${file_name}"
        echo "INIT FILE_NAME: "${input_file}
        ${HADOOP_BIN} fs -test -e "${input_file}"
        if [ $? -ne 0 ];then
            break
        fi
        merge_file_list=`echo ${merge_file_list} ${input_file}`
        #continue
        #target_file="${_output}/${INDEX_PATH_GFLAG}/${file_name}_${part}"
        #echo "TARGET FILE NAME: "${target_file}
        #
        ##${HADOOP_BIN} fs -mv ${input_file} ${target_file}
        #${HADOOP_BIN} fs -cp ${input_file} ${target_file}
        #if [ $? -ne 0 ]; then
        #    log_error "cp [${input_file}] to [${target_file}] fail!!!"
        #    exit 1
        #fi

        let "shard_id++"
    done
    echo "${HADOOP_BIN} fs -cat ${merge_file_list} | ${HADOOP_BIN} fs -put - ${_output}/${INDEX_PATH_GFLAG}/${file_name}"
    ${HADOOP_BIN} fs -cat ${merge_file_list} | ${HADOOP_BIN} fs -put - ${_output}/${INDEX_PATH_GFLAG}/${file_name}
    return ${?}
}

# 通过cat方式合并索引
function download_index_to_local()
{
    local _cur_bs_id=${1}
    local hadoop_bs_lib=${2}
    
    cd ${DOWNLOAD_INDEX_LIB}
    rm -rf ${_cur_bs_id} && mkdir ${_cur_bs_id} && cd ${_cur_bs_id}
    ${HADOOP_BIN} fs -get ${hadoop_bs_lib}/${INDEX_PATH_GFLAG} .
    for file in ${INDEX_CODEBOOKS_ARRAY[@]}
    do 
        cp ${TOOLS_LOCAL_LIB_NAME}/${INDEX_PATH_GFLAG}/${file} ${INDEX_PATH_GFLAG} 
    done
}

if [ ${is_download} -eq 1 ]; then
    _bs_num=${BS_NUMBER}
    echo "_bs_num" $_bs_num    
    #每个bs下的分片个数
    _shard_num=$((${MERGE_TASK_NUMBER}/${BS_NUMBER}))
	echo $_shard_num    
    echo "start download puck index from hadoop to local, total bs number=${_bs_num}"
    #-m后的输出目录
    merge_output_lib="${RECALL_FEA_MERGE_OUTPUT}"
    #对不同shard 下的相同bs，标记顺序
    reorder_temp_lib="${RECALL_FEA_MERGE_OUTPUT_TEMP}"
    
    if [ -d ${RECALL_FEA_MERGE_OUTPUT_TEMP} ]; then
        rm -rf ${RECALL_FEA_MERGE_OUTPUT_TEMP}
    fi
    #本地下载目录 
    echo ${DOWNLOAD_INDEX_LIB}
    if [ -d ${DOWNLOAD_INDEX_LIB} ]; then
        rm -rf ${DOWNLOAD_INDEX_LIB}
    fi
    mkdir ${DOWNLOAD_INDEX_LIB}

    bs_id=0
    while(( ${bs_id} < ${_bs_num} ))
    do
        cd ${PROJECT_PATH}
        #移动后的输出目录
        merge_bs_temp_lib="${reorder_temp_lib}/${bs_id}"
        ${HADOOP_BIN} fs -test -e "${merge_bs_temp_lib}"
        if [ $? -eq 0 ];then
            ${HADOOP_BIN} fs -rmr ${merge_bs_temp_lib}
        fi
        ${HADOOP_BIN} fs -mkdir "${merge_bs_temp_lib}/${INDEX_PATH_GFLAG}"

        for file in ${INDEX_FILES_ARRAY[@]}
        do
            merge_index_file_by_order "${file}" "${merge_output_lib}" "${merge_bs_temp_lib}" "${bs_id}" "${_shard_num}"
        done
        download_index_to_local "${bs_id}" "${merge_bs_temp_lib}"
        retcode=$? 
    	[ $retcode -ne 0 ] && exit $retcode	
    	echo "download puck index (bs id = ${k}) suc"
        let "bs_id++"
    done
fi

exit $?



