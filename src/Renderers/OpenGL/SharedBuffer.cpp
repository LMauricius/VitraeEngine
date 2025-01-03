#include "Vitrae/Renderers/OpenGL/SharedBuffer.hpp"

namespace Vitrae
{

OpenGLRawSharedBuffer::OpenGLRawSharedBuffer(const SetupParams &params)
    : RawSharedBuffer(), m_usage(params.usage)
{
    glGenBuffers(1, &m_glBufferHandle);
    String glLabel = String("Buffer ") + params.friendlyName;
    glObjectLabel(GL_BUFFER, m_glBufferHandle, glLabel.size(), glLabel.data());

    if (params.size > 0) {
        resize(params.size);
    }

    if (m_usage & BufferUsageHint::HOST_INIT) {
        requestBufferPtr();
    }
}

OpenGLRawSharedBuffer::~OpenGLRawSharedBuffer()
{
    glDeleteBuffers(1, &m_glBufferHandle);
}

void OpenGLRawSharedBuffer::synchronize()
{
    if (m_bufferPtr) {
        glUnmapNamedBuffer(m_glBufferHandle);
        m_bufferPtr = nullptr;

        // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
        {
            MMETER_SCOPE_PROFILER("Waiting for GL operations");

            glFinish();
        }
#endif
    }
}

bool OpenGLRawSharedBuffer::isSynchronized() const
{
    return m_bufferPtr == nullptr;
}

void OpenGLRawSharedBuffer::requestBufferPtr() const
{
    if (!m_bufferPtr) {
        m_bufferPtr = (Byte *)glMapNamedBuffer(m_glBufferHandle, GL_READ_WRITE);
        m_dirtySpan = {0, 0};
    }
}

void OpenGLRawSharedBuffer::requestResizeBuffer(std::size_t size) const
{
    // have to unmap to do GL operations on it
    if (m_bufferPtr) {
        glUnmapNamedBuffer(m_glBufferHandle);
        m_bufferPtr = nullptr;
    }
    glBindBuffer(getGlTarget(), m_glBufferHandle);
    glBufferData(getGlTarget(), size, nullptr, getGlUsage());
    glBindBuffer(getGlTarget(), 0);
}

GLenum OpenGLRawSharedBuffer::getGlUsage() const
{
    enum {
        STREAM,
        STATIC,
        DYNAMIC
    } frequency;
    enum {
        DRAW,
        READ,
        COPY
    } nature;
    constexpr GLenum combinedUsage[3][3] = {{GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY},
                                            {GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY},
                                            {GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY}};

    if (m_usage & (BufferUsageHint::HOST_WRITE | BufferUsageHint::GPU_COMPUTE)) {
        frequency = DYNAMIC;
    } else if (m_usage & (BufferUsageHint::HOST_READ | BufferUsageHint::GPU_DRAW)) {
        frequency = STATIC;
    } else {
        frequency = STREAM;
    }

    if (m_usage & BufferUsageHint::GPU_DRAW) {
        nature = DRAW;
    } else if (m_usage & BufferUsageHint::HOST_READ) {
        nature = READ;
    } else {
        nature = COPY;
    }

    return combinedUsage[frequency][nature];
}

GLenum OpenGLRawSharedBuffer::getGlTarget() const
{
    constexpr GLint maxUBOSize = 16000; // ~16kb
    /*
    To get the proper size of the UBO, use:
    []() {
        GLint size;
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &size);
        return size;
    }();
    */
    if (m_size >= maxUBOSize || m_usage & BufferUsageHint::GPU_COMPUTE) {
        return GL_SHADER_STORAGE_BUFFER;
    } else {
        return GL_UNIFORM_BUFFER;
    }
}

std::size_t OpenGLRawSharedBuffer::memory_cost() const
{
    return sizeof(*this) + m_size;
}

} // namespace Vitrae