#pragma once

#include "Vitrae/Data/ClearColor.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Setup/RenderTexture.hpp"

#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <span>

namespace Vitrae
{
class ComponentRoot;
class Texture;
class ParamList;

/**
 * A FrameStore is a target for rendering, binding pixel output and processing data together
 */
class FrameStore : public dynasma::PolymorphicBase
{
  public:
    struct TextureBindParams
    {
        ComponentRoot &root;
        std::vector<RenderTextureSpec> renderTextureSpecs;
        String friendlyName = "";
    };
    struct WindowDisplayParams
    {
        ComponentRoot &root;

        std::size_t width, height;
        String title;
        bool isFullscreen;

        ClearColor clearColor = FixedClearColor::Default;
        ClearColor clearDepth = FixedClearColor::Default;

        std::function<void()> onClose;
        std::function<void(glm::vec2 motion, bool bLeft, bool bRight, bool bMiddle)> onDrag;
    };

    virtual ~FrameStore() = default;
    virtual std::size_t memory_cost() const = 0;

    virtual void resize(glm::vec2 size) = 0;
    virtual void bindOutput(const RenderTextureSpec &spec) = 0;

    virtual glm::uvec2 getSize() const = 0;
    virtual dynasma::FirmPtr<const ParamList> getRenderComponents() const = 0;
    virtual std::span<const RenderTextureSpec> getRenderTextureSpecs() const = 0;

    virtual void sync(bool vsync) = 0;
};

struct FrameStoreSeed
{
    using Asset = FrameStore;

    inline std::size_t load_cost() const { return 1; }

    std::variant<FrameStore::TextureBindParams, FrameStore::WindowDisplayParams> kernel;
};

using FrameStoreManager = dynasma::AbstractManager<FrameStoreSeed>;

} // namespace Vitrae