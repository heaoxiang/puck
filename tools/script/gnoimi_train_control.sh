#!/bin/bash


function _usage(){
    usage_str="sh ${0} [-i init] [-b build] [-t train] [-h help]
    \n\t\t
    \n\toptions:
    \n\t\t-i use for init method and init file name is necessary 
    \n\t\t-b use for build method
    \n\t\t-t use for train method
    \n\t\t-h use for help" 
    echo -e ${usage_str}
}

if [ ! -n "$1" ] ;then
    _usage
    exit 1
fi

is_train=0
is_build=0
is_init=0
init_file=""
while getopts "i:tbh" opt
do
    case $opt in
        t) 
            is_train=1;;
        b) 
            is_build=1;;
        i) 
            is_init=1
            init_file=$OPTARG;;
        h)
            _usage
            exit 0;;
        *)
            _usage
            exit 1;;
    esac
done

if [ $is_init -eq 1 ]; then
    echo "init feature file :" $init_file
    #all_data_url="./puck_index/all_data.url"
    python ./script/initProcessData.py -f ${init_file}
    retcode=$?
    if [ $retcode == 1 ]; then
        errmsg="input param error!"
    elif [ $retcode == 2 ]; then
        errmsg="input file is not exists!"
    elif [ $retcode == 3 ]; then
        errmsg="init feature file error!"
    elif [ $retcode == 0 ]; then
        errmsg="init feature file succeed!"
    fi
    echo `date`  $errmsg
    [ $retcode -ne 0 ] && exit $retcode
fi

curr_path=`pwd`
lib_path=$curr_path/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lib_path

if [ $is_train -eq 1 ]; then
    echo "start train"
    ./bin/train --flagfile=conf/puck_train.conf 
    retcode=$?
    echo "train retcode : " $retcode
    [ $retcode -ne 0 ] && exit $retcode
    echo "train succ"
    #cp gnoimi_index/filer_codebook.dat mid-data/filer_codebook.dat
fi

if [ $is_build -eq 1 ]; then
    echo "start build gnoimi"
    ./bin/build --flagfile=conf/puck_train.conf 
    retcode=$?
    echo "build gnoimi retcode : " $retcode
    [ $retcode -ne 0 ] && exit $retcode
    echo "build gnoimi succ"
fi

