#ifndef SHAREDPTR_NONATOMIC_H
#define SHAREDPTR_NONATOMIC_H

#include <cstdint>
#include <cassert>

namespace nonatomic {

template<typename T>
class WeakPtr;

template<typename T>
class EnableSharedFromThis;

template<typename T>
void _enableSharedFromThis(EnableSharedFromThis<T>* that, void* data);
template<typename T>
void _enableSharedFromThis(...) { }

template<typename T>
class SharedPtr
{
public:
    SharedPtr();
    SharedPtr(T* ptr);
    SharedPtr(const SharedPtr<T>& copy);
    ~SharedPtr();

    SharedPtr<T>& operator=(const SharedPtr<T>& copy);

    void reset(T* ptr = 0);

    T* operator->() { return data->ptr; }
    const T* operator->() const { return data->ptr; }
    T& operator*() { return *data->ptr; }
    const T& operator*() const { return *data->ptr; }

    operator bool() const { return data->ptr != 0; }

private:
    class Data
    {
    public:
        Data() : sharedCount(0), weakCount(0), ptr(0) { }
        Data(uint32_t sc, uint32_t wc, T* p) : sharedCount(sc), weakCount(wc), ptr(p) { }

        uint32_t sharedCount;
        uint32_t weakCount;
        T* ptr;

        //SharedPtrPool* pool;
        //uint32_t poolPosition;
    };

    SharedPtr(Data* data);

    void clear();

    Data* data;

    friend class WeakPtr<T>;
    friend class EnableSharedFromThis<T>;
    friend class SharedPtrPool;
    friend void _enableSharedFromThis<T>(EnableSharedFromThis<T>* that, void* data);
};

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
inline SharedPtr<T>::SharedPtr()
    : data(new Data(1, 0, 0))
{
}

template<typename T>
SharedPtr<T>::SharedPtr(T* ptr)
    : data(new Data(1, 0, ptr))
{
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
}

template<typename T>
inline void SharedPtr<T>::clear()
{
    Data* d = data;
    data = 0;

    assert(d->sharedCount > 0);
    if (!--d->sharedCount) {
        T* ptr = d->ptr;
        d->ptr = 0;
        delete ptr;
        if (!d->weakCount)
            delete d;
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
                delete d;
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

} // namespace nonatomic

#endif
