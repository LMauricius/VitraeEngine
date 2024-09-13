#include "Vitrae/Assets/SharedBuffer.hpp"

namespace Vitrae
{
RawSharedBuffer::RawSharedBuffer() : m_size(0), m_bufferPtr(nullptr), m_dirtySpan(0, 0) {}

void RawSharedBuffer::resize(std::size_t size)
{
    if (size > m_size) {
        if (m_dirtySpan.first == m_dirtySpan.second) {
            m_dirtySpan.first = m_size;
        }
        m_dirtySpan.second = size;
    }

    requestResizeBuffer(size);
    m_size = size;
}

const Byte *RawSharedBuffer::data() const
{
    if (!m_bufferPtr)
        requestBufferPtr();
    return m_bufferPtr;
}

Byte *RawSharedBuffer::data()
{
    if (!m_bufferPtr)
        requestBufferPtr();
    m_dirtySpan = {0, m_size};
    return m_bufferPtr;
}

std::span<const Byte> RawSharedBuffer::operator[](std::pair<std::size_t, std::size_t> slice) const
{
    if (!m_bufferPtr)
        requestBufferPtr();

    return std::span<const Byte>(m_bufferPtr + slice.first, slice.second - slice.first);
}

std::span<Byte> RawSharedBuffer::operator[](std::pair<std::size_t, std::size_t> slice)
{
    if (!m_bufferPtr)
        requestBufferPtr();

    if (m_dirtySpan.first > slice.first)
        m_dirtySpan.first = slice.first;

    if (m_dirtySpan.second < slice.second)
        m_dirtySpan.second = slice.second;

    return std::span<Byte>(m_bufferPtr + slice.first, slice.second - slice.first);
}

} // namespace Vitrae