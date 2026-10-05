#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <new>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Vitrae
{

/**
 * @brief A map that expects its keys to be searched through often, but rarely changed
 * @note Modifying the values is fast
 *
 * @tparam KeyT the type of the key
 * @tparam MappedT the type of the value
 * @tparam CompT the comparison functor type
 */
template <class KeyT, class MappedT> class StableMap
{
    template <class KeyRefT, class MappedRefT> class AbstractStableMapIterator
    {
        friend class StableMap;

        KeyRefT *mp_key;
        MappedRefT *mp_value;

        AbstractStableMapIterator(KeyRefT *key, MappedRefT *value) : mp_key(key), mp_value(value) {}

      public:
        using iterator_category = std::random_access_iterator_tag;

        using value_type = std::pair<KeyRefT &, MappedRefT &>;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using allocator_type = std::allocator<std::byte>;

        using pointer = std::pair<KeyRefT *, MappedRefT *>;
        using reference = std::pair<KeyRefT &, MappedRefT &>;

        AbstractStableMapIterator() : mp_key(nullptr), mp_value(nullptr) {}
        AbstractStableMapIterator(const AbstractStableMapIterator &) = default;
        AbstractStableMapIterator(AbstractStableMapIterator &&) = default;

        template <class OtherKeyRefT, class OtherMappedRefT>
            requires(std::convertible_to<OtherKeyRefT *, KeyRefT *> &&
                     std::convertible_to<OtherMappedRefT *, MappedRefT *>)
        AbstractStableMapIterator(AbstractStableMapIterator<OtherKeyRefT, OtherMappedRefT> &&other)
            : mp_key(other.mp_key), mp_value(other.mp_value)
        {}
        template <class OtherKeyRefT, class OtherMappedRefT>
            requires(std::convertible_to<OtherKeyRefT *, KeyRefT *> &&
                     std::convertible_to<OtherMappedRefT *, MappedRefT *>)
        AbstractStableMapIterator(
            const AbstractStableMapIterator<OtherKeyRefT, OtherMappedRefT> &other)
            : mp_key(other.mp_key), mp_value(other.mp_value)
        {}

        AbstractStableMapIterator &operator=(const AbstractStableMapIterator &other) = default;
        AbstractStableMapIterator &operator=(AbstractStableMapIterator &&other) = default;

        template <class OtherKeyRefT, class OtherMappedRefT>
            requires(std::convertible_to<OtherKeyRefT, KeyRefT> &&
                     std::convertible_to<OtherMappedRefT, MappedRefT>)
        AbstractStableMapIterator &operator=(
            AbstractStableMapIterator<OtherKeyRefT, OtherMappedRefT> &&other)
        {
            mp_key = other.mp_key;
            mp_value = other.mp_value;
            return *this;
        }

        auto operator++()
        {
            mp_key++;
            mp_value++;
            return *this;
        }
        auto operator--()
        {
            mp_key--;
            mp_value--;
            return *this;
        }

        auto operator++(int)
        {
            auto tmp = *this;
            mp_key++;
            mp_value++;
            return tmp;
        }
        auto operator--(int)
        {
            auto tmp = *this;
            mp_key--;
            mp_value--;
            return tmp;
        }

        auto operator+=(difference_type n)
        {
            mp_key += n;
            mp_value += n;
            return *this;
        }
        auto operator-=(difference_type n)
        {
            mp_key -= n;
            mp_value -= n;
            return *this;
        }

        auto operator+(difference_type n)
        {
            return AbstractStableMapIterator(mp_key + n, mp_value + n);
        }
        auto operator-(difference_type n)
        {
            return AbstractStableMapIterator(mp_key - n, mp_value - n);
        }
        friend auto operator+(difference_type n, const AbstractStableMapIterator &it)
        {
            return it + n;
        }
        friend auto operator-(difference_type n, const AbstractStableMapIterator &it)
        {
            return it - n;
        }

        auto operator-(const AbstractStableMapIterator &other) { return mp_key - other.mp_key; }

        bool operator==(const AbstractStableMapIterator &other) const
        {
            return mp_key == other.mp_key;
        }
        auto operator<=>(const AbstractStableMapIterator &other) const
        {
            return mp_key <=> other.mp_key;
        }

        auto operator*() const { return value_type(*mp_key, *mp_value); }
    };

  public:
    using StableMapIterator = AbstractStableMapIterator<const KeyT, MappedT>;
    using CStableMapIterator = AbstractStableMapIterator<const KeyT, const MappedT>;

    using key_type = KeyT;
    using mapped_type = MappedT;
    using value_type = StableMapIterator::value_type;
    using const_value_type = CStableMapIterator::value_type;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = std::allocator<std::byte>;

    using pointer = StableMapIterator::pointer;
    using const_pointer = CStableMapIterator::pointer;
    using reference = StableMapIterator::reference;
    using const_reference = CStableMapIterator::reference;
    using iterator = StableMapIterator;
    using const_iterator = CStableMapIterator;

    StableMap() : m_data(nullptr), m_size(0) {};

    StableMap(const StableMap &o) : m_size(o.m_size), m_data(allocateBuffer(o.m_size))
    {
        constructAll(o.begin());
    }

    StableMap(StableMap &&o) noexcept : m_data(o.m_data), m_size(o.m_size)
    {
        o.m_size = 0;
        o.m_data = nullptr;
    }

    template <class InputItT>
    StableMap(InputItT first, InputItT last)
        requires requires(InputItT it) {
            { (*it).first } -> std::convertible_to<KeyT>;
            { (*it).second } -> std::convertible_to<MappedT>;
        }
    {
        // sort before allocating: vector/sort may throw, nothing to clean up yet
        std::vector<InputItT> sortedIterators;
        for (auto it = first; it != last; ++it) {
            sortedIterators.emplace_back(it);
        }
        std::sort(sortedIterators.begin(), sortedIterators.end(),
                  [&](auto a, auto b) { return (*a).first < (*b).first; });

        m_size = sortedIterators.size();
        m_data = allocateBuffer(m_size);
        constructAll(std::views::transform(sortedIterators, [](const InputItT &it) -> decltype(auto) {
                         return *it;
                     }).begin());
    }

    StableMap(std::initializer_list<std::pair<KeyT, MappedT>> initList)
        : StableMap(initList.begin(), initList.end())
    {}

    template <class OKeyT, class OMappedT>
    StableMap(const std::map<OKeyT, OMappedT> &orderedList)
        requires std::convertible_to<OKeyT, KeyT> && std::convertible_to<OMappedT, MappedT>
        : m_size(orderedList.size()), m_data(allocateBuffer(orderedList.size()))
    {
        constructAll(orderedList.begin());
    }

    template <class OKeyT, class OMappedT>
    StableMap(std::map<OKeyT, OMappedT> &&orderedList)
        requires std::convertible_to<OKeyT, KeyT> && std::convertible_to<OMappedT, MappedT>
        : m_size(orderedList.size()), m_data(allocateBuffer(orderedList.size()))
    {
        // move iterator yields rvalue pairs: values moved, const keys copied
        constructAll(std::make_move_iterator(orderedList.begin()));
    }

    ~StableMap()
    {
        destroyFirst(m_size);
        freeBuffer(m_data);
    }

    StableMap &operator=(const StableMap &o)
    {
        if (this != &o)
            *this = StableMap(o);
        return *this;
    }
    StableMap &operator=(StableMap &&o) noexcept
    {
        if (this != &o) {
            destroyFirst(m_size);
            freeBuffer(m_data);
            m_size = o.m_size;
            m_data = o.m_data;
            o.m_size = 0;
            o.m_data = nullptr;
        }
        return *this;
    }

    std::size_t size() const { return m_size; }
    std::span<const KeyT> keys() const { return std::span<const KeyT>(getKeyList(), m_size); }
    std::span<MappedT> values() { return std::span<MappedT>(getValueList(), m_size); }
    std::span<const MappedT> values() const
    {
        return std::span<const MappedT>(getValueList(), m_size);
    }

    StableMapIterator begin() { return StableMapIterator(getKeyList(), getValueList()); }
    StableMapIterator end()
    {
        return StableMapIterator(getKeyList() + m_size, getValueList() + m_size);
    }
    CStableMapIterator begin() const { return CStableMapIterator(getKeyList(), getValueList()); }
    CStableMapIterator end() const
    {
        return CStableMapIterator(getKeyList() + m_size, getValueList() + m_size);
    }
    CStableMapIterator cbegin() const { return CStableMapIterator(getKeyList(), getValueList()); }
    CStableMapIterator cend() const
    {
        return CStableMapIterator(getKeyList() + m_size, getValueList() + m_size);
    }

    StableMapIterator find(const KeyT &key)
    {
        std::size_t ind = findClosestIndex(key);
        if (ind < m_size && !(key < getKeyList()[ind])) {
            return StableMapIterator(getKeyList() + ind, getValueList() + ind);
        }
        return end();
    }

    CStableMapIterator find(const KeyT &key) const
    {
        std::size_t ind = findClosestIndex(key);
        if (ind < m_size && !(key < getKeyList()[ind])) {
            return CStableMapIterator(getKeyList() + ind, getValueList() + ind);
        }
        return cend();
    }

    StableMapIterator lower_bound(const KeyT &key)
    {
        std::size_t ind = findClosestIndex(key);
        return StableMapIterator(getKeyList() + ind, getValueList() + ind);
    }

    CStableMapIterator lower_bound(const KeyT &key) const
    {
        std::size_t ind = findClosestIndex(key);
        return CStableMapIterator(getKeyList() + ind, getValueList() + ind);
    }

    StableMapIterator lower_bound_next(const KeyT &key, CStableMapIterator start)
    {
        std::size_t ind = start - cbegin();
        if (key < getKeyList()[ind]) {
            throw std::out_of_range("lower_bound_next: key is not in range [start, ...)");
        } else if (getKeyList()[ind] < key) {
            if (ind + 1 < m_size && getKeyList()[ind + 1] < key) {
                ind = findClosestIndex(key, ind + 2, m_size, (ind + 2 + m_size) / 2);
                return StableMapIterator(getKeyList() + ind, getValueList() + ind);
            }
            return StableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
        }

        return StableMapIterator(getKeyList() + ind, getValueList() + ind);
    }

    CStableMapIterator lower_bound_next(const KeyT &key, CStableMapIterator start) const
    {
        std::size_t ind = start - cbegin();
        if (key < getKeyList()[ind]) {
            throw std::out_of_range("lower_bound_next: key is not in range [start, ...)");
        } else if (getKeyList()[ind] < key) {
            if (ind + 1 < m_size && getKeyList()[ind + 1] < key) {
                ind = findClosestIndex(key, ind + 2, m_size, (ind + 2 + m_size) / 2);
                return CStableMapIterator(getKeyList() + ind, getValueList() + ind);
            }
            return CStableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
        }

        return start;
    }

    StableMapIterator upper_bound(const KeyT &key)
    {
        std::size_t ind = findClosestIndex(key);
        if (ind < m_size && !(key < getKeyList()[ind])) {
            return StableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
        }
        return StableMapIterator(getKeyList() + ind, getValueList() + ind);
    }

    CStableMapIterator upper_bound(const KeyT &key) const
    {
        std::size_t ind = findClosestIndex(key);
        if (ind < m_size && !(key < getKeyList()[ind])) {
            return CStableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
        }
        return CStableMapIterator(getKeyList() + ind, getValueList() + ind);
    }

    StableMapIterator upper_bound_next(const KeyT &key, CStableMapIterator start)
    {
        std::size_t ind = start - cbegin();
        if (key < getKeyList()[ind]) {
            throw std::out_of_range("upper_bound_next: key is not in range [start, ...)");
        } else if (getKeyList()[ind] < key) {
            // next is end or greater
            if (ind + 1 == m_size || key < getKeyList()[ind + 1]) {
                return StableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
            }
            // next is equal
            if (!(getKeyList()[ind + 1] < key)) {
                return StableMapIterator(getKeyList() + ind + 2, getValueList() + ind + 2);
            }
            // next is smaller: binary search the rest
            ind = findClosestIndex(key, ind + 2, m_size, (ind + 2 + m_size) / 2);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                ++ind;
            }
            return StableMapIterator(getKeyList() + ind, getValueList() + ind);
        }

        return StableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
    }

    CStableMapIterator upper_bound_next(const KeyT &key, CStableMapIterator start) const
    {
        std::size_t ind = start - cbegin();
        if (key < getKeyList()[ind]) {
            throw std::out_of_range("upper_bound_next: key is not in range [start, ...)");
        } else if (getKeyList()[ind] < key) {
            // next is end or greater
            if (ind + 1 == m_size || key < getKeyList()[ind + 1]) {
                return CStableMapIterator(getKeyList() + ind + 1, getValueList() + ind + 1);
            }
            // next is equal
            if (!(getKeyList()[ind + 1] < key)) {
                return CStableMapIterator(getKeyList() + ind + 2, getValueList() + ind + 2);
            }
            // next is smaller: binary search the rest
            ind = findClosestIndex(key, ind + 2, m_size, (ind + 2 + m_size) / 2);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                ++ind;
            }
            return CStableMapIterator(getKeyList() + ind, getValueList() + ind);
        }

        return start + 1;
    }

    MappedT &operator[](const KeyT &key)
    {
        if constexpr (std::is_default_constructible_v<MappedT>) {
            std::size_t ind;
            if (m_size > 0) {
                ind = findClosestIndex(key);
                if (ind < m_size && !(key < getKeyList()[ind])) {
                    return getValueList()[ind];
                }
            } else {
                ind = 0;
            }

            std::byte *p_oldData = m_data;
            start_realloc_w_uninit();
            try {
                new (getKeyList() + ind) KeyT(key);
            }
            catch (...) {
                abort_realloc_w_uninit(p_oldData);
                throw;
            }
            try {
                new (getValueList() + ind) MappedT();
            }
            catch (...) {
                getKeyList()[ind].~KeyT();
                abort_realloc_w_uninit(p_oldData);
                throw;
            }
            finish_realloc_w_uninit(p_oldData, ind);
            return getValueList()[ind];
        } else {
            return at(key);
        }
    }
    const MappedT &operator[](const KeyT &key) const { return at(key); }

    MappedT &at(const KeyT &key)
    {
        if (m_size > 0) {
            std::size_t ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                return getValueList()[ind];
            }
        }
        throw std::out_of_range("Key not found");
    }

    const MappedT &at(const KeyT &key) const
    {
        if (m_size > 0) {
            std::size_t ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                return getValueList()[ind];
            }
        }
        throw std::out_of_range("Key not found");
    }

    template <class... Args> std::pair<iterator, bool> emplace(const KeyT &key, Args &&...args)
    {
        std::size_t ind;
        if (m_size > 0) {
            ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                return std::make_pair(iterator(getKeyList() + ind, getValueList() + ind), false);
            }
        } else {
            ind = 0;
        }

        std::byte *p_oldData = m_data;
        start_realloc_w_uninit();
        try {
            new (getKeyList() + ind) KeyT(key);
        }
        catch (...) {
            abort_realloc_w_uninit(p_oldData);
            throw;
        }
        try {
            new (getValueList() + ind) MappedT(std::forward<Args>(args)...);
        }
        catch (...) {
            getKeyList()[ind].~KeyT();
            abort_realloc_w_uninit(p_oldData);
            throw;
        }
        finish_realloc_w_uninit(p_oldData, ind);
        return std::make_pair(iterator(getKeyList() + ind, getValueList() + ind), true);
    }

    /**
     * Safe even if obj refers to an element of this map (unlike `map[key] = obj`, where
     * operator[] may free the buffer obj lives in before the assignment reads it)
     */
    template <class M> std::pair<iterator, bool> insert_or_assign(const KeyT &key, M &&obj)
    {
        // emplace constructs from obj before freeing the old buffer.
        // obj is forwarded twice, but emplace only consumes it when inserting
        auto res = emplace(key, std::forward<M>(obj));
        if (!res.second) {
            (*res.first).second = std::forward<M>(obj);
        }
        return res;
    }

    std::pair<iterator, bool> insert(const std::pair<const KeyT, MappedT> &value)
    {
        return emplace(value.first, value.second);
    }

    std::pair<iterator, bool> insert(std::pair<const KeyT &, MappedT &> value)
    {
        return emplace(value.first, value.second);
    }

    std::size_t erase(const KeyT &key)
    {
        if (m_size > 0) {
            std::size_t ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                realloc_w_erased(ind);
                return 1;
            }
        }

        return 0;
    }

    std::size_t erase(CStableMapIterator pos)
    {
        std::size_t ind = pos.mp_key - getKeyList();
        assert(ind < m_size);
        realloc_w_erased(ind);
        return 1;
    }

    void clear()
    {
        destroyFirst(m_size);
        freeBuffer(m_data);
        m_size = 0;
        m_data = nullptr;
    }

  protected:
    std::size_t m_size;
    std::byte *m_data; // starts with keys, also contains values. Owned by the map

    static constexpr std::size_t getValueBufferOffset(std::size_t numElements)
    {
        return (alignof(MappedT) <= alignof(KeyT))
                   ? numElements * sizeof(KeyT)
                   : (numElements * sizeof(KeyT) + alignof(MappedT) - 1) / alignof(MappedT) *
                         alignof(MappedT);
    }

    static constexpr std::size_t getBufferSize(std::size_t numElements)
    {
        return getValueBufferOffset(numElements) + numElements * sizeof(MappedT);
    }

    // aligned operator new: honors over-aligned keys/values (new std::byte[] only guarantees
    // __STDCPP_DEFAULT_NEW_ALIGNMENT__). Must be paired with freeBuffer
    static std::byte *allocateBuffer(std::size_t numElements)
    {
        return static_cast<std::byte *>(::operator new(
            getBufferSize(numElements), std::align_val_t(std::max(alignof(KeyT), alignof(MappedT)))));
    }

    static void freeBuffer(std::byte *data) noexcept
    {
        ::operator delete(data, std::align_val_t(std::max(alignof(KeyT), alignof(MappedT))));
    }

    /// destroys elements [0,n)
    void destroyFirst(std::size_t n) noexcept
    {
        for (std::size_t i = 0; i < n; ++i) {
            getKeyList()[i].~KeyT();
            getValueList()[i].~MappedT();
        }
    }

    /**
     * Constructs elements [0,m_size) in the allocated m_data from consecutive pair-likes at `it`.
     * Members of rvalue pairs are forwarded as rvalues (moved).
     * For constructors only: on throw destroys built elements, frees m_data and rethrows,
     * since the dtor won't run for a throwing ctor
     */
    template <class It> void constructAll(It it)
    {
        std::size_t i = 0;
        try {
            for (; i < m_size; ++i, ++it) {
                auto &&kv = *it;
                new (getKeyList() + i) KeyT(std::forward<decltype(kv)>(kv).first);
                try {
                    new (getValueList() + i) MappedT(std::forward<decltype(kv)>(kv).second);
                } catch (...) {
                    getKeyList()[i].~KeyT();
                    throw;
                }
            }
        } catch (...) {
            destroyFirst(i);
            freeBuffer(m_data);
            throw;
        }
    }

    std::size_t findClosestIndex(const KeyT &key) const
    {
        return findClosestIndex(key, 0, m_size, m_size / 2);
    }

    std::size_t findClosestIndex(const KeyT &key, std::size_t leftIndex, std::size_t rightIndex,
                                 std::size_t midIndex) const
    {
        // uses binary search to find the index of the closest key
        const KeyT *keyList = getKeyList();
        if (leftIndex < rightIndex) {
            if (keyList[midIndex] < key) {
                leftIndex = midIndex + 1;
            } else {
                rightIndex = midIndex;
            }

            while (leftIndex < rightIndex) {
                midIndex = (leftIndex + rightIndex) / 2;
                if (keyList[midIndex] < key) {
                    leftIndex = midIndex + 1;
                } else {
                    rightIndex = midIndex;
                }
            }
        }
        return leftIndex;
    }

    KeyT *getKeyList() { return reinterpret_cast<KeyT *>(m_data); }
    const KeyT *getKeyList() const { return reinterpret_cast<const KeyT *>(m_data); }
    MappedT *getValueList()
    {
        return reinterpret_cast<MappedT *>(m_data + getValueBufferOffset(m_size));
    }
    const MappedT *getValueList() const
    {
        return reinterpret_cast<const MappedT *>(m_data + getValueBufferOffset(m_size));
    }

    void realloc_w_erased(std::size_t erasingIndex)
    {
        // a throwing move would leave elems split between buffers
        static_assert(std::is_nothrow_move_constructible_v<KeyT> &&
                      std::is_nothrow_move_constructible_v<MappedT>);

        std::byte *newData = allocateBuffer(m_size - 1);
        KeyT *newKeyList = reinterpret_cast<KeyT *>(newData);
        MappedT *newValueList =
            reinterpret_cast<MappedT *>(newData + getValueBufferOffset(m_size - 1));

        // move data before
        for (std::size_t i = 0; i < erasingIndex; ++i) {
            new (newKeyList + i) KeyT(std::move(getKeyList()[i]));
            new (newValueList + i) MappedT(std::move(getValueList()[i]));
            getKeyList()[i].~KeyT();
            getValueList()[i].~MappedT();
        }

        // erase
        getKeyList()[erasingIndex].~KeyT();
        getValueList()[erasingIndex].~MappedT();

        // move data after
        for (std::size_t i = erasingIndex + 1; i < m_size; ++i) {
            new (newKeyList + i - 1) KeyT(std::move(getKeyList()[i]));
            new (newValueList + i - 1) MappedT(std::move(getValueList()[i]));
            getKeyList()[i].~KeyT();
            getValueList()[i].~MappedT();
        }

        freeBuffer(m_data);
        m_data = newData;
        --m_size;
    }

    void start_realloc_w_uninit()
    {
        m_data = allocateBuffer(m_size + 1);
        ++m_size;
    }

    // undoes start_realloc_w_uninit; new buffer must hold no constructed elems
    void abort_realloc_w_uninit(std::byte *oldData)
    {
        freeBuffer(m_data);
        m_data = oldData;
        --m_size;
    }

    void finish_realloc_w_uninit(std::byte *oldData, std::size_t uninitIndex)
    {
        // a throwing move would leave elems split between buffers
        static_assert(std::is_nothrow_move_constructible_v<KeyT> &&
                      std::is_nothrow_move_constructible_v<MappedT>);

        KeyT *oldKeyList = reinterpret_cast<KeyT *>(oldData);
        MappedT *oldValueList =
            reinterpret_cast<MappedT *>(oldData + getValueBufferOffset(m_size - 1));

        // move data before
        for (std::size_t i = 0; i < uninitIndex; ++i) {
            new (getKeyList() + i) KeyT(std::move(oldKeyList[i]));
            new (getValueList() + i) MappedT(std::move(oldValueList[i]));
            oldKeyList[i].~KeyT();
            oldValueList[i].~MappedT();
        }

        // move data after
        for (std::size_t i = uninitIndex; i < m_size - 1; ++i) {
            new (getKeyList() + i + 1) KeyT(std::move(oldKeyList[i]));
            new (getValueList() + i + 1) MappedT(std::move(oldValueList[i]));
            oldKeyList[i].~KeyT();
            oldValueList[i].~MappedT();
        }

        freeBuffer(oldData);
    }
};

} // namespace Vitrae