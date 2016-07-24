#ifndef SHAREDPTR_NONATOMIC_H
#define SHAREDPTR_NONATOMIC_H

#include <cstdint>
#include <cassert>

namespace nonatomic {

template<typename T>
class WeakPtr;

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
    friend class SharedPtrPool;
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

    typename SharedPtr<T>::Data* data;
};

template<typename T>
inline SharedPtr<T>::SharedPtr()
    : data(new Data(1, 0, 0))
{
}

template<typename T>
SharedPtr<T>::SharedPtr(T* ptr)
    : data(new Data(1, 0, ptr))
{
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
    if (!--data->sharedCount) {
        delete data->ptr;
        data->ptr = 0;
        if (!data->weakCount)
            delete data;
    }
    data = 0;
}

template<typename T>
inline void SharedPtr<T>::reset(T* ptr)
{
    clear();
    data = new Data(1, 0, ptr);
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
inline void WeakPtr<T>::clear()
{
    if (data && !data->sharedCount) {
        assert(!data->ptr);
        if (!--data->weakCount) {
            delete data;
        }
    }
    data = 0;
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

} // namespace nonatomic

#endif
