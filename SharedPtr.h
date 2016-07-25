#ifndef SHAREDPTR_NONATOMIC_H
#define SHAREDPTR_NONATOMIC_H

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <new>

namespace nonatomic {

template<typename T>
class shared_ptr;

template<typename T>
class weak_ptr;

template<typename T>
class enable_shared_from_this;

template<typename T>
void _enable_shared_from_this(enable_shared_from_this<T>* that, void* data);
template<typename T>
void _enable_shared_from_this(...) { }

template<typename T>
class shared_ptr_pool;

template<typename T>
class shared_ptr_pool_scope
{
public:
    shared_ptr_pool_scope(const shared_ptr_pool_scope<T>& copy);
    ~shared_ptr_pool_scope();

    shared_ptr_pool_scope<T>& operator=(const shared_ptr_pool_scope<T>& copy);

    void* mem(uint32_t offset) const;
    uint32_t use_count() const { return pool->count; }

private:
    shared_ptr_pool_scope(shared_ptr_pool<T>* p)
        : pool(p)
    {
        ++pool->count;
    }

    shared_ptr_pool<T>* pool;

    friend class shared_ptr<T>;
};

template<typename T>
class shared_ptr
{
public:
    shared_ptr();
    shared_ptr(T* ptr);

    shared_ptr(const shared_ptr_pool_scope<T>& scope, uint32_t poolOffset);
    shared_ptr(const shared_ptr_pool_scope<T>& scope, uint32_t poolOffset, T* ptr);

    shared_ptr(const shared_ptr<T>& copy);
    ~shared_ptr();

    shared_ptr<T>& operator=(const shared_ptr<T>& copy);

    void reset(T* ptr = 0);
    void reset(const shared_ptr_pool_scope<T>& scope, uint32_t poolOffset, T* ptr = 0);

    T* get() { return data->ptr; }
    const T* get() const { return data->ptr; }

    //T* take() { T* ptr = 0; if (data->sharedCount == 1) { ptr = data->ptr; data->ptr = 0; reset(); } return ptr; }

    T* operator->() { return data->ptr; }
    const T* operator->() const { return data->ptr; }
    T& operator*() { return *data->ptr; }
    const T& operator*() const { return *data->ptr; }

    operator bool() const { return data->ptr != 0; }
    bool operator!() const { return !data->ptr; }

    uint32_t use_count() const { return data->sharedCount; }

    static shared_ptr_pool_scope<T> make_pool(uint32_t num);
    static shared_ptr<T> allocate(const shared_ptr_pool_scope<T>& scope, uint32_t offset);

private:
    class Data
    {
    public:
        Data() : sharedCount(0), weakCount(0), ptr(0) { }
        Data(uint32_t sc, uint32_t wc, T* p, shared_ptr_pool<T>* pl = 0, uint32_t poff = 0)
            : sharedCount(sc), weakCount(wc), ptr(p), pool(pl), poolOffset(poff)
        {
        }

        uint32_t sharedCount;
        uint32_t weakCount;
        T* ptr;

        shared_ptr_pool<T>* pool;
        uint32_t poolOffset;

        static void reset(Data* data);
    };

    shared_ptr(Data* data);

    void clear();

    Data* data;

    friend class weak_ptr<T>;
    friend class enable_shared_from_this<T>;
    friend class shared_ptr_pool<T>;
    friend void _enable_shared_from_this<T>(enable_shared_from_this<T>* that, void* data);
};

template<typename T>
class weak_ptr
{
public:
    weak_ptr();
    weak_ptr(const shared_ptr<T>& shared);
    weak_ptr(const weak_ptr<T>& copy);
    ~weak_ptr();

    weak_ptr<T>& operator=(const shared_ptr<T>& copy);
    weak_ptr<T>& operator=(const weak_ptr<T>& copy);

    bool expired() const;
    shared_ptr<T> lock() const;

    uint32_t use_count() const { return data ? data->sharedCount : 0; }

private:
    void clear();

    void assign(typename shared_ptr<T>::Data* data);

    typename shared_ptr<T>::Data* data;

    friend class enable_shared_from_this<T>;
};

template<typename T>
void _enable_shared_from_this(enable_shared_from_this<T>* that, void* data)
{
    that->assign(static_cast<typename shared_ptr<T>::Data*>(data));
}

template<typename T>
void shared_ptr<T>::Data::reset(Data* data)
{
    if (!data->pool) {
        delete data;
    } else {
        shared_ptr_pool<T>* pool = data->pool;
        data->~Data();
        assert(pool->count > 0);
        if (!--pool->count)
            delete pool;
    }
}

template<typename T>
inline shared_ptr<T>::shared_ptr()
    : data(new Data(1, 0, 0))
{
}

template<typename T>
inline shared_ptr<T>::shared_ptr(const shared_ptr_pool_scope<T>& scope, uint32_t poolOffset)
{
    shared_ptr_pool<T>* pool = scope.pool;
    data = new(pool->metaMem(poolOffset)) Data(1, 0, 0, poolOffset, pool);
    if (pool)
        ++pool->count;
}

template<typename T>
shared_ptr<T>::shared_ptr(T* ptr)
    : data(new Data(1, 0, ptr))
{
    _enable_shared_from_this<T>(ptr, data);
}

template<typename T>
shared_ptr<T>::shared_ptr(const shared_ptr_pool_scope<T>& scope, uint32_t poolOffset, T* ptr)
{
    shared_ptr_pool<T>* pool = scope.pool;
    data = new(pool->metaMem(poolOffset)) Data(1, 0, ptr, pool, poolOffset);
    if (pool)
        ++pool->count;

    _enable_shared_from_this<T>(ptr, data);
}

template<typename T>
inline shared_ptr<T>::shared_ptr(const shared_ptr<T>& copy)
    : data(copy.data)
{
    ++data->sharedCount;
}

template<typename T>
inline shared_ptr<T>::shared_ptr(Data* data)
    : data(data)
{
    ++data->sharedCount;
}

template<typename T>
inline shared_ptr<T>::~shared_ptr()
{
    clear();
}

template<typename T>
shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr<T>& copy)
{
    clear();
    data = copy.data;
    ++data->sharedCount;
    return *this;
}

template<typename T>
inline void shared_ptr<T>::clear()
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
inline void shared_ptr<T>::reset(T* ptr)
{
    clear();
    data = new Data(1, 0, ptr);
    _enable_shared_from_this<T>(ptr, data);
}

template<typename T>
inline void shared_ptr<T>::reset(const shared_ptr_pool_scope<T>& scope, uint32_t poolOffset, T* ptr)
{
    clear();
    shared_ptr_pool<T>* pool = scope.pool;
    data = new(pool->metaMem(poolOffset)) Data(1, 0, ptr, pool, poolOffset);
    if (pool)
        ++pool->count;
    _enable_shared_from_this<T>(ptr, data);
}

template<typename T>
inline shared_ptr_pool_scope<T> shared_ptr<T>::make_pool(uint32_t num)
{
    shared_ptr_pool<T>* pool = new shared_ptr_pool<T>(num);
    return shared_ptr_pool_scope<T>(pool);
}

template<typename T>
shared_ptr<T> shared_ptr<T>::allocate(const shared_ptr_pool_scope<T>& scope, uint32_t offset)
{
    T* t = new(scope.mem(offset)) T;
    return shared_ptr<T>(scope, offset, t);
}

template<typename T>
inline weak_ptr<T>::weak_ptr()
    : data(0)
{
}

template<typename T>
inline weak_ptr<T>::weak_ptr(const shared_ptr<T>& shared)
    : data(shared.data)
{
    ++data->weakCount;
}

template<typename T>
inline weak_ptr<T>::weak_ptr(const weak_ptr<T>& weak)
    : data(weak.data)
{
    if (data)
        ++data->weakCount;
}

template<typename T>
inline weak_ptr<T>::~weak_ptr()
{
    clear();
}

template<typename T>
inline void weak_ptr<T>::assign(typename shared_ptr<T>::Data* d)
{
    clear();
    data = d;
    ++data->weakCount;
}

template<typename T>
inline void weak_ptr<T>::clear()
{
    if (data) {
        typename shared_ptr<T>::Data* d = data;
        data = 0;
        if (!d->sharedCount) {
            assert(!d->ptr);
            assert(d->weakCount > 0);
            if (!--d->weakCount) {
                shared_ptr<T>::Data::reset(d);
            }
        } else {
            assert(d->weakCount > 0);
            --d->weakCount;
        }
    }
}

template<typename T>
inline weak_ptr<T>& weak_ptr<T>::operator=(const shared_ptr<T>& copy)
{
    clear();
    data = copy.data;
    ++data->weakCount;
    return *this;
}

template<typename T>
inline weak_ptr<T>& weak_ptr<T>::operator=(const weak_ptr<T>& copy)
{
    clear();
    data = copy.data;
    if (data)
        ++data->weakCount;
    return *this;
}

template<typename T>
inline bool weak_ptr<T>::expired() const
{
    return !data || !data->ptr;
}

template<typename T>
inline shared_ptr<T> weak_ptr<T>::lock() const
{
    if (data && data->ptr) {
        assert(data->sharedCount > 0);
        return shared_ptr<T>(data);
    }
    return shared_ptr<T>();
}

template<typename T>
class enable_shared_from_this
{
public:
    shared_ptr<T> shared_from_this();
    shared_ptr<const T> shared_from_this() const;

private:
    void assign(typename shared_ptr<T>::Data* data);

    weak_ptr<T> _shared_from_this;

    friend void _enable_shared_from_this<T>(enable_shared_from_this<T>* that, void* data);
};

template<typename T>
inline shared_ptr<T> enable_shared_from_this<T>::shared_from_this()
{
    return _shared_from_this.lock();
}

template<typename T>
inline shared_ptr<const T> enable_shared_from_this<T>::shared_from_this() const
{
    return _shared_from_this.lock();
}

template<typename T>
inline void enable_shared_from_this<T>::assign(typename shared_ptr<T>::Data* data)
{
    _shared_from_this.assign(data);
}

template<typename T>
class shared_ptr_pool
{
public:
    ~shared_ptr_pool();

private:
    shared_ptr_pool(uint32_t sz);

    void* metaMem(uint32_t pos) const
    {
        return reinterpret_cast<char*>(data) + ((sizeof(typename shared_ptr<T>::Data) + sizeof(T)) * pos);
    }
    void* mem(uint32_t pos) const
    {
        return reinterpret_cast<char*>(data) + ((sizeof(typename shared_ptr<T>::Data) + sizeof(T)) * pos) + sizeof(typename shared_ptr<T>::Data);
    }

    void* data;
    uint32_t count;

    friend class shared_ptr<T>;
    friend class shared_ptr_pool_scope<T>;
};

template<typename T>
shared_ptr_pool<T>::shared_ptr_pool(uint32_t sz)
    : count(0)
{
    data = malloc((sizeof(typename shared_ptr<T>::Data) + sizeof(T)) * sz);
}

template<typename T>
shared_ptr_pool<T>::~shared_ptr_pool()
{
    free(data);
}

template<typename T>
shared_ptr_pool_scope<T>::~shared_ptr_pool_scope()
{
    assert(pool->count > 0);
    if (!--pool->count) {
        delete pool;
    }
}

template<typename T>
shared_ptr_pool_scope<T>::shared_ptr_pool_scope(const shared_ptr_pool_scope<T>& copy)
    : pool(copy.pool)
{
    ++pool->count;
}

template<typename T>
shared_ptr_pool_scope<T>& shared_ptr_pool_scope<T>::operator=(const shared_ptr_pool_scope<T>& copy)
{
    assert(pool->count > 0);
    if (!--pool->count)
        delete pool;
    pool = copy.pool;
    ++pool->count;
    return *this;
}

template<typename T>
inline void* shared_ptr_pool_scope<T>::mem(uint32_t offset) const
{
    return pool->mem(offset);
}

} // namespace nonatomic

#endif
