#pragma once

#include "Vitrae/Assets/SharedBuffer.hpp"

#include "glad/glad.h"

namespace Vitrae
{

class OpenGLRawSharedBuffer : public RawSharedBuffer
{
  public:
    OpenGLRawSharedBuffer(const SetupParams &params);
    ~OpenGLRawSharedBuffer();

    void synchronize() override;
    bool isSynchronized() const override;
    std::size_t memory_cost() const override;

    inline GLuint getGlBufferHandle() const { return m_glBufferHandle; }

  private:
    void requestBufferPtr() const override;
    void requestResizeBuffer(std::size_t size) const override;

    GLenum getGlUsage() const;
    GLenum getGlTarget() const;

    GLuint m_glBufferHandle;
    BufferUsageHints m_usage;
};

} // namespace Vitrae
