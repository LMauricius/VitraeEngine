#include "Vitrae/Visuals/Compositor.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"

#include "MMeter.h"

#include <fstream>
#include <ranges>

namespace Vitrae
{
class Renderer;
Compositor::Compositor(ComponentRoot &root)
    : m_root(root), m_needsRebuild(true), m_needsFrameStoreRegeneration(true), m_pipeline(),
      m_localProperties(&parameters),
      mp_composeMethod(dynasma::makeStandalone<Method<ComposeTask>>()),
      m_defaultVertexMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      m_defaultFragmentMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      m_defaultComputeMethod(dynasma::makeStandalone<Method<ShaderTask>>())
{}
std::size_t Compositor::memory_cost() const
{
    /// TODO: implement
    return sizeof(Compositor);
}

void Compositor::setComposeMethod(dynasma::FirmPtr<Method<ComposeTask>> p_method)
{
    mp_composeMethod = p_method;
    m_needsRebuild = true;
}

void Compositor::setDefaultShadingMethod(dynasma::FirmPtr<Method<ShaderTask>> p_vertexMethod,
                                         dynasma::FirmPtr<Method<ShaderTask>> p_fragmentMethod)
{
    m_defaultVertexMethod = p_vertexMethod;
    m_defaultFragmentMethod = p_fragmentMethod;
}

void Compositor::setDefaultComputeMethod(dynasma::FirmPtr<Method<ShaderTask>> p_method)
{
    m_defaultComputeMethod = p_method;
}

void Compositor::setOutput(dynasma::FirmPtr<FrameStore> p_store)
{
    mp_frameStore = p_store;

    m_needsFrameStoreRegeneration = true;
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

    // set the output frame property
    for (auto desiredNameId : m_desiredProperties.getSpecNameIds()) {
        parameters.set(desiredNameId, mp_frameStore);
    }

    // setup the rendering context
    Renderer &rend = m_root.getComponent<Renderer>();
    RenderRunContext context{.properties = m_localProperties,
                             .renderer = rend,
                             .methodCombinator = m_shadingMethodCombinator,
                             .p_defaultVertexMethod = m_defaultVertexMethod,
                             .p_defaultFragmentMethod = m_defaultFragmentMethod,
                             .p_defaultComputeMethod = m_defaultComputeMethod,
                             .preparedCompositorFrameStores = m_preparedFrameStores,
                             .preparedCompositorTextures = m_preparedTextures};

    bool tryExecute = true;

    while (tryExecute) {
        MMETER_SCOPE_PROFILER("Execution attempt");

        tryExecute = false;

        // rebuild the pipeline if needed
        if (m_needsRebuild) {
            rebuildPipeline();
        }
        if (m_needsFrameStoreRegeneration) {
            regenerateFrameStores();
        }

        try {
            // execute the pipeline
            {
                MMETER_SCOPE_PROFILER("Pipeline execution");

                for (auto &pipeitem : m_pipeline.items) {
                    pipeitem.p_task->run(context);
                }
            }

            // sync the framebuffers
            {
                MMETER_SCOPE_PROFILER("FrameStore sync");

                for (auto p_store : m_uniqueFrameStores) {
                    p_store->sync();
                }
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

    RenderSetupContext context{
        .renderer = m_root.getComponent<Renderer>(),
        .p_defaultVertexMethod = m_defaultVertexMethod,
        .p_defaultFragmentMethod = m_defaultFragmentMethod,
        .p_defaultComputeMethod = m_defaultComputeMethod,
    };

    m_pipeline =
        Pipeline<ComposeTask>(mp_composeMethod, m_desiredProperties.getSpecList(), context);

    m_localProperties.clear();

    String filePrefix =
        std::string("shaderdebug/") + String(mp_composeMethod->getFriendlyName()) + "_compositor";
    {
        std::ofstream file;
        String filename = filePrefix + ".dot";
        file.open(filename);
        exportPipeline(m_pipeline, file);
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

    // clear the buffers (except for the final output)
    m_preparedFrameStores.clear();
    m_uniqueFrameStores.clear();
    m_preparedTextures.clear();
    for (auto desiredNameId : m_desiredProperties.getSpecNameIds()) {
        m_preparedFrameStores[desiredNameId] = mp_frameStore;
    }

    // fill the buffers
    for (auto &pipeitem : std::ranges::reverse_view{m_pipeline.items}) {
        pipeitem.p_task->prepareRequiredLocalAssets(m_preparedFrameStores, m_preparedTextures,
                                                    parameters);
    }

    // build list of unique framestore
    for (auto [nameId, p_store] : m_preparedFrameStores) {
        m_uniqueFrameStores.insert(p_store);
    }
}

} // namespace Vitrae