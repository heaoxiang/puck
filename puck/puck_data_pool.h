/***************************************************************************
 *
 * Copyright (c) 2017 Baidu.com, Inc. All Rights Reserved *
 **************************************************************************/ /**
 * @file   puck_data_pool.h
 * @author huangben(huangben@baidu.com)
 * @date   2017年08月13日 星期一 19时50分32秒
 * @brief
 *
 **/
#pragma once

#include "puck/simple_data_pool.h"

namespace puck {

template<typename T>
struct ClassFactory: public DataFactory {
    void* CreateData() const {
        return new T();
    }
    void DestroyData(void* ptr) const {
        T* data = static_cast<T*>(ptr);
        delete data;
    }
};

//前向声明
template<typename T>
struct DataHandler;

/** DataHandlerPool 扩展了SimpleDataPool, 从而提供一个更加易用的接口.
 *  提供borrow return reserve stat等api
 */
template<typename T, typename F = ClassFactory<T> >
class DataHandlerPool {
public:
    friend struct DataHandler<T>;
    DataHandlerPool() : _pool(&_factory) { }
    T* Borrow() {
        return static_cast<T*>(_pool.Borrow());
    }
    void Return(T* ptr) {
        _pool.Return(ptr);
    }
    void Reserve(unsigned n) {
        _pool.Reserve(n);
    }
    SimpleDataPool::Stat stat() const {
        return _pool.stat();
    }

private:
    F _factory;
    SimpleDataPool _pool;
private:
    SimpleDataPool* Pool() {
        return &_pool;
    }
};

/** DataHandler作为T的warpper，起到RAII的作用,析构时自动归还pool
 */
template<typename T>
struct DataHandler {
    //DataHandler(T* p, SimpleDataPool* pool) : ptr(p), owner(pool) { }
    DataHandler(DataHandlerPool<T>& pool) : ptr(pool.Borrow()), owner(pool.Pool()) { }
    ~DataHandler() {
        owner->Return(ptr);
    }
    T* operator->() const {
        return ptr;
    }
    T& operator*() const {
        return *ptr;
    }
    T* get() const {
        return ptr;
    }
    // 提供一个接口访问内部数据. 在一般情况中不会用到
    void swap(T*& rhs) {
        std::swap(ptr, rhs);
    }

private:
    T* ptr;
    SimpleDataPool* owner;
};

}
