# 项目名称

Puck是一种ANN检索技术，支撑构建超大规模高维度低时延高吞吐的检索系统。

wiki 地址：http://wiki.baidu.com/pages/viewpage.action?pageId=717321453

------
# 项目背景

基于卷积神经网络（Convolutional Neural Network）的全连接层的特征向量可以设计相应的检索系统：基于CNN特征，将query与库中所有样本进行相关性计算，输出最相关的TOP-K个结果。
<br />这类服务通常需要解决两个问题：特征选取以及对应的相关性算法。在使用CNN模型提取特征的时候，最终得到的特征维度较高，直接使用PCA等降维的手段，虽然能达到特征维度约减的目的，但在保持必要的检索精度前提下，能够降低的维度还是有限的，因而对于这一类高维度特征检索，有必要为它构建够高效合理的快速检索机制，使其适应海量数据的检索。

------
# 应用场景

基于特征向量的检索，通常具有三个典型的主要特征：样本数据量大、特征维度高以及要求响应时间短。GNOIMI是一种近似最近邻(Approximate Nearest Neighbor)检索方法，通过将全空间分割成很多小的子空间，在搜索的时候，通过某种方式，快速锁定在某一（几）子空间，然后在该（几个）子空间里做遍历，从而达到次线性的计算复杂度。

------
## 快速开始

1. 登录icode克隆最新代码：http://icode.baidu.com/repos/baidu/mms-graph/gnoimi/tree/master
2. 进入gnoimi根目录后，根据训练建库的配置参数修改src/gflags/gnoimi_gflags.cpp中的相应的项。
3. 执行bcloud build
4. 编译结束后，检索相关的demo程序在output/bin目录下(gnoimi_client)
5. 将训练建库的产出文件夹gnoimi_index拷贝至output。
5. ./gnoimi_client query_file_path recall_doc_file_path

------

## 测试
------

## 如何贡献

提交issue前请确认你符合如下要求：

- 请先给PMC委员会gnoimi-pmc@baidu.com 发送邮件描述背景和预计方案，PMC发起讨论和决策
- 根据讨论结果实施方案，CR由PMC委员会一致通过方能ci到主干
- 符合[baidu C++代码规范](http://wiki.baidu.com/pages/viewpage.action?pageId=333009429)
- 代码出现的位置和其定位相符。
- 有对应的单测代码，不降低召回率。

------

## 维护者

Owner : Puck-PMC
成员：haungben@baidu.com
     yinjie06@baidu.com
     chenzaiping@baidu.com

----------

## 用户讨论群

Hi群（Puck HELP）：1672339

------

# GNOIMI训练、建库和检索算法简介

### GNOIMI训练

**Step 1.Gnoimi训练样本随机抽样**

用户保证原始特征数据无重复，从原始数据中随机抽样。抽样数据集个数小于等于500w。

<div align="center">
<img src="http://latex.codecogs.com/gif.latex?P=\{P_{1},...,P_{N} \},P_i\epsilon R^D"/>
</div>
<br />

**Step 2.初始化聚类中心**

对抽样样本进行KMEANS聚类，得到初始的一级聚类中心：


<div align="center">
<img src="http://latex.codecogs.com/gif.latex?S=\{S_1,...,S_k\},S_i \epsilon R^D" />
</div>

<br />
计算抽样样本与其所属一级聚类中心的残差，对残差进行聚类得到初始的二级聚类中心:


<div align="center">
<img src="http://latex.codecogs.com/gif.latex?T=\{T_1,...,T_k\},T_i \epsilon R^D" />
</div>

<br />
alpha参数矩阵所有元素初始值均为1。


<br />空间分割如图所示，所有一级聚类中心共享二级聚类中心。

<img src="http://wiki.baidu.com/download/attachments/604931951/MacHi%202018-11-12%2011-48-17.png?version=1&modificationDate=1541994510000&api=v2" width = "400" height = "350" div align=center />
<br />

**Step 3.迭代训练聚类中心**

计算每个抽样样本所属的一级和二级聚类中心，并根据下面公式迭代更新一二级聚类中心向量和参数矩阵:

<div align="center">
<img src="http://latex.codecogs.com/gif.latex?\sum_{i:k_i=k,l_i=l}^{ }\left\|p_i-\left(S_k+\alpha\left[k,l \right]T_l \right) \right\|^{2}\rightarrow min" />
</div>
<br />
训练结束空间分割更符合真实数据分布:
<br />
<img src="http://wiki.baidu.com/download/attachments/604931951/MacHi%202018-11-12%2011-47-45.png?version=1&modificationDate=1541994584000&api=v2" width = "400" height = "350" div align=center />
<br />

**Step 4.PQ训练样本随机抽样**

计算抽样数据集中样本所属于的一二级距离中心，得到样本与一二级聚类中心的残差数据集。抽样数据集个数小于等于100w，抽样条目不重复。

**Step 5.PQ矢量量化训练**

将残差数据集分为nsq个空间，每个子空间特征维度为feature_dim/nsq，每个子空间分别进行KMEANS聚类，得到256个聚类中心(一个char占8bit，可用一个char字长标记所有的聚类中心ID)，得到每个子空间的码本。将nsq个子空间的子码本做笛卡尔积，得到整个数据集的PQ码本。

### GNOIMI建库

**Step 1.GNOIMI建库**

计算原始特征向量数据集中样本所属的一二级聚类中心，计算过程使用多线程。

**Step 2.PQ向量压缩**

计算原始特征向量数据集中样本与其所属的一二级聚类中心的残差。将残差向量分为nsq个子空间，在每个子空间内，计算该子特征向量距离最近的聚类中心并记录聚类中心ID，将feature_dim维度的特征向量映射为nsq个聚类中心的ID，可用nsq个聚类中心ID标识该特征向量。通常取nsq = feature_dim进行四分之一量化，feature_dim * sizeof(float) -> nsq *sizeof(char)。
<br />在检索过程中，将query与该样本在每个子空间内的距离，转化为与该样本距离最近的聚类中心的距离。因而，在检索过程中，无需加载原始特征向量，可降低检索过程中所需要的内存。

### GNOIMI在线检索

**检索过程**

1.当whether_norm=1时，特征进行归一化。
<br />2.计算query与一级聚类中心的距离并排序。
<br />3.计算query与前gnoimi_search_cells个一级聚类中心下的二级聚类中心的距离并排序，共计gnoimi_search_cells * gnoimi_cells_count个二级聚类中心。
<br />4.以优先队列的方式，从最近的二级聚类中心开始，依次取出其下的样本，并计算query与这些样本的距离，取满neighbors_count个为止。
<br />5.排序后返回topK个样本的contsign和对应的距离。

