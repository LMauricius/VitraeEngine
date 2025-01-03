#pragma once

#include "Vitrae/Types/Typedefs.hpp"
#include "Vitrae/Util/PropertySpec.hpp"

#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include "glm/glm.hpp"

#include <optional>

namespace Vitrae
{
class ComponentRoot;
class Texture;
class PropertyList;

/**
 * A FrameStore is a single image-like resource
 */
class FrameStore : public dynasma::PolymorphicBase
{
  public:
    struct OutputTextureSpec
    {
        PropertySpec fragmentSpec;
        dynasma::FirmPtr<Texture> p_texture;
    };

    struct TextureBindParams
    {
        ComponentRoot &root;
        std::optional<dynasma::FirmPtr<Texture>> p_colorTexture;
        std::optional<dynasma::FirmPtr<Texture>> p_depthTexture;
        std::vector<OutputTextureSpec> outputTextureSpecs;
        String friendlyName = "";
    };
    struct WindowDisplayParams
    {
        ComponentRoot &root;

        std::size_t width, height;
        String title;
        bool isFullscreen;

        std::function<void()> onClose;
        std::function<void(glm::vec2 motion, bool bLeft, bool bRight, bool bMiddle)> onDrag;
    };

    virtual ~FrameStore() = default;
    virtual std::size_t memory_cost() const = 0;
    virtual void resize(glm::vec2 size) = 0;

    virtual glm::vec2 getSize() const = 0;
    virtual dynasma::FirmPtr<const PropertyList> getRenderComponents() const = 0;

    virtual void sync() = 0;
};

struct FrameStoreSeed
{
    using Asset = FrameStore;

    inline std::size_t load_cost() const { return 1; }

    std::variant<FrameStore::TextureBindParams, FrameStore::WindowDisplayParams> kernel;
};

using FrameStoreManager = dynasma::AbstractManager<FrameStoreSeed>;

} // namespace Vitrae