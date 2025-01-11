#pragma once

#include <memory>

namespace Vitrae
{

/**
 * @brief a class that acts like a unique ptr to void
 */
class UniqueAnyPtr
{
    void *mPtr;
    void (*mDeleter)(void *);

  public:
    // constructors
    inline UniqueAnyPtr() : mPtr(nullptr), mDeleter(nullptr) {}
    template <class T> UniqueAnyPtr(T *ptr)
    {
        mPtr = ptr;
        mDeleter = [](void *ptr) { delete static_cast<T *>(ptr); };
    }

    template <class T> UniqueAnyPtr(std::unique_ptr<T> &&ptr)
    {
        mPtr = ptr.release();
        mDeleter = [](void *ptr) { delete static_cast<T *>(ptr); };
    }

    // move constructor
    inline UniqueAnyPtr(UniqueAnyPtr &&other)
    {
        mPtr = other.mPtr;
        mDeleter = other.mDeleter;
        other.mPtr = nullptr;
        other.mDeleter = nullptr;
    }

    inline ~UniqueAnyPtr()
    {
        if (mPtr != nullptr)
            (*mDeleter)(mPtr);
    }

    // move assignment
    template <class T> UniqueAnyPtr &operator=(T *ptr)
    {
        if (mPtr != nullptr)
            (*mDeleter)(mPtr);
        mPtr = ptr;
        mDeleter = [](void *ptr) { delete static_cast<T *>(ptr); };
        return *this;
    }

    template <class T> UniqueAnyPtr &operator=(std::unique_ptr<T> &&ptr)
    {
        if (mPtr != nullptr)
            (*mDeleter)(mPtr);
        mPtr = ptr.release();
        mDeleter = [](void *ptr) { delete static_cast<T *>(ptr); };
        return *this;
    }

    inline UniqueAnyPtr &operator=(UniqueAnyPtr &&other)
    {
        if (mPtr != nullptr)
            (*mDeleter)(mPtr);
        mPtr = other.mPtr;
        mDeleter = other.mDeleter;
        other.mPtr = nullptr;
        other.mDeleter = nullptr;
        return *this;
    }

    inline operator bool() const { return mPtr != nullptr; }

    /**
     * @returns the raw pointer to the object or nullptr if not set
     */
    template <class T> T *get() const { return static_cast<T *>(mPtr); }
};
} // namespace Vitrae