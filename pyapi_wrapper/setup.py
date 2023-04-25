"""
Copyright (c) 2023 Baidu, Inc.  All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
"""
from setuptools import setup, find_packages
import os

long_description="""
This project is a library for approximate nearest neighbor(ANN) search. It contains two algorithms, Puck and Tinker. This project is written in C++ with wrappers for python3.

Puck is an efficient approache for large-scale dataset, which has the best performance of multiple 1B-datasets in NeurIPS'21 competition track. It use PQ to compressed used memory during search stage. If the memory is going to be a bottleneck, Puck could resolve your problems.

Tinker is an efficient approache for smaller dataset(like 10M, 100M), which has better performance then Nmslib in big-ann-benchmarks. The relationships among similarity points are well thought out, Tinker need more memory to save these. Thinker cost more memory then Puck, but has better performace than Puck. If you want a better performance searching service and need not concerned about memory used, Tinker is a better choiese.
"""

setup(
    name='puck',
    version='0.0.1',
    description='A library for efficient similarity search and clustering of dense vectors ',
    long_description=long_description,
    url='https://console.cloud.baidu-int.com/devops/icode/repos/baidu/mirana/puck/tree/stable',
    author='Huang,Ben Yin,Jie',
    author_email='huangben@baidu.com',
    license='Apache License',
    keywords='search nearest neighbors',
    packages=['puck'],
    package_data={
        'puck': ['*.so'],
    },
    py_modules=["py_puck_api"],

    classifiers=[
        "BigANN",
        "puck",
        "search nearest neighbors ",
    ],
    python_requires='>=3.6',
)
