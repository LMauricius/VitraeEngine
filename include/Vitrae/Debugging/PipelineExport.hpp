#pragma once

#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Pipelines/PipelineContainer.hpp"
#include "Vitrae/TypeConversion/StringConvert.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <set>

namespace Vitrae
{

/**
 * @returns The unique identifier for the pipeline dependant for relevant aliases
 */
template <TaskChild BasicTask> inline String getPipelineId(const Pipeline<BasicTask> &pipeline)
{
    String ret;

    // List of outputs
    for (const PropertyList &specs : {pipeline.outputSpecs, pipeline.filterSpecs}) {
        for (auto &spec : specs.getSpecList()) {
            String chosenName = pipeline.usedSelection.choiceStringFor(spec.name);
            ret += "_";
            if (spec.name == chosenName) {
                ret += spec.name;
            } else {
                ret += spec.name + "-" + chosenName;
            }
        }
    }

    // Some short hash for making it clear
    ret = std::to_string(std::hash<String>{}(ret) & 0xffffff) + ret;

    return ret;
}

/**
 * @brief Exports a graph depicting the pipeline in the dot language
 * @param pipeline The pipeline to export
 * @param out The output stream to which to write
 */
template <TaskChild BasicTask>
void exportPipeline(const Pipeline<BasicTask> &pipeline, const PropertyAliases &aliases,
                    std::ostream &out, StringView prefix = "", bool isMainGraph = true,
                    bool expandSubGraphs = false)
{
    using Pipeline = Pipeline<BasicTask>;
    using PipelineContainer = PipelineContainer<BasicTask>;

    std::stringstream connectionsSS;

    /*
    This lists all subpipelines, by name
    */
    std::unordered_map<String, const Pipeline *> subpipelines;

    auto colorFromHash = [&](std::size_t hash) {
        hash = (hash & 0xffffff) ^ ((hash >> 24) & 0xffffff) ^ (hash >> 48);
        int r = hash & 0xff;
        int g = (hash >> 8) & 0xff;
        int b = (hash >> 16) & 0xff;
        // add a blueish tint
        return String("#") + toHexString(127 + r / 2, 2) + toHexString(160 + g / 3, 2) +
               toHexString(192 + b / 4, 2);
    };

    auto colorFromName = [&](StringView name) {
        return colorFromHash(std::hash<StringId>{}(name));
    };

    auto escapedLabel = [&](StringView label) -> String {
        String ret = searchAndReplace(String(label), "\"", "\\\"");
        ret = searchAndReplace(ret, "\n", "\\n");
        return ret;
    };
    auto getPropId = [&](const PropertySpec &spec) -> String {
        String realName = aliases.choiceStringFor(spec.name);
        return String("Prop_") + realName;
    };
    auto getTaskId = [&](const BasicTask &task) -> String {
        return String("Task_") + std::to_string((std::size_t)&task);
    };
    auto outputTaskNode = [&](StringView id, std::size_t ord, const BasicTask &task) {
        out << prefix << id << " [";
        out << "label=\""
            << escapedLabel(String("#") + std::to_string(ord) + "\\n" +
                            String(task.getFriendlyName()))
            << "\", ";
        out << "group=\"Tasks\", ";
        out << "shape=box, ";
        out << "style=\"filled\", ";
        if (const PipelineContainer *p_container = dynamic_cast<const PipelineContainer *>(&task);
            expandSubGraphs && p_container) {
            out << "fillcolor=\"" + colorFromName(task.getFriendlyName()) + "\", ";
            out << "bgcolor=\"" + colorFromName(task.getFriendlyName()) + "\", ";
            subpipelines.emplace(String(task.getFriendlyName()),
                                 &p_container->getContainedPipeline(aliases));
        } else {
            out << "fillcolor=\"lightblue\", ";
            out << "bgcolor=\"lightblue\", ";
        }
        out << "];\n";
    };
    auto outputPropNode = [&](StringView id, const PropertySpec &spec, bool horizontal) {
        out << prefix << id << " [";
        if (aliases.choiceStringFor(spec.name) != spec.name)
            out << "label=\"" << spec.name << "\n("
                << escapedLabel(aliases.choiceStringFor(spec.name)) << ")";
        else
            out << "label=\"" << escapedLabel(spec.name);
        if (spec.typeInfo == Variant::getTypeInfo<void>()) {
            out << "\", ";
            out << "shape=octagon, ";
            out << "style=\"rounded,filled\", ";
            out << "fillcolor=\"lightgoldenrod\", ";
            out << "bgcolor=\"lightgoldenrod\", ";
        } else {
            if (horizontal) {
                out << ": " << escapedLabel(spec.typeInfo.getShortTypeName()) << "\", ";
            } else {
                out << "\\n: " << escapedLabel(spec.typeInfo.getShortTypeName()) << "\", ";
            }
            out << "shape=box, ";
            out << "style=\"rounded,filled\", ";
            out << "fillcolor=\"lightyellow\", ";
            out << "bgcolor=\"lightyellow\", ";
        }
        out << "];\n";
    };
    auto outputInvisNode = [&](StringView id) {
        out << prefix << id << " [";
        out << "style=\"invis\", ";
        out << "shape=\"point\", ";
        out << "];\n";
    };
    auto sameRank = [&](StringView from, StringView to) {
        out << "{label=\"\";style=invis;rank=same; " << prefix << from << "; " << prefix << to
            << "}\n";
    };
    auto outputUsage = [&](StringView from, StringView to, bool changeRank) {
        connectionsSS << prefix << from << " -> " << prefix << to;
        connectionsSS << " [";
        if (!changeRank) {
            connectionsSS << "minlen=0,";
        }
        connectionsSS << "dir=none,";
        connectionsSS << "color=darkblue";
        connectionsSS << "] ";
        connectionsSS << ";\n";
    };
    auto outputConsumption = [&](StringView from, StringView to, bool changeRank) {
        connectionsSS << prefix << from << " -> " << prefix << to;
        connectionsSS << " [";
        if (!changeRank) {
            connectionsSS << "minlen=0,";
        }
        connectionsSS << "dir=forward,";
        connectionsSS << "color=red";
        connectionsSS << "] ";
        connectionsSS << ";\n";
    };
    auto outputGeneration = [&](StringView from, StringView to, bool changeRank) {
        connectionsSS << prefix << from << " -> " << prefix << to;
        connectionsSS << " [";
        if (!changeRank) {
            connectionsSS << "minlen=0,";
        }
        connectionsSS << "dir=forward,";
        connectionsSS << "color=forestgreen";
        connectionsSS << "] ";
        connectionsSS << ";\n";
    };
    auto outputModification = [&](StringView from, StringView to, bool changeRank) {
        connectionsSS << prefix << from << " -> " << prefix << to;
        connectionsSS << " [";
        if (!changeRank) {
            connectionsSS << "minlen=0,";
        }
        connectionsSS << "dir=both,";
        connectionsSS << "color=fuchsia";
        connectionsSS << "] ";
        connectionsSS << ";\n";
    };
    auto outputEquivalence = [&](StringView from, StringView to, bool changeRank,
                                 StringView lhead = "", StringView ltail = "") {
        connectionsSS << prefix << from << " -> " << prefix << to;
        connectionsSS << "[style=dashed";
        if (lhead.length()) {
            connectionsSS << ", lhead=" << lhead;
        }
        if (ltail.length()) {
            connectionsSS << ", ltail=" << ltail;
        }
        if (!changeRank) {
            connectionsSS << ", minlen=0";
        }
        connectionsSS << ", dir=none";
        connectionsSS << "];\n";
    };
    auto outputInvisibleDirection = [&](StringView from, StringView to, bool changeRank,
                                        StringView lhead = "", StringView ltail = "") {
        connectionsSS << prefix << from << " -> " << prefix << to;
        if (lhead.length()) {
            connectionsSS << ", lhead=" << lhead;
        }
        if (ltail.length()) {
            connectionsSS << ", ltail=" << ltail;
        }
        if (!changeRank) {
            connectionsSS << ", minlen=0";
        }
        connectionsSS << "];\n";
    };

    /*
    This references the nodes that are currently active, i.e. added to the graph and not consumed
    */
    std::map<StringId, String> activeNodeIdsPerNameId;

    /*
    This references nodes for whichwe know their IDs, but weren't added to the graph yet
    */
    std::map<StringId, String> pendingNodeIdsPerNameId;

    /*
    Counts the number of times we re-added a node for the same property
    */
    std::map<StringId, std::size_t> repetitionCounterPerNameId;

    bool horizontalInputsOutputs = pipeline.inputSpecs.count() > pipeline.items.size() ||
                                   pipeline.outputSpecs.count() > pipeline.items.size();

    auto currentPropertyNodeReference = [&](const PropertySpec &spec) {
        String realName = aliases.choiceStringFor(spec.name);

        if (auto it = activeNodeIdsPerNameId.find(realName); it != activeNodeIdsPerNameId.end()) {
            return it->second;
        } else if (auto it = pendingNodeIdsPerNameId.find(realName);
                   it != pendingNodeIdsPerNameId.end()) {
            return it->second;
        } else {
            String id = getPropId(spec);
            if (auto it = repetitionCounterPerNameId.find(realName);
                it != repetitionCounterPerNameId.end()) {
                ++it->second;
                id += std::to_string(it->second);
            } else {
                repetitionCounterPerNameId[realName] = 1;
            }

            pendingNodeIdsPerNameId[realName] = id;
            return id;
        }
    };

    auto consumePropertyNode = [&](const PropertySpec &spec) {
        String realName = aliases.choiceStringFor(spec.name);

        if (auto it = activeNodeIdsPerNameId.find(realName); it != activeNodeIdsPerNameId.end()) {
            String id = it->second;
            activeNodeIdsPerNameId.erase(it);
            return id;
        } else {
            throw std::runtime_error("consumePropertyNode: property " + std::string(realName) +
                                     " has no active");
        }
    };

    auto addPropertyNodeBefore = [&](const PropertySpec &spec) {
        String realName = aliases.choiceStringFor(spec.name);

        if (auto it = activeNodeIdsPerNameId.find(realName); it == activeNodeIdsPerNameId.end()) {
            String id;
            if (auto it = pendingNodeIdsPerNameId.find(realName);
                it != pendingNodeIdsPerNameId.end()) {
                id = it->second;
                pendingNodeIdsPerNameId.erase(it);
            } else {
                id = currentPropertyNodeReference(spec);
            }

            activeNodeIdsPerNameId.emplace(realName, id);
            outputPropNode(id, spec, horizontalInputsOutputs);
        }
    };

    auto addPropertyNodeHere = [&](const PropertySpec &spec) {
        String realName = aliases.choiceStringFor(spec.name);

        if (auto it = activeNodeIdsPerNameId.find(realName); it != activeNodeIdsPerNameId.end()) {
            String prevId = it->second;
            consumePropertyNode(spec);
            String newId = currentPropertyNodeReference(spec);
            pendingNodeIdsPerNameId.erase(realName);
            activeNodeIdsPerNameId[realName] = newId;
            outputPropNode(newId, spec, horizontalInputsOutputs);
            outputEquivalence(prevId, newId, true);
        } else {
            String id;
            if (auto it = pendingNodeIdsPerNameId.find(realName);
                it != pendingNodeIdsPerNameId.end()) {
                id = it->second;
                pendingNodeIdsPerNameId.erase(it);
            } else {
                id = currentPropertyNodeReference(spec);
                pendingNodeIdsPerNameId.erase(realName);
            }

            activeNodeIdsPerNameId.emplace(realName, id);
            outputPropNode(id, spec, horizontalInputsOutputs);
        }
    };

    /*
    Output
    */

    if (isMainGraph) {
        out << "digraph {\n";
        out << "\trankdir=\"LR\"\n";
        out << "\tranksep=0.25\n";
        out << "\tnodesep=0.13\n";
        out << "\tcompound=true;\n";
    }

    // inputs
    out << "\tsubgraph cluster_" << prefix << "inputs {\n";
    out << "\t\tlabel=\"Inputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";
    out << "\t\tcolor=\"black\";\n";

    for (const PropertyList *p_specs : {&pipeline.inputSpecs, &pipeline.filterSpecs,
                                        &pipeline.consumingSpecs, &pipeline.pipethroughSpecs}) {
        for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
            out << "\t\t";
            addPropertyNodeHere(spec);
        }
    }
    out << "\t}\n";

    out << "\tsubgraph cluster_" << prefix << "processing {\n";
    out << "\t\tlabel=\"Tasks\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";
    out << "\t\tcolor=\"black\";\n";

    // tasks
    std::size_t ordinalI = 0;
    for (auto p_task : pipeline.items) {
        ++ordinalI;
        out << "\t\t";

        for (auto [nameId, spec] : p_task->getInputSpecs(aliases).getMappedSpecs()) {
            out << "\t";
            addPropertyNodeBefore(spec);
            outputUsage(currentPropertyNodeReference(spec), getTaskId(*p_task), true);
        }

        for (auto [nameId, spec] : p_task->getConsumingSpecs(aliases).getMappedSpecs()) {
            out << "\t";
            addPropertyNodeBefore(spec);
            String propNodeId = currentPropertyNodeReference(spec);
            consumePropertyNode(spec);
            outputConsumption(propNodeId, getTaskId(*p_task), true);
        }

        for (auto [nameId, spec] : p_task->getFilterSpecs(aliases).getMappedSpecs()) {
            out << "\t";
            addPropertyNodeHere(spec);
            String curPropNodeId = currentPropertyNodeReference(spec);

            outputModification(curPropNodeId, getTaskId(*p_task), false);
            sameRank(getTaskId(*p_task), curPropNodeId);
        }

        for (auto [nameId, spec] : p_task->getOutputSpecs().getMappedSpecs()) {
            out << "\t";
            if (auto it = activeNodeIdsPerNameId.find(spec.name);
                it != activeNodeIdsPerNameId.end()) {
                activeNodeIdsPerNameId.erase(it);
            }
            outputGeneration(getTaskId(*p_task), currentPropertyNodeReference(spec), true);
        }
        outputTaskNode(getTaskId(*p_task), ordinalI, *p_task);

        // unused local properties
        for (auto [nameId, spec] : pipeline.localSpecs.getMappedSpecs()) {
            addPropertyNodeBefore(spec);
        }
    }

    out << "\t}\n";

    // outputs
    out << "\tsubgraph cluster_" << prefix << "outputs {\n";
    out << "\t\tlabel=\"Outputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";
    out << "\t\tcolor=\"black\";\n";

    for (auto [nameId, spec] : pipeline.outputSpecs.getMappedSpecs()) {
        out << "\t\t";
        addPropertyNodeHere(spec);
    }
    for (auto [nameId, spec] : pipeline.pipethroughSpecs.getMappedSpecs()) {
        out << "\t\t";
        addPropertyNodeHere(spec);
    }

    out << "\t}\n";

    out << connectionsSS.str() << "\n";

    // subpipelines
    for (auto [name, p] : subpipelines) {
        String newPref = String(prefix) + "sub" + std::to_string((std::size_t)p) + "_";

        out << "\tsubgraph cluster_" << newPref << "pipeline" << " {\n";
        out << "\t\tlabel=\"" << escapedLabel(name) << "\";\n";
        out << "\t\tcluster=true;\n";
        out << "\t\tstyle=\"dashed,filled\";\n";
        out << "\t\tcolor=\"black\";\n";
        out << "\t\tfillcolor=\"" + colorFromName(name) + "\";\n";

        exportPipeline(*p, aliases, out, newPref, false, expandSubGraphs);

        out << "\t}\n";
    }

    if (isMainGraph) {
        out << "}";
    }
}

} // namespace Vitrae