#pragma once

#include "Vitrae/Pipelines/Pipeline.hpp"
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
void exportPipeline(const Pipeline<BasicTask> &pipeline, std::ostream &out)
{
    using Pipeline = Pipeline<BasicTask>;

    auto escapedLabel = [&](StringView label) -> String {
        String ret = searchAndReplace(String(label), "\"", "\\\"");
        ret = searchAndReplace(ret, "\n", "\\n");
        return ret;
    };
    auto getPropId = [&](const PropertySpec &spec) -> String {
        return String("Prop_") + spec.name;
    };
    auto getTaskId = [&](const BasicTask &task) -> String {
        return String("Task_") + escapedLabel(task.getFriendlyName()) +
               std::to_string((std::size_t)&task);
    };
    auto outputTaskNode = [&](StringView id, std::size_t ord, const BasicTask &task) {
        out << id << " [";
        out << "label=\""
            << escapedLabel(String("#") + std::to_string(ord) + "\\n" +
                            String(task.getFriendlyName()))
            << "\", ";
        out << "group=\"Tasks\", ";
        out << "shape=box, ";
        out << "style=\"filled\", ";
        out << "fillcolor=\"lightblue\", ";
        out << "bgcolor=\"lightblue\", ";
        out << "];\n";
    };
    auto outputPropNode = [&](StringView id, const PropertySpec &spec, bool horizontal) {
        out << id << " [";
        out << "label=\"" << escapedLabel(spec.name);
        if (spec.typeInfo == Variant::getTypeInfo<void>()) {
            out << "\", ";
            out << "shape=hexagon, ";
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
        out << id << " [";
        out << "style=\"invis\", ";
        out << "shape=\"point\", ";
        out << "];\n";
    };
    auto sameRank = [&](StringView from, StringView to) {
        out << "{rank=same; " << from << "; " << to << "}\n";
    };
    auto outputUsage = [&](StringView from, StringView to, bool changeRank) {
        out << from << " -> " << to;
        if (!changeRank) {
            out << " [minlen=0]";
        }
        out << ";\n";
    };
    auto outputConsumption = [&](StringView from, StringView to, bool changeRank) {
        out << from << " -> " << to;
        if (!changeRank) {
            out << " [minlen=0]";
        }
        out << ";\n";
    };
    auto outputGeneration = [&](StringView from, StringView to, bool changeRank) {
        out << from << " -> " << to;
        if (!changeRank) {
            out << " [minlen=0]";
        }
        out << ";\n";
    };
    auto outputModification = [&](StringView from, StringView to, bool changeRank) {
        out << from << " -> " << to;
        if (!changeRank) {
            out << " [minlen=0]";
        }
        out << ";\n";
    };
    auto outputEquivalence = [&](StringView from, StringView to, bool changeRank,
                                 StringView lhead = "", StringView ltail = "") {
        out << from << " -> " << to;
        out << "[style=dashed";
        if (lhead.length()) {
            out << ", lhead=" << lhead;
        }
        if (ltail.length()) {
            out << ", ltail=" << ltail;
        }
        if (!changeRank) {
            out << ", minlen=0";
        }
        out << "];\n";
    };
    auto outputInvisibleDirection = [&](StringView from, StringView to, bool changeRank,
                                        StringView lhead = "", StringView ltail = "") {
        out << from << " -> " << to << "[style=invis";
        if (lhead.length()) {
            out << ", lhead=" << lhead;
        }
        if (ltail.length()) {
            out << ", ltail=" << ltail;
        }
        if (!changeRank) {
            out << ", minlen=0";
        }
        out << "];\n";
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
        if (auto it = activeNodeIdsPerNameId.find(spec.name); it != activeNodeIdsPerNameId.end()) {
            return it->second;
        } else if (auto it = pendingNodeIdsPerNameId.find(spec.name);
                   it != pendingNodeIdsPerNameId.end()) {
            return it->second;
        } else {
            String id = getPropId(spec);
            if (auto it = repetitionCounterPerNameId.find(spec.name);
                it != repetitionCounterPerNameId.end()) {
                ++it->second;
                id += std::to_string(it->second);
            } else {
                repetitionCounterPerNameId[spec.name] = 1;
            }

            pendingNodeIdsPerNameId[spec.name] = id;
            return id;
        }
    };

    auto addPropertyNodeHere = [&](const PropertySpec &spec) {
        if (auto it = activeNodeIdsPerNameId.find(spec.name); it != activeNodeIdsPerNameId.end()) {
            throw std::runtime_error("addPropertyNodeHere: property " + std::string(spec.name) +
                                     " already has an active node: " + it->second);
        } else {
            String id;
            if (auto it = pendingNodeIdsPerNameId.find(spec.name);
                it != pendingNodeIdsPerNameId.end()) {
                id = it->second;
                pendingNodeIdsPerNameId.erase(it);
            } else {
                id = currentPropertyNodeReference(spec);
            }

            activeNodeIdsPerNameId.emplace(spec.name, id);
            outputPropNode(id, spec, horizontalInputsOutputs);
        }
    };

    auto consumePropertyNode = [&](const PropertySpec &spec) {
        if (auto it = activeNodeIdsPerNameId.find(spec.name); it != activeNodeIdsPerNameId.end()) {
            String id = it->second;
            activeNodeIdsPerNameId.erase(it);
            return id;
        } else if (auto it = pendingNodeIdsPerNameId.find(spec.name);
                   it != pendingNodeIdsPerNameId.end()) {
            String id = it->second;
            addPropertyNodeHere(spec);
            return id;
        } else {
            throw std::runtime_error("consumePropertyNode: property " + std::string(spec.name) +
                                     " has no active or pending node");
        }
    };

    /*
    Output
    */

    out << "digraph {\n";
    out << "\trankdir=\"LR\"\n";
    out << "\tranksep=0.25\n";
    out << "\tnodesep=0.13\n";
    out << "\tcompound=true;\n";

    // inputs
    out << "\tsubgraph cluster_inputs {\n";
    out << "\t\tlabel=\"Inputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";

    for (const PropertyList *p_specs : {&pipeline.inputSpecs, &pipeline.filterSpecs,
                                        &pipeline.consumingSpecs, &pipeline.pipethroughSpecs}) {
        for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
            out << "\t\t";
            addPropertyNodeHere(spec);
        }
    }
    out << "\t}\n";

    out << "\tsubgraph cluster_processing {\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=invis;\n";

    // tasks
    std::size_t ordinalI = 0;
    for (auto p_task : pipeline.items) {
        ++ordinalI;
        out << "\t\t";
        outputTaskNode(getTaskId(*p_task), ordinalI, *p_task);

        for (auto [nameId, spec] : p_task->getInputSpecs(pipeline.usedSelection).getMappedSpecs()) {
            out << "\t";
            outputUsage(currentPropertyNodeReference(spec), getTaskId(*p_task), true);
        }

        for (auto [nameId, spec] :
             p_task->getConsumingSpecs(pipeline.usedSelection).getMappedSpecs()) {
            out << "\t";
            String propNodeId = currentPropertyNodeReference(spec);
            consumePropertyNode(spec);
            outputConsumption(propNodeId, getTaskId(*p_task), false);
        }

        for (auto [nameId, spec] :
             p_task->getFilterSpecs(pipeline.usedSelection).getMappedSpecs()) {
            out << "\t";
            String prevPropNodeId = currentPropertyNodeReference(spec);
            consumePropertyNode(spec);
            String curPropNodeId = currentPropertyNodeReference(spec);
            addPropertyNodeHere(spec);
            outputEquivalence(prevPropNodeId, curPropNodeId, false);
            outputModification(curPropNodeId, getTaskId(*p_task), false);
            sameRank(curPropNodeId, getTaskId(*p_task));
        }

        for (auto [nameId, spec] :
             p_task->getOutputSpecs(pipeline.usedSelection).getMappedSpecs()) {
            out << "\t";
            outputGeneration(getTaskId(*p_task), currentPropertyNodeReference(spec), false);
        }
    }
    out << "\t}\n";

    // unused local properties
    for (auto [nameId, spec] : pipeline.localSpecs.getMappedSpecs()) {
        if (pendingNodeIdsPerNameId.find(nameId) == pendingNodeIdsPerNameId.end()) {
            addPropertyNodeHere(spec);
        }
    }

    // outputs
    out << "\tsubgraph cluster_outputs {\n";
    out << "\t\tlabel=\"Outputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";

    for (auto [nameId, spec] : pipeline.outputSpecs.getMappedSpecs()) {
        out << "\t\t";
        if (auto it = pendingNodeIdsPerNameId.find(nameId); it != pendingNodeIdsPerNameId.end()) {
            String id = it->second;
            addPropertyNodeHere(spec);
        } else if (auto it = activeNodeIdsPerNameId.find(nameId);
                   it != activeNodeIdsPerNameId.end()) {
            String prevPropNodeId = currentPropertyNodeReference(spec);
            consumePropertyNode(spec);
            String curPropNodeId = currentPropertyNodeReference(spec);
            addPropertyNodeHere(spec);
            outputEquivalence(prevPropNodeId, curPropNodeId, false);
        } else {
            throw std::runtime_error("outputs: property " + std::string(spec.name) +
                                     " has no active or pending node");
        }
    }
    for (auto [nameId, spec] : pipeline.pipethroughSpecs.getMappedSpecs()) {
        out << "\t\t";
        if (auto it = activeNodeIdsPerNameId.find(nameId); it != activeNodeIdsPerNameId.end()) {
            String prevPropNodeId = currentPropertyNodeReference(spec);
            consumePropertyNode(spec);
            String curPropNodeId = currentPropertyNodeReference(spec);
            addPropertyNodeHere(spec);
            outputEquivalence(prevPropNodeId, curPropNodeId, false);
        } else {
            throw std::runtime_error("outputs: property " + std::string(spec.name) +
                                     " has no active or pending node");
        }
    }

    out << "\t}\n";

    out << "}";
}

} // namespace Vitrae