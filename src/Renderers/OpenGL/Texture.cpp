#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

#include "stb/stb_image.h"

#include <map>
#include <vector>

namespace Vitrae
{
OpenGLTexture::OpenGLTexture(WrappingType horWrap, WrappingType verWrap, FilterType minFilter,
                             FilterType magFilter, bool useMipMaps, glm::vec4 borderColor)
    : m_sentToGPU(false)
{
    switch (horWrap) {
    case WrappingType::BORDER_COLOR:
        mGLWrapS = GL_CLAMP_TO_BORDER;
        break;
    case WrappingType::CLAMP:
        mGLWrapS = GL_CLAMP_TO_EDGE;
        break;
    case WrappingType::REPEAT:
        mGLWrapS = GL_REPEAT;
        break;
    case WrappingType::MIRROR:
        mGLWrapS = GL_MIRRORED_REPEAT;
        break;
    }
    switch (verWrap) {
    case WrappingType::BORDER_COLOR:
        mGLWrapT = GL_CLAMP_TO_BORDER;
        break;
    case WrappingType::CLAMP:
        mGLWrapT = GL_CLAMP_TO_EDGE;
        break;
    case WrappingType::REPEAT:
        mGLWrapT = GL_REPEAT;
        break;
    case WrappingType::MIRROR:
        mGLWrapT = GL_MIRRORED_REPEAT;
        break;
    }
    switch (magFilter) {
    case FilterType::LINEAR:
        mGLMagFilter = GL_LINEAR;
        break;
    case FilterType::NEAREST:
        mGLMagFilter = GL_NEAREST;
        break;
    }
    if (useMipMaps) {
        switch (minFilter) {
        case FilterType::LINEAR:
            mGLMinFilter = GL_LINEAR_MIPMAP_LINEAR;
            break;
        case FilterType::NEAREST:
            mGLMinFilter = GL_NEAREST_MIPMAP_NEAREST;
            break;
        }
    } else {
        switch (minFilter) {
        case FilterType::LINEAR:
            mGLMinFilter = GL_LINEAR;
            break;
        case FilterType::NEAREST:
            mGLMinFilter = GL_NEAREST;
            break;
        }
    }
    mUseMipMaps = useMipMaps;
    mBorderColor = borderColor;
}

OpenGLTexture::OpenGLTexture(const FileLoadParams &params)
    : OpenGLTexture(params.horWrap, params.verWrap, params.minFilter, params.magFilter,
                    params.useMipMaps, {0.0f, 0.0f, 0.0f, 0.0f})
{
    int stbChannelFormat;
    unsigned char *data =
        stbi_load(params.filepath.c_str(), &mWidth, &mHeight, &stbChannelFormat, STBI_default);

    m_stats = TextureStats{
        .averageColor = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    mGLChannelType = GL_UNSIGNED_BYTE;
    mUseSwizzle = false;
    switch (stbChannelFormat) {
    case STBI_grey:
        mGLInternalFormat = GL_RED;
        mGLChannelFormat = GL_RED;
        mSwizzle = {GL_RED, GL_RED, GL_RED, GL_ONE};
        mUseSwizzle = true;

        for (int i = 0; i < mWidth * mHeight; i++) {
            m_stats.value().averageColor +=
                glm::vec4(data[i] / 255.0f, data[i] / 255.0f, data[i] / 255.0f, 1.0f);
        }
        m_stats.value().averageColor /= mWidth * mHeight;
        break;
    case STBI_grey_alpha:
        params.root.getWarningStream()
            << "Texture load cannot convert from gray_alpha format; red_green used!" << std::endl;
        mGLInternalFormat = GL_RG;
        mGLChannelFormat = GL_RG;
        mSwizzle = {GL_RED, GL_RED, GL_RED, GL_GREEN};
        mUseSwizzle = true;

        for (int i = 0; i < mWidth * mHeight; i += 2) {
            m_stats.value().averageColor += glm::vec4(data[i] / 255.0f, data[i] / 255.0f,
                                                      data[i] / 255.0f, data[i + 1] / 255.0f);
        }
        m_stats.value().averageColor /= mWidth * mHeight;

        break;
    case STBI_rgb:
        mGLInternalFormat = GL_RGB;
        mGLChannelFormat = GL_RGB;

        for (int i = 0; i < mWidth * mHeight; i += 3) {
            m_stats.value().averageColor +=
                glm::vec4(data[i] / 255.0f, data[i + 1] / 255.0f, data[i + 2] / 255.0f, 1.0f);
        }
        m_stats.value().averageColor /= mWidth * mHeight;
        break;
    case STBI_rgb_alpha:
        mGLInternalFormat = GL_RGBA;
        mGLChannelFormat = GL_RGBA;

        for (int i = 0; i < mWidth * mHeight; i += 4) {
            m_stats.value().averageColor += glm::vec4(data[i] / 255.0f, data[i + 1] / 255.0f,
                                                      data[i + 2] / 255.0f, data[i + 3] / 255.0f);
        }
        m_stats.value().averageColor /= mWidth * mHeight;
        break;
    }

    if (stbi_failure_reason()) {
        params.root.getErrStream() << "Texture load from '" << params.filepath
                                   << "' failed: " << stbi_failure_reason() << std::endl;
        loadToGPU(nullptr, params.filepath.filename().string());

        stbi_image_free(data);
    } else {
        params.root.getInfoStream() << "Texture '" << params.filepath << "' loaded." << std::endl;
        loadToGPU(data, params.filepath.filename().string());
        stbi_image_free(data);
    }
}

OpenGLTexture::OpenGLTexture(const EmptyParams &params)
    : OpenGLTexture(params.horWrap, params.verWrap, params.minFilter, params.magFilter,
                    params.useMipMaps, params.borderColor)
{
    m_stats.reset();

    mUseSwizzle = false;
    switch (params.channelType) {
    case ChannelType::GRAYSCALE:
        mGLInternalFormat = GL_RED;
        mGLChannelFormat = GL_RED;
        mSwizzle = {GL_RED, GL_RED, GL_RED, GL_ONE};
        mUseSwizzle = true;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::GRAYSCALE_ALPHA:
        mGLInternalFormat = GL_RG;
        mGLChannelFormat = GL_RG;
        mSwizzle = {GL_RED, GL_RED, GL_RED, GL_GREEN};
        mUseSwizzle = true;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::RGB:
        mGLInternalFormat = GL_RGB;
        mGLChannelFormat = GL_RGB;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::RGBA:
        mGLInternalFormat = GL_RGBA;
        mGLChannelFormat = GL_RGBA;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::DEPTH:
        mGLInternalFormat = GL_DEPTH_COMPONENT;
        mGLChannelFormat = GL_DEPTH_COMPONENT;
        mGLChannelType = GL_FLOAT;
        break;
    case ChannelType::SCALAR_SNORM8:
        mGLInternalFormat = GL_R8_SNORM;
        mGLChannelFormat = GL_RED;
        mGLChannelType = GL_BYTE;
        break;
    case ChannelType::VEC2_SNORM8:
        mGLInternalFormat = GL_RG8_SNORM;
        mGLChannelFormat = GL_RG;
        mGLChannelType = GL_BYTE;
        break;
    case ChannelType::VEC3_SNORM8:
        mGLInternalFormat = GL_RGB8_SNORM;
        mGLChannelFormat = GL_RGB;
        mGLChannelType = GL_BYTE;
        break;
    case ChannelType::VEC4_SNORM8:
        mGLInternalFormat = GL_RGBA8_SNORM;
        mGLChannelFormat = GL_RGBA;
        mGLChannelType = GL_BYTE;
        break;
    case ChannelType::SCALAR_UNORM8:
        mGLInternalFormat = GL_R8;
        mGLChannelFormat = GL_RED;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::VEC2_UNORM8:
        mGLInternalFormat = GL_RG8;
        mGLChannelFormat = GL_RG;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::VEC3_UNORM8:
        mGLInternalFormat = GL_RGB8;
        mGLChannelFormat = GL_RGB;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::VEC4_UNORM8:
        mGLInternalFormat = GL_RGBA8;
        mGLChannelFormat = GL_RGBA;
        mGLChannelType = GL_UNSIGNED_BYTE;
        break;
    case ChannelType::SCALAR_FLOAT16:
        mGLInternalFormat = GL_R16F;
        mGLChannelFormat = GL_RED;
        mGLChannelType = GL_HALF_FLOAT;
        break;
    case ChannelType::VEC2_FLOAT16:
        mGLInternalFormat = GL_RG16F;
        mGLChannelFormat = GL_RG;
        mGLChannelType = GL_HALF_FLOAT;
        break;
    case ChannelType::VEC3_FLOAT16:
        mGLInternalFormat = GL_RGB16F;
        mGLChannelFormat = GL_RGB;
        mGLChannelType = GL_HALF_FLOAT;
        break;
    case ChannelType::VEC4_FLOAT16:
        mGLInternalFormat = GL_RGBA16F;
        mGLChannelFormat = GL_RGBA;
        mGLChannelType = GL_HALF_FLOAT;
        break;
    case ChannelType::SCALAR_FLOAT32:
        mGLInternalFormat = GL_R32F;
        mGLChannelFormat = GL_RED;
        mGLChannelType = GL_FLOAT;
        break;
    case ChannelType::VEC2_FLOAT32:
        mGLInternalFormat = GL_RG32F;
        mGLChannelFormat = GL_RG;
        mGLChannelType = GL_FLOAT;
        break;
    case ChannelType::VEC3_FLOAT32:
        mGLInternalFormat = GL_RGB32F;
        mGLChannelFormat = GL_RGB;
        mGLChannelType = GL_FLOAT;
        break;
    case ChannelType::VEC4_FLOAT32:
        mGLInternalFormat = GL_RGBA32F;
        mGLChannelFormat = GL_RGBA;
        mGLChannelType = GL_FLOAT;
        break;
    }

    mWidth = params.size.x;
    mHeight = params.size.y;

    loadToGPU(nullptr, params.friendlyName);
}

OpenGLTexture::OpenGLTexture(const PureColorParams &params)
    : OpenGLTexture(WrappingType::REPEAT, WrappingType::REPEAT, FilterType::NEAREST,
                    FilterType::NEAREST, false, params.color)
{
    mUseSwizzle = false;
    mGLInternalFormat = GL_RGBA;
    mGLChannelFormat = GL_RGBA;
    mGLChannelType = GL_UNSIGNED_BYTE;

    mWidth = 1;
    mHeight = 1;

    unsigned char data[4] = {
        (unsigned char)(255.0 * params.color.r),
        (unsigned char)(255.0 * params.color.g),
        (unsigned char)(255.0 * params.color.b),
        (unsigned char)(255.0 * params.color.a),
    };

    loadToGPU(data, String("rgba(") + std::to_string((int)(255 * params.color.r)) + " " +
                        std::to_string((int)(255 * params.color.g)) + " " +
                        std::to_string((int)(255 * params.color.b)) + " " +
                        std::to_string((int)(255 * params.color.a)) + ")");

    m_stats = TextureStats{params.color};
}

OpenGLTexture::~OpenGLTexture()
{
    unloadFromGPU();
}

std::size_t OpenGLTexture::memory_cost() const
{
    return mWidth * mHeight * 3; /// TODO: properly calculate num channels
}

void OpenGLTexture::loadToGPU(const unsigned char *data, StringView friendlyName)
{
    if (!m_sentToGPU) {
        m_sentToGPU = true;
        glGenTextures(1, &glTextureId);
        glBindTexture(GL_TEXTURE_2D, glTextureId);

        glTexImage2D(GL_TEXTURE_2D, 0, mGLInternalFormat, mWidth, mHeight, 0, mGLChannelFormat,
                     mGLChannelType, data);

        if (mUseSwizzle) {
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mSwizzleArr);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mGLMagFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mGLMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mGLWrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mGLWrapT);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &mBorderColor[0]);

        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        String glLabel = String("texture ") + String(friendlyName);
        glObjectLabel(GL_TEXTURE, glTextureId, glLabel.size(), glLabel.data());
    }
}

void OpenGLTexture::unloadFromGPU()
{
    if (m_sentToGPU) {
        m_sentToGPU = false;
        glDeleteTextures(1, &glTextureId);
    }
}

} // namespace Vitrae