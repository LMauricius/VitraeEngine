#pragma once

#include "Vitrae/Pipelines/Method.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Util/ArgumentScope.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

namespace Vitrae
{
class Renderer;
class FrameStore;
class Texture;
class ComponentRoot;

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

struct RenderComposeContext
{
    ArgumentScope &properties;
    ComponentRoot &root;
    const PropertyAliases &aliases;
};

class ComposeTask : public Task
{
  protected:
  public:
    /**
     * Prepare local assets required by the task. Ran from last task to first
     */
    virtual void prepareRequiredLocalAssets(RenderComposeContext ctx) const = 0;

    /**
     * Execute the task
     * @throws ComposeTaskRequirementsChangedException if the pipeline needs to be rebuilt
     */
    virtual void run(RenderComposeContext ctx) const = 0;
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