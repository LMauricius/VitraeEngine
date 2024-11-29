#pragma once

#include "Vitrae/Pipelines/Method.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

namespace Vitrae
{
class Renderer;
class FrameStore;
class Texture;
class ComponentRoot;

struct RenderSetupContext;
struct RenderRunContext;

/*
Common exceptions of compose tasks
*/

/**
 * Thrown if an error occurs during the execution of a compose task
 */
class ComposeTaskException
{};

/**
 * Thrown if the requirements of the task have been changed while running and the pipeline needs to
 * be rebuilt
 */
class ComposeTaskRequirementsChangedException : public ComposeTaskException
{};

class ComposeTask : public Task
{
  protected:
  public:
    using InputSpecsDeducingContext = RenderSetupContext;

    using Task::Task;

    /**
     * Execute the task
     * @throws ComposeTaskRequirementsChangedException if the pipeline needs to be rebuilt
     */
    virtual void run(RenderRunContext args) const = 0;
    virtual void prepareRequiredLocalAssets(
        StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
        StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
        const ScopedDict &properties,
        const RenderSetupContext &context) const = 0;

    /// TODO: implement this and move to sources

    inline std::size_t memory_cost() const override { return 1; }

    inline virtual const StableMap<StringId, PropertySpec> &getInputSpecs(
        const RenderSetupContext &args) const
    {
        return m_inputSpecs;
    }
    inline virtual const StableMap<StringId, PropertySpec> &getOutputSpecs() const
    {
        return m_outputSpecs;
    }

    inline void extractUsedTypes(std::set<const TypeInfo *> &typeSet) const override
    {
        for (auto &specs : {m_inputSpecs, m_outputSpecs}) {
            for (auto [nameId, spec] : specs) {
                typeSet.insert(&spec.typeInfo);
            }
        }
    }
    inline void extractSubTasks(std::set<const Task *> &taskSet) const override
    {
        taskSet.insert(this);
    }
};

struct RenderSetupContext {
    Renderer &renderer;
    dynasma::FirmPtr<Method<ComposeTask>> p_composeMethod;
    dynasma::FirmPtr<Method<ShaderTask>> p_defaultVertexMethod,
        p_defaultFragmentMethod, p_defaultComputeMethod;
};

struct RenderRunContext {
    ScopedDict &properties;
    Renderer &renderer;
    MethodCombinator<ShaderTask> &methodCombinator;
    dynasma::FirmPtr<Method<ComposeTask>> p_composeMethod;
    dynasma::FirmPtr<Method<ShaderTask>> p_defaultVertexMethod,
        p_defaultFragmentMethod, p_defaultComputeMethod;
    const StableMap<StringId, dynasma::FirmPtr<FrameStore>>
        &preparedCompositorFrameStores;
    const StableMap<StringId, dynasma::FirmPtr<Texture>>
        &preparedCompositorTextures;
};

namespace StandardCompositorOutputNames
{
static constexpr const char OUTPUT[] = "rendered_scene";
} // namespace StandardComposeOutputNames

namespace StandardCompositorOutputTypes
{
static constexpr const TypeInfo &OUTPUT_TYPE = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>();
}

} // namespace Vitrae