#include "Vitrae/Visuals/Compositor.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"

#include "MMeter.h"

#include <fstream>
#include <ranges>

namespace Vitrae
{
class Renderer;

const PropertySpec Compositor::FRAME_STORE_TARGET_SPEC = {
    .name = "fs_target",
    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>(),

};

Compositor::Compositor(ComponentRoot &root)
    : m_root(root), m_needsRebuild(true), m_needsFrameStoreRegeneration(true), m_pipeline(),
      m_localProperties(&parameters)
{}
std::size_t Compositor::memory_cost() const
{
    /// TODO: implement
    return sizeof(Compositor);
}

void Compositor::setPropertyAliases(const PropertyAliases &aliases)
{
    m_aliases = aliases;

    m_needsRebuild = true;
}

void Compositor::setDesiredProperties(const PropertyList &properties)
{
    m_desiredProperties = properties;

    m_needsRebuild = true;
}

void Compositor::compose()
{
    MMETER_SCOPE_PROFILER("Compositor::compose");

    // ScopedDict localVars(&parameters);

    // setup the rendering context
    ArgumentScope scope(&m_localProperties, &m_aliases);
    RenderComposeContext context{
        .properties = scope,
        .root = m_root,
        .aliases = m_aliases,
    };

    bool tryExecute = true;

    while (tryExecute) {
        MMETER_SCOPE_PROFILER("Execution attempt");

        tryExecute = false;

        // rebuild the pipeline if needed
        while (m_needsRebuild || m_needsFrameStoreRegeneration) {
            if (m_needsRebuild) {
                rebuildPipeline();
            }
            if (m_needsFrameStoreRegeneration) {
                regenerateFrameStores();
            }
        }

        try {
            // execute the pipeline
            {
                MMETER_SCOPE_PROFILER("Pipeline execution");

                for (auto p_task : m_pipeline.items) {
                    p_task->run(context);
                }
            }

            // sync the final framebuffer
            {
                MMETER_SCOPE_PROFILER("FrameStore sync");

                parameters.get(FRAME_STORE_TARGET_SPEC.name)
                    .get<dynasma::FirmPtr<FrameStore>>()
                    ->sync();
            }
        }
        catch (ComposeTaskRequirementsChangedException) {
            m_needsRebuild = true;
            tryExecute = true;
        }
    }
}

void Compositor::rebuildPipeline()
{
    MMETER_SCOPE_PROFILER("Compositor::rebuildPipeline");

    m_needsRebuild = false;

    // setup the rendering context
    ArgumentScope scope(&m_localProperties, &m_aliases);
    RenderComposeContext context{
        .properties = scope,
        .root = m_root,
        .aliases = m_aliases,
    };

    m_pipeline = Pipeline<ComposeTask>(m_root.getComponent<MethodCollection>().getComposeMethod(),
                                       m_desiredProperties, m_aliases);

    m_localProperties.clear();

    String filePrefix = std::string("shaderdebug/") + "compositor_" + getPipelineId(m_pipeline);
    {
        std::ofstream file;
        String filename = filePrefix + ".dot";
        file.open(filename);
        exportPipeline(m_pipeline, m_aliases, file);
        file.close();

        m_root.getInfoStream() << "Compositor graph stored to: '" << std::filesystem::current_path()
                               << "/" << filename << "'" << std::endl;
    }

    m_needsFrameStoreRegeneration = true;
}

void Compositor::regenerateFrameStores()
{
    MMETER_SCOPE_PROFILER("Compositor::regenerateFrameStores");

    m_needsFrameStoreRegeneration = false;

    // setup the rendering context
    ArgumentScope scope(&m_localProperties, &m_aliases);
    RenderComposeContext context{
        .properties = scope,
        .root = m_root,
        .aliases = m_aliases,
    };

    // process
    try {
        for (auto p_task : std::ranges::reverse_view{m_pipeline.items}) {
            p_task->prepareRequiredLocalAssets(context);
        }
    }
    catch (ComposeTaskRequirementsChangedException) {
        m_needsRebuild = true;
    }

    String filePrefix = std::string("shaderdebug/") + "full_" + getPipelineId(m_pipeline);
    {
        std::ofstream file;
        String filename = filePrefix + ".dot";
        file.open(filename);
        exportPipeline(m_pipeline, m_aliases, file, "", true, true);
        file.close();

        m_root.getInfoStream() << "Compositor graph stored to: '" << std::filesystem::current_path()
                               << "/" << filename << "'" << std::endl;
    }
}

} // namespace Vitrae