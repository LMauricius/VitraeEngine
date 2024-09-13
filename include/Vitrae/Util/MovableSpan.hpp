#pragma once

#include <array>
#include <iterator>

namespace Vitrae
{

/**
 * @brief this class is akin to a modifiable initializer_list
 */
template <typename T> class MovableSpan
{
    T *m_data;
    std::size_t m_size;

  public:
    MovableSpan() = delete;
    MovableSpan(const MovableSpan &) = delete;
    MovableSpan(MovableSpan &&other) = default;
    MovableSpan(T (&&data)[], std::size_t size) : m_data(data), m_size(size) {}
    template <std::size_t N> MovableSpan(T (&&data)[N]) : m_data(data), m_size(N) {}
    template <std::size_t N> MovableSpan(std::array<T, N> &&data) : m_data(data.data()), m_size(N)
    {}

    ~MovableSpan() = default;

    MovableSpan &operator=(const MovableSpan &) = delete;
    MovableSpan &operator=(MovableSpan &&other) = delete;

    T *data() const { return m_data; }
    std::size_t size() const { return m_size; }

    T &&operator[](std::size_t index) const { return std::move(m_data[index]); }

    T *begin() const { return m_data; }
    T *end() const { return m_data + m_size; }
    const T *cbegin() const { return m_data; }
    const T *cend() const { return m_data + m_size; }
    std::reverse_iterator<T *> rbegin() const
    {
        return std::reverse_iterator<T *>(m_data + m_size);
    }
    std::reverse_iterator<T *> rend() const { return std::reverse_iterator<T *>(m_data); }
    std::reverse_iterator<const T *> crbegin() const
    {
        return std::reverse_iterator<const T *>(m_data + m_size);
    }
    std::reverse_iterator<const T *> crend() const
    {
        return std::reverse_iterator<const T *>(m_data);
    }
};
} // namespace Vitrae