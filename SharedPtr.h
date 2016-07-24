#ifndef SHAREDPTR_NONATOMIC_H
#define SHAREDPTR_NONATOMIC_H

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <new>

namespace nonatomic {

template<typename T>
class SharedPtr;

template<typename T>
class WeakPtr;

template<typename T>
class EnableSharedFromThis;

template<typename T>
void _enableSharedFromThis(EnableSharedFromThis<T>* that, void* data);
template<typename T>
void _enableSharedFromThis(...) { }

template<typename T>
class SharedPtrPool;

template<typename T>
class SharedPtrPoolScope
{
public:
    SharedPtrPoolScope(const SharedPtrPoolScope<T>& copy);
    ~SharedPtrPoolScope();

    SharedPtrPoolScope<T>& operator=(const SharedPtrPoolScope<T>& copy);

    void* mem(uint32_t offset);
    uint32_t useCount() const { return pool->count; }

private:
    SharedPtrPoolScope(SharedPtrPool<T>* p)
        : pool(p)
    {
        ++pool->count;
    }

    SharedPtrPool<T>* pool;

    friend class SharedPtr<T>;
};

template<typename T>
class SharedPtr
{
public:
    SharedPtr();
    SharedPtr(T* ptr);

    SharedPtr(const SharedPtrPoolScope<T>& scope, uint32_t poolOffset);
    SharedPtr(const SharedPtrPoolScope<T>& scope, uint32_t poolOffset, T* ptr);

    SharedPtr(const SharedPtr<T>& copy);
    ~SharedPtr();

    SharedPtr<T>& operator=(const SharedPtr<T>& copy);

    void reset(T* ptr = 0);
    void reset(const SharedPtrPoolScope<T>& scope, uint32_t poolOffset, T* ptr = 0);

    T* operator->() { return data->ptr; }
    const T* operator->() const { return data->ptr; }
    T& operator*() { return *data->ptr; }
    const T& operator*() const { return *data->ptr; }

    operator bool() const { return data->ptr != 0; }

    static SharedPtrPoolScope<T> makePool(uint32_t num);

private:
    class Data
    {
    public:
        Data() : sharedCount(0), weakCount(0), ptr(0) { }
        Data(uint32_t sc, uint32_t wc, T* p, SharedPtrPool<T>* pl = 0, uint32_t poff = 0)
            : sharedCount(sc), weakCount(wc), ptr(p), pool(pl), poolOffset(poff)
        {
        }

        uint32_t sharedCount;
        uint32_t weakCount;
        T* ptr;

        SharedPtrPool<T>* pool;
        uint32_t poolOffset;

        static void reset(Data* data);
    };

    SharedPtr(Data* data);

    void clear();

    Data* data;

    friend class WeakPtr<T>;
    friend class EnableSharedFromThis<T>;
    friend class SharedPtrPool<T>;
    friend void _enableSharedFromThis<T>(EnableSharedFromThis<T>* that, void* data);
};

template<typename T>
class SharedPtrPool
{
public:
    ~SharedPtrPool();

private:
    SharedPtrPool(uint32_t sz);

    void* metaMem(uint32_t pos)
    {
        return reinterpret_cast<char*>(data) + ((sizeof(typename SharedPtr<T>::Data) + sizeof(T)) * pos);
    }
    void* mem(uint32_t pos)
    {
        return reinterpret_cast<char*>(data) + ((sizeof(typename SharedPtr<T>::Data) + sizeof(T)) * pos) + sizeof(typename SharedPtr<T>::Data);
    }

    void* data;
    uint32_t count;

    friend class SharedPtr<T>;
    friend class SharedPtrPoolScope<T>;
};

template<typename T>
SharedPtrPoolScope<T>::~SharedPtrPoolScope()
{
    assert(pool->count > 0);
    if (!--pool->count) {
        delete pool;
    }
}

template<typename T>
SharedPtrPoolScope<T>::SharedPtrPoolScope(const SharedPtrPoolScope<T>& copy)
    : pool(copy.pool)
{
    ++pool->count;
}

template<typename T>
SharedPtrPoolScope<T>& SharedPtrPoolScope<T>::operator=(const SharedPtrPoolScope<T>& copy)
{
    assert(pool->count > 0);
    if (!--pool->count)
        delete pool;
    pool = copy.pool;
    ++pool->count;
    return *this;
}


template<typename T>
class WeakPtr
{
public:
    WeakPtr();
    WeakPtr(const SharedPtr<T>& shared);
    WeakPtr(const WeakPtr<T>& copy);
    ~WeakPtr();

    WeakPtr<T>& operator=(const SharedPtr<T>& copy);
    WeakPtr<T>& operator=(const WeakPtr<T>& copy);

    bool expired() const;
    SharedPtr<T> lock() const;

private:
    void clear();

    void assign(typename SharedPtr<T>::Data* data);

    typename SharedPtr<T>::Data* data;

    friend class EnableSharedFromThis<T>;
};

template<typename T>
void _enableSharedFromThis(EnableSharedFromThis<T>* that, void* data)
{
    that->assign(static_cast<typename SharedPtr<T>::Data*>(data));
}

template<typename T>
void SharedPtr<T>::Data::reset(Data* data)
{
    if (!data->pool) {
        delete data;
    } else {
        SharedPtrPool<T>* pool = data->pool;
        data->~Data();
        assert(pool->count > 0);
        if (!--pool->count)
            delete pool;
    }
}

template<typename T>
inline SharedPtr<T>::SharedPtr()
    : data(new Data(1, 0, 0))
{
}

template<typename T>
inline SharedPtr<T>::SharedPtr(const SharedPtrPoolScope<T>& scope, uint32_t poolOffset)
{
    SharedPtrPool<T>* pool = scope.pool;
    data = new(pool->metaMem(poolOffset)) Data(1, 0, 0, poolOffset, pool);
    if (pool)
        ++pool->count;
}

template<typename T>
SharedPtr<T>::SharedPtr(T* ptr)
    : data(new Data(1, 0, ptr))
{
    _enableSharedFromThis<T>(ptr, data);
}

template<typename T>
SharedPtr<T>::SharedPtr(const SharedPtrPoolScope<T>& scope, uint32_t poolOffset, T* ptr)
{
    SharedPtrPool<T>* pool = scope.pool;
    data = new(pool->metaMem(poolOffset)) Data(1, 0, ptr, pool, poolOffset);
    if (pool)
        ++pool->count;

    _enableSharedFromThis<T>(ptr, data);
}

template<typename T>
inline SharedPtr<T>::SharedPtr(const SharedPtr<T>& copy)
    : data(copy.data)
{
    ++data->sharedCount;
}

template<typename T>
inline SharedPtr<T>::SharedPtr(Data* data)
    : data(data)
{
    ++data->sharedCount;
}

template<typename T>
inline SharedPtr<T>::~SharedPtr()
{
    clear();
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<T>& copy)
{
    clear();
    data = copy.data;
    ++data->sharedCount;
    return *this;
}

template<typename T>
inline void SharedPtr<T>::clear()
{
    Data* d = data;
    data = 0;

    assert(d->sharedCount > 0);
    if (d->sharedCount == 1) {
        T* ptr = d->ptr;
        d->ptr = 0;
        if (ptr) {
            if (d->pool)
                ptr->~T();
            else
                delete ptr;
        }
        if (!d->weakCount) {
            Data::reset(d);
        } else {
            --d->sharedCount;
        }
    } else {
        --d->sharedCount;
    }
}

template<typename T>
inline void SharedPtr<T>::reset(T* ptr)
{
    clear();
    data = new Data(1, 0, ptr);
    _enableSharedFromThis<T>(ptr, data);
}

template<typename T>
inline void SharedPtr<T>::reset(const SharedPtrPoolScope<T>& scope, uint32_t poolOffset, T* ptr)
{
    clear();
    SharedPtrPool<T>* pool = scope.pool;
    data = new(pool->metaMem(poolOffset)) Data(1, 0, ptr, pool, poolOffset);
    if (pool)
        ++pool->count;
    _enableSharedFromThis<T>(ptr, data);
}

template<typename T>
inline SharedPtrPoolScope<T> SharedPtr<T>::makePool(uint32_t num)
{
    SharedPtrPool<T>* pool = new SharedPtrPool<T>(num);
    return SharedPtrPoolScope<T>(pool);
}

template<typename T>
inline WeakPtr<T>::WeakPtr()
    : data(0)
{
}

template<typename T>
inline WeakPtr<T>::WeakPtr(const SharedPtr<T>& shared)
    : data(shared.data)
{
    ++data->weakCount;
}

template<typename T>
inline WeakPtr<T>::WeakPtr(const WeakPtr<T>& weak)
    : data(weak.data)
{
    if (data)
        ++data->weakCount;
}

template<typename T>
inline WeakPtr<T>::~WeakPtr()
{
    clear();
}

template<typename T>
inline void WeakPtr<T>::assign(typename SharedPtr<T>::Data* d)
{
    clear();
    data = d;
    ++data->weakCount;
}

template<typename T>
inline void WeakPtr<T>::clear()
{
    if (data) {
        typename SharedPtr<T>::Data* d = data;
        data = 0;
        if (!d->sharedCount) {
            assert(!d->ptr);
            assert(d->weakCount > 0);
            if (!--d->weakCount) {
                SharedPtr<T>::Data::reset(d);
            }
        } else {
            assert(d->weakCount > 0);
            --d->weakCount;
        }
    }
}

template<typename T>
inline WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<T>& copy)
{
    clear();
    data = copy.data;
    ++data->weakCount;
    return *this;
}

template<typename T>
inline WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<T>& copy)
{
    clear();
    data = copy.data;
    if (data)
        ++data->weakCount;
    return *this;
}

template<typename T>
inline bool WeakPtr<T>::expired() const
{
    return !data || !data->ptr;
}

template<typename T>
inline SharedPtr<T> WeakPtr<T>::lock() const
{
    if (data && data->ptr) {
        assert(data->sharedCount > 0);
        return SharedPtr<T>(data);
    }
    return SharedPtr<T>();
}

template<typename T>
class EnableSharedFromThis
{
public:
    SharedPtr<T> sharedFromThis();
    SharedPtr<const T> sharedFromThis() const;

private:
    void assign(typename SharedPtr<T>::Data* data);

    WeakPtr<T> _sharedFromThis;

    friend void _enableSharedFromThis<T>(EnableSharedFromThis<T>* that, void* data);
};

template<typename T>
inline SharedPtr<T> EnableSharedFromThis<T>::sharedFromThis()
{
    return _sharedFromThis.lock();
}

template<typename T>
inline SharedPtr<const T> EnableSharedFromThis<T>::sharedFromThis() const
{
    return _sharedFromThis.lock();
}

template<typename T>
inline void EnableSharedFromThis<T>::assign(typename SharedPtr<T>::Data* data)
{
    _sharedFromThis.assign(data);
}

template<typename T>
SharedPtrPool<T>::SharedPtrPool(uint32_t sz)
    : count(0)
{
    data = malloc((sizeof(typename SharedPtr<T>::Data) + sizeof(T)) * sz);
}

template<typename T>
SharedPtrPool<T>::~SharedPtrPool()
{
    free(data);
}

template<typename T>
inline void* SharedPtrPoolScope<T>::mem(uint32_t offset)
{
    return pool->mem(offset);
}

} // namespace nonatomic

#endif
