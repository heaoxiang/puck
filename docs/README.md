## 项目背景（ANN）
基于卷积神经网络（Convolutional Neural Network）的全连接层的特征向量可以设计相应的检索系统：基于CNN特征，将query与库中所有样本进行相关性计算，输出最相关的结果集。
这类服务通常需要解决两个问题：特征选取以及对应的相关性算法。在使用CNN模型提取特征的时候，最终得到的特征维度较高，直接使用PCA等降维的手段，虽然能达到特征维度约减的目的，但在保持必要的检索精度前提下，能够降低的维度还是有限的，因而对于这一类高维度特征检索，有必要为它构建够高效合理的快速检索机制，使其适应海量数据的检索。

## 应用场景
基于特征向量的检索，通常具有三个典型的主要特征：样本数据量大、特征维度高以及要求响应时间短。                   
Puck&Tinker是一种近似最近邻(Approximate Nearest Neighbor)检索方法，通过将全空间分割成很多小的子空间，在搜索的时候，通过某种方式，快速锁定在某一（几）子空间，然后在该（几个）子空间里做遍历，从而达到次线性的计算复杂度。目前广泛应用于内容、搜索、推荐、人脸识别、网盘等等场景

## 关于Puck&Tinker

### Puck
名称来源：Puck源自经典MOBA游戏DOTA中的智力英雄，取其飘逸、灵动之意。  
ANN的检索性能是重中之重，PUCK设计并实现了多种优化方案，着重提升性能和效果，包括但不限于：
* 采用二层倒排索引架构，能更敏感的感知数据分布，从而非常高效的分割子空间，减少搜索范围；同时采用共享二级类聚中心的方式，大幅减少训练时间
* 训练时采用启发式迭代的方法，不断优化一二级类聚中心，通过等价空间变换，训练获得更好的数据分布描述
* 采用多层级量化加速查找，优先通过大尺度量化的小特征快速找到候选集，再通过稍大一些的量化特征二次查找
* 在各个检索环节打磨极致的剪枝， 针对loss函数，通过多种公式变化，最大程度减少在线检索计算量，缩短计算时间
* 严格的内存cacheline对齐和紧致排列，最大程度降低cache miss
* 支持大尺度的量化，单实例支持尽可能多的数据，针对大尺度量化定向优化，减少量化损失; 同时支持非均匀量化，更加适应各种纬度的特征


除了性能以外，Puck还做了很多功能拓展：
* 实时插入：支持无锁结构的实时插入，做到数据的实时更新
* 分布式建库：索引的构建过程支持分布式扩展，全量索引可以通过map-reduce一起建库，无需按分片build，大大加快和简化建库流程。  分布式建库工具
* 条件查询：支持检索过程中的条件查询，从底层索引检索过程中就过滤掉不符合要求的结果，解决多路召回归并经常遇到的截断问题，更好满足组合检索的要求
* 自适应参数：ANN方法检索参数众多，应用起来有不小门槛，不了解技术细节的用户并不容易找到最优参数，Puck提供参数自适应功能，在大部分情况下使用默认参数即可得到很好效果

### Tinker
名称来源：tinker同样源自经典MOBA游戏DOTA中的智力英雄
缘起：puck在大数据集上表现优异，但在千万级以下的小数据集且要求高召回率的场景下性能不如nmslib（benchmark），我们思索说如何能继续突破，使得puck在小数据集上一样能达到SOTA
方案：经过不断尝试，我们确认在puck当前检索结构上基本已经优化到了极致，不会再有质的提升；于是我们转而选择在检索算法上去做突破，经过多轮讨论、尝试和失败，终于提出了tinker算法，tinker的最终效果大大超出了我们最初预期，取得了惊人效果

## 比赛获奖情况
首届国际向量检索大赛BigANN是由人工智能领域全球顶级学术会议NeurIPS发起，由微软、facebook等公司协办，是全球最高水平的赛事，旨在提升大规模ANN的研究创新和生产环境中的落地应用。虽是首届大赛，但因NeurIPS的极高知名度和权威性，吸引了众多知名企业和顶尖大学的同台竞技。本届比赛已于2021年12月NeurlPS’21会议期间公布结果       
Puck在参赛的四个数据集中均排名第一
           比赛详情：https://big-ann-benchmarks.com/ 
           比赛结果：https://github.com/harsha-simhadri/big-ann-benchmarks/tree/main/t1_t2 
       
       
## Benchmark
性能对标结论：见benchmark


## Puck使用方法简介（训练、建库、检索）
### 1. 准备特征向量数据
<figure>

#### 1.1 特征文件

<figure>
特征文件是二进制文件（默认文件名字是puck_index/all_data.feat.bin）。特征向量的纬度是dim，每个向量的存储长度是：sizeof(int)+dim * sizeof(float)，存储格式如下：

| field  | field type   |  description |
| :------------: | :------------: | :------------: |
| d  |  int | the vector dimension  |
|  components | float * d  | the vector components  |
</figure>

#### 1.2 标签文件
<figure>
标签文件（puck_index/all_data.url）是明文存储。特征向量在特征文件的顺序（local id），与其标签在标签文件的顺序保持一直。在实时插入、分布式建库等功能中，必须绑定每个样本的标签（puck_index/all_data.url）。
</figure>

#### 1.3 数据格式化demo脚本
<figure>

可参考脚本：tools/script/puck_train_control.sh。这个demo的输入数据格式可参考tools/demo/init-feature-example，对特征向量长度检查&预处理（归一、IP2COS等）后，写特征文件（默认写puck_index/all_data.feat.bin）和标签文件（puck_index/all_data.url）。
```shell
## 使用方法
sh tools/script/puck_train_control.sh -i your_feature_file_name
```

</figure>

</figure>

### 2. 了解训练、建库和检索参数
<figure>

该代码库的训练、建库和检索参数均通过gflags的方式指定

#### 2.1训练&建库参数
<figure>

**与数据集相关的参数**

<figure>
    feature_dim：特征向量的纬度，需要由用户指定。
</figure>

**影响训练效果的参数**

<figure>

Tinker有着绝对的性能优势，内存使用上大于Puck, 小于Nmslib；如果你的应用不是内存瓶颈，重点在计算上，请大胆使用tinker；当在大规模数据集上，内存成为瓶颈时，Puck具备了更多优势，表现远优于Faiss-IVF和Faiss-IVFPQ，随着数据规模越大，Puck优势越明显。

<table style="font-size:12px">
   <tr> <!-- 第一行数据 -->
        <th style="text-align:center;font-size:18px" width="10%">算法 </th> 
        <th style="text-align:center" width="20%">内存 </th> 
        <th style="text-align:center" width="40%">检索性能 </th> 
        <th style="text-align:center" width="15%">备注</th> 
        <th style="text-align:center" width="15%">训练参数</th> 
    </tr>
    <tr> <!-- 第二行数据 -->
        <td >Puck（默认） </th> 
        <td >内存消耗最少，略高于样本总数 * dim * sizeof（char） </td> 
        <td rowspan="3"><b>1千万数据规模，召回率要求不高(<= 94%)时:</b>
        <p>
        Tinker > Puck-Flat > Puck > HNSW(nmslib)> Faiss-IVF &  Faiss-IVFPQ
        <p>
        <b>1千万数据规模，召回率要求高(> 94%)时:</b>
        <p>
        Tinker > HNSW(nmslib) > Puck-Flat > Puck > Faiss-IVF &  Faiss-IVFPQ
        <p>
        <b>1亿数据规模下，</b>
        <p>
        Tinker >  Puck-Flat > Puck > Faiss-IVF &  Faiss-IVFPQ（HNSW建库内存超限160G内存） </td> 
        <td>支持实时建库，适用于超大规模数据集</td> 
        <td>默认</td> 
    </tr>
    <tr> <!-- 第三行数据 -->
        <td> Puck-Flat </td> 
        <td> 内存消耗比Puck高，略高于样本总数 * dim * sizeof(float) </td>
        <td> 支持实时建库 </td>
        <td>--whether_pq=0</td> 
    </tr>
        <tr> <!-- 第Ⅳ行数据 -->
        <td> Tinker </td> 
        <td> 三种方法中消耗最高，但低于HNSW（nmslib） </td>
        <td> 适用于内存非瓶颈、对召回率要求较高的场景 </td>
        <td>--index_type=2</td> 
    </tr>
</table>

</figure>

**聚类中心**

<figure>
coarse_cluster_count、fine_cluster_count的选取与数据规模有关，推荐值如下：

| 数据规模  | coarse_cluster_count  |  fine_cluster_count |
| :------------: | :------------: | :------------: |
| <=500w  |  500 | 500  |
|  <=1kw | 1000  | 1000  |
| <=5kw  | 2000  | 2000  |
| <=10kw  | 3000  | 3000  |
|  >10kw |  5000 | 5000 |
</figure>

 其他参数使用默认值，通常可以达到较好的效果。检索参数极限调优，可联系hi群1672339
</figure>

#### 2.2 检索参数


检索过程可以分为2部分，每个过程对应的检索参数如下：
<figure>

##### 计算query与聚类中心的距离并排序：

<figure>
    
search_coarse_count：检索一级聚类中心（coarse_cluster_count）的个数，取值越大子空间的检索范围越大，需要<=coarse_cluster_count
    
</figure>

##### 计算query与top-M个聚类中心下样本的距离：

<figure>

Puck & Puck-Flat：

<figure>

neighbors_count：比对query与样本的个数。取值越大，检索范围越大，召回率越高，耗时增加，QPS下降。
    
</figure>

Tinker：

<figure>

tinker_search_range：候选队列长度。取值越大，检索范围越大，召回率越高，耗时增加，QPS下降。
    
</figure>
</figure>

</figure>

### 3. 训练
<figure>
当前支持本地训练。提供统一demo脚本（tools/script/puck_train_control.sh）。训练建库数据（puck_index/all_data.feat.bin）准备完成后，可直接执行 

```shell
#训练
sh tools/script/puck_train_control.sh -t
```

</figure>
### 4. 建库

### 4.1 建库
<figure>

```shell
#本地建库
sh tools/script/puck_train_control.sh -b
```
</figure>

### 5. 检索
<figure>
可参考demo文件demo/search_client.cpp。索引所长的目录可通过gflag修改（index_path），默认值是puck_index。

创建索引的实例后，通过init()方法加载索引，search检索最近的topk个样本，获得相似度（distance）和样本的local idx。

```cpp
std::unique_ptr<puck::Searcher> searcher(new puck::PuckIndex());
int ret = searcher->init();
if (ret != 0) {
    return -1;
}
//获取query的特征向量feature_data
...

puck::Request request;
puck::Response response;
request.topk = 100;
request.feature = feature_data.data();
ret = searcher->search(&request, &response);
if (ret != 0) {
    return -1;
}
for (size_t j = 0; j < response.result_num; j ++) {
    LOG(INFO)<<response.local_idx[j]<<"\t"<<response.distance[j];
}
```

</figure>
