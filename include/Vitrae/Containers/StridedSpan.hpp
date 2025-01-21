#pragma once

#include <cstddef>
#include <iterator>

namespace Vitrae
{

// StridedSpan class
template <class T> class StridedSpan
{
  public:
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using const_pointer = const T *;
    using const_reference = const T &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // Nested iterator class
    class iterator
    {
      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        iterator(char *ptr, std::ptrdiff_t stride) : m_ptr(ptr), m_stride(stride) {}

        iterator &operator++()
        {
            m_ptr += m_stride;
            return *this;
        }

        iterator operator++(int)
        {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        iterator &operator--()
        {
            m_ptr -= m_stride;
            return *this;
        }

        iterator operator--(int)
        {
            iterator temp = *this;
            --(*this);
            return temp;
        }

        reference operator*() const { return *reinterpret_cast<pointer>(m_ptr); }

        bool operator==(const iterator &other) const { return m_ptr == other.m_ptr; }
        bool operator!=(const iterator &other) const { return !(*this == other); }

        iterator operator+(difference_type n) const
        {
            return iterator(m_ptr + n * m_stride, m_stride);
        }

        iterator operator-(difference_type n) const
        {
            return iterator(m_ptr - n * m_stride, m_stride);
        }

        difference_type operator-(const iterator &other) const
        {
            return (m_ptr - other.m_ptr) / static_cast<difference_type>(m_stride);
        }

      private:
        char *m_ptr;
        std::ptrdiff_t m_stride;
    };

    // Constructors
    constexpr StridedSpan() noexcept : m_data(nullptr), m_size(0), m_stride(0) {}

    constexpr StridedSpan(const StridedSpan &other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_stride(other.m_stride)
    {}

    constexpr StridedSpan(pointer begin, pointer end, std::ptrdiff_t stride) noexcept
        : m_data(reinterpret_cast<char *>(begin)),
          m_size(static_cast<size_type>(
              (reinterpret_cast<char *>(end) - reinterpret_cast<char *>(begin)) / stride)),
          m_stride(stride)
    {}

    constexpr StridedSpan(pointer begin, size_type count, std::ptrdiff_t stride) noexcept
        : m_data(reinterpret_cast<char *>(begin)), m_size(count), m_stride(stride)
    {}

    // Element access
    constexpr reference operator[](size_type idx) const
    {
        return *reinterpret_cast<pointer>(m_data + idx * m_stride);
    }

    constexpr reference front() const { return operator[](0); }

    constexpr reference back() const { return operator[](m_size - 1); }

    constexpr pointer data() const noexcept { return reinterpret_cast<pointer>(m_data); }

    // Observers
    constexpr size_type size() const noexcept { return m_size; }

    constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr size_type stride() const noexcept { return m_stride; }

    // Iterators
    constexpr iterator begin() const noexcept { return iterator(m_data, m_stride); }

    constexpr iterator end() const noexcept
    {
        return iterator(m_data + m_size * m_stride, m_stride);
    }

  private:
    char *m_data;
    size_type m_size;
    std::ptrdiff_t m_stride;
};

} // namespace Vitrae