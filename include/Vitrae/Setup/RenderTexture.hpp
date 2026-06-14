#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Data/ClearColor.hpp"
#include "Vitrae/Data/RenderComponents.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"

namespace Vitrae
{

/**
 * Specification for using a texture as a render target, as bound to a FrameStore
 */
struct RenderTextureSpec
{
    dynasma::FirmPtr<Texture2DBase> p_texture;
    RenderComponent shaderComponent;
    ClearColor clearColor;

    RenderTextureSpec() = delete;
    RenderTextureSpec(RenderTextureSpec &&) = default;
    RenderTextureSpec(const RenderTextureSpec &) = default;
    RenderTextureSpec &operator=(RenderTextureSpec &&) = delete;
    RenderTextureSpec &operator=(const RenderTextureSpec &) = delete;

    /**
     * Sets p_texture and shaderComponent to compatible types depending on texture's PIXEL_TYPE
     * @param p_texture Converted to Texture2DBase
     * @param name The name for the ParamSpec of shaderComponent. typeInfo is set automatically
     */
    template <PixelType PIXEL_TYPE>
    RenderTextureSpec(dynasma::FirmPtr<Texture2D<PIXEL_TYPE>> p_texture, String componentName,
                      ClearColor clearColor = FixedClearColor::Default);

    /**
     * Sets p_texture=the texture and shaderComponent=FixedRenderComponent::DEPTH
     */
    RenderTextureSpec(dynasma::FirmPtr<Texture2D<PixelType::DEPTH>> p_texture,
                      ClearColor clearColor = FixedClearColor::Default);

    /**
     * Sets p_texture=the texture and shaderComponent=FixedRenderComponent::STENCIL
     */
    RenderTextureSpec(dynasma::FirmPtr<Texture2D<PixelType::STENCIL>> p_texture,
                      ClearColor clearColor = FixedClearColor::Default);

    /**
     * Sets p_texture=the texture and shaderComponent=FixedRenderComponent::DEPTH_AND_STENCIL
     */
    RenderTextureSpec(dynasma::FirmPtr<Texture2D<PixelType::DEPTH_AND_STENCIL>> p_texture,
                      ClearColor clearColor = FixedClearColor::Default);

    /**
     * Sets p_texture=the texture and shaderComponent=component
     * This is unsafe because it doesn't check if the texture's PIXEL_TYPE matches the component,
     * so put in its own factory function
     */
    static RenderTextureSpec fromUnsafe(dynasma::FirmPtr<Texture2DBase> p_texture,
                                        RenderComponent component, ClearColor clearColor);

  private:
    RenderTextureSpec(dynasma::FirmPtr<Texture2DBase> p_texture, RenderComponent shaderComponent,
                      ClearColor clearColor);
};

// ==== Implementation for templates ===============================================================

template <PixelType PIXEL_TYPE>
inline RenderTextureSpec::RenderTextureSpec(dynasma::FirmPtr<Texture2D<PIXEL_TYPE>> p_texture,
                                            String componentName, ClearColor clearColor)
    : p_texture{p_texture}, shaderComponent{ParamSpec{
                                .name = componentName,
                                .typeInfo = TYPE_INFO<PixelValueType<PIXEL_TYPE>>,
                            }},
      clearColor{clearColor}
{}

inline RenderTextureSpec::RenderTextureSpec(dynasma::FirmPtr<Texture2D<PixelType::DEPTH>> p_texture,
                                            ClearColor clearColor)
    : p_texture{p_texture}, shaderComponent{FixedRenderComponent::DEPTH}, clearColor{clearColor}
{}

inline RenderTextureSpec::RenderTextureSpec(
    dynasma::FirmPtr<Texture2D<PixelType::STENCIL>> p_texture, ClearColor clearColor)
    : p_texture{p_texture}, shaderComponent{FixedRenderComponent::STENCIL}, clearColor{clearColor}
{}

inline RenderTextureSpec::RenderTextureSpec(
    dynasma::FirmPtr<Texture2D<PixelType::DEPTH_AND_STENCIL>> p_texture, ClearColor clearColor)
    : p_texture{p_texture}, shaderComponent{FixedRenderComponent::DEPTH_AND_STENCIL},
      clearColor{clearColor}
{}

inline RenderTextureSpec RenderTextureSpec::fromUnsafe(dynasma::FirmPtr<Texture2DBase> p_texture,
                                                       RenderComponent component,
                                                       ClearColor clearColor)
{
    return {p_texture, component, clearColor};
}

inline RenderTextureSpec::RenderTextureSpec(dynasma::FirmPtr<Texture2DBase> p_texture,
                                            RenderComponent shaderComponent, ClearColor clearColor)
    : p_texture{p_texture}, shaderComponent{shaderComponent}, clearColor{clearColor}
{}
} // namespace Vitrae