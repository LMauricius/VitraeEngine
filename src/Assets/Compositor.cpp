#include "Vitrae/Assets/Compositor.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Pipelines/PipelineContainer.hpp"

#include "MMeter.h"

#include <fstream>
#include <ranges>

namespace Vitrae
{
class Renderer;

Compositor::Compositor(ComponentRoot &root)
    : m_root(root), m_needsRebuild(true), m_needsFrameStoreRegeneration(true), m_pipeline(),
      m_localProperties(&parameters)
{}
std::size_t Compositor::memory_cost() const
{
    /// TODO: implement
    return sizeof(Compositor);
}

void Compositor::setParamAliases(const ParamAliases &aliases)
{
    m_aliases = aliases;

    m_needsRebuild = true;
}

void Compositor::setDesiredProperties(const ParamList &properties)
{
    m_desiredProperties = properties;

    m_needsRebuild = true;
}

const ParamList &Compositor::getInputSpecs() const
{
    return m_pipeline.inputSpecs;
}

void Compositor::compose()
{
    MMETER_SCOPE_PROFILER("Compositor::compose");

    // VariantScope localVars(&parameters);

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

                parameters.get(StandardParam::fs_display.name)
                    .get<dynasma::FirmPtr<FrameStore>>()
                    ->sync(parameters.get(StandardParam::vsync.name).get<bool>());
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

    // erase previous pipeline
    for (auto p_task : m_pipeline.items) {
        if (const PipelineContainer<ComposeTask> *p_container =
                dynamic_cast<const PipelineContainer<ComposeTask> *>(&*p_task);
            p_container) {
            p_container->rebuildContainedPipeline(m_aliases);
        }
    }

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

    String filePrefix =
        std::string("shaderdebug/") + "compositor_" + getPipelineId(m_pipeline, m_aliases);
    {
        std::ofstream file;
        String filename = filePrefix + ".dot";
        file.open(filename);
        exportPipeline(m_pipeline, m_aliases, file);
        file.close();

        m_root.getInfoStream() << "Compositor graph stored to: '"
                               << (std::filesystem::current_path() / filename) << "'" << std::endl;
    }

    // add compositor properties, so they are visible from the outside
    m_pipeline.inputSpecs.insert_back(StandardParam::vsync);

    // Set default values
    for (auto p_specs :
         {&m_pipeline.inputSpecs, &m_pipeline.filterSpecs, &m_pipeline.consumingSpecs}) {
        for (auto &spec : p_specs->getSpecList()) {
            if (spec.defaultValue.getAssignedTypeInfo() != TYPE_INFO<void> &&
                !parameters.has(spec.name)) {
                parameters.set(spec.name, spec.defaultValue);
            }
        }
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

        String filePrefix =
            std::string("shaderdebug/") + "full_" + getPipelineId(m_pipeline, m_aliases);
        {
            std::ofstream file;
            String filename = filePrefix + ".dot";
            file.open(filename);
            exportPipeline(m_pipeline, m_aliases, file, "", true, true);
            file.close();

            m_root.getInfoStream()
                << "Compositor graph stored to: '" << (std::filesystem::current_path() / filename)
                << "'" << std::endl;
        }
    }
    catch (ComposeTaskRequirementsChangedException) {
        m_needsRebuild = true;
    }
}

} // namespace Vitrae