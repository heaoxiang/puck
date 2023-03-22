Puck & Tinker 一种ANN检索技术，支撑构建超大规模高维度低时延高吞吐的检索系统。
开源库puck，提供纯净的ANN检索代码。
详细文档：https://ku.baidu-int.com/knowledge/HFVrC7hq1Q/pKzJfZczuc/nT0Tf16CUS/UNUJyiP9P3Qu3X

## Description
This project is a library for approximate nearest neighbor(ANN) search. It contains two algorithms, Puck and Tinker. This project is written in C++ with wrappers for python3.

Puck is an efficient approache for large-scale dataset, which has the best performance of multiple 1B-datasets in NeurIPS'21 competition track. It use PQ to compressed used memory during search stage. If the memory is going to be a bottleneck, Puck could resolve your problems.

Tinker is an efficient approache for smaller dataset(like 10M, 100M), which has better performance then Nmslib in big-ann-benchmarks. The relationships among similarity points are well thought out, Tinker need more memory to save these. Thinker cost more memory then Puck, but has better performace than Puck. If you want a better performance searching service and need not concerned about memory used, Tinker is a better choiese.

## Introduction
This project supports cosine similarity, L2(Euclidean) and IP(Inner Product, conditioned). When two vectors are normalized, L2 distance is equal to 2 - 2 * cos. IP2COS is a transform method that convert IP distance to cos distance. the distance value in search result is always L2.

Puck use a compressed vectors(after PQ) instead of the original vectors, the memory cost just over to 1/4 of the original vectors by default. With the increase of datasize, Puck's advantage is more obvious.

Tinker need save relationships of similarity points, the memory cost is more than the original vectors (less than Nmslib) by default. 

more details in benchmarks.

## install

The prerequisite is mkl.

1.Clone this project. 


cd puck

2.Set up the environment variable MKL_ROOT

MKL must be installed to compile puck, download the MKL installation package corresponding to the operating system from the official website, and configure the corresponding installation path after the installation is complete.

https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html#gs.qt4aoj

export MKL_ROOT = /opt/intel/oneapi/mkl/latest/

3.Use cmake to build this project.

mkdir build

cd build

cmake ..

make

## How to use
### format vector dataset
The vectors are stored in raw little endian. 
Each vector takes 4+d*4 bytes for .fvecs format, where d is the dimensionality of the vector.
this formated dataset is named puck_index/all_data.feat.bin by default.

### train & build 

cd output/build_tools

sh script/puck_train_control.sh -t -b 

### search
see demo/puck_client.cpp
see demo/tinker_client.cpp

## benchmark

