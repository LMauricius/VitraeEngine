#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "glad/glad.h"

#include <filesystem>

namespace Vitrae
{
class ComponentRoot;
class OpenGLRenderer;

/**
 * A Texture is a single image-like resource
 */
class OpenGLTexture : public Texture
{
  public:
    OpenGLTexture(const FileLoadParams &params);
    OpenGLTexture(const EmptyParams &params);
    OpenGLTexture(const PureColorParams &params);
    ~OpenGLTexture();

    void loadToGPU(const unsigned char *data, StringView friendlyName);
    void unloadFromGPU();

    std::size_t memory_cost() const override;

    GLuint glTextureId;

  protected:
    OpenGLTexture(WrappingType horWrap, WrappingType verWrap, FilterType minFilter,
                  FilterType magFilter, bool useMipMaps, glm::vec4 borderColor);

    GLint mGLInternalFormat;
    GLint mGLChannelFormat;
    GLenum mGLChannelType;
    GLint mGLMagFilter, mGLMinFilter, mGLWrapS, mGLWrapT;
    bool mUseMipMaps;
    glm::vec4 mBorderColor;
    union {
        struct
        {
            GLint r, g, b, a;
        } mSwizzle;
        GLint mSwizzleArr[4];
    };
    bool mUseSwizzle;

    bool m_sentToGPU;
};

} // namespace Vitrae