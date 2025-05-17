#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Params/ArgumentGetter.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Data/RenderComponents.hpp"
#include "Vitrae/Data/ClearColor.hpp"

#include "dynasma/keepers/abstract.hpp"

#include "glm/glm.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Shares one render component from input FrameStore to filtered FrameStore.
 * If the input isn't modified later, this is like copying the component over the filtered FrameStore
 */
class ComposeFrameToFrame : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        String inputFrameStoreName;
        String targetFrameStoreName;
        RenderComponent shaderComponent;
        std::vector<String> outputTokenNames;
    };

    ComposeFrameToFrame(const SetupParams &params);
    ~ComposeFrameToFrame() = default;

    std::size_t memory_cost() const override;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const ParamAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const ParamAliases &aliases) const override;

    void run(RenderComposeContext args) const override;
    void prepareRequiredLocalAssets(RenderComposeContext args) const override;

    StringView getFriendlyName() const override;

  protected:
    SetupParams m_params;
    ParamList m_inputSpecs;
    ParamList m_filterSpecs;
    ParamList m_outputSpecs;

    String m_friendlyName;
};

struct ComposeFrameToFrameKeeperSeed
{
    using Asset = ComposeFrameToFrame;
    std::variant<ComposeFrameToFrame::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeFrameToFrameKeeper = dynasma::AbstractKeeper<ComposeFrameToFrameKeeperSeed>;

} // namespace Vitrae