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
    using Pipeitem = Pipeline::PipeItem;

    auto escapedLabel = [&](StringView label) -> String {
        String ret = searchAndReplace(String(label), "\"", "\\\"");
        ret = searchAndReplace(ret, "\n", "\\n");
        return ret;
    };
    auto getInputPropId = [&](const PropertySpec &spec) -> String {
        return String("Prop_") + spec.name;
    };
    auto getOutputPropId = [&](const PropertySpec &spec) -> String {
        return String("Prop_") + spec.name;
    };
    auto getTaskId = [&](const BasicTask &task) -> String {
        return String("Task_") + std::to_string((std::size_t)&task);
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
    auto outputConnection = [&](StringView from, StringView to, bool changeRank) {
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

    std::map<StringId, const Pipeitem *> itemsPerOutputs;
    std::set<StringId> usedOutputs;
    std::set<StringId> itemUsedOutputs;
    std::map<StringId, const PropertySpec *> intermediateSpecs;

    for (auto [nameId, spec] : pipeline.outputSpecs) {
        usedOutputs.insert(nameId);
        intermediateSpecs[nameId] = &spec;
    }

    for (auto [nameId, spec] : pipeline.inputSpecs) {
        intermediateSpecs[nameId] = &spec;
    }

    for (auto &item : pipeline.items) {
        for (auto [nameId, localNameId] : item.outputToLocalVariables) {
            itemsPerOutputs[nameId] = &item;
            intermediateSpecs[nameId] = &item.p_task->getOutputSpecs().at(nameId);
        }
        for (auto [nameId, localNameId] : item.inputToLocalVariables) {
            usedOutputs.insert(nameId);
            itemUsedOutputs.insert(nameId);
        }
    }

    bool horizontalInputsOutputs = pipeline.inputSpecs.size() > pipeline.items.size() ||
                                   pipeline.outputSpecs.size() > pipeline.items.size();

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

    for (auto [nameId, spec] : pipeline.inputSpecs) {
        out << "\t\t";
        outputPropNode(getInputPropId(spec), spec, horizontalInputsOutputs);
    }
    out << "\t}\n";

    out << "\tsubgraph cluster_processing {\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=invis;\n";

    // intermediates
    for (auto [nameId, p_spec] : intermediateSpecs) {
        if (pipeline.inputSpecs.find(nameId) == pipeline.inputSpecs.end() &&
            itemUsedOutputs.find(nameId) != itemUsedOutputs.end()) {
            out << "\t\t";
            outputPropNode(getOutputPropId(*p_spec), *p_spec, false);
        }
    }

    // tasks
    std::size_t ordinalI = 0;
    for (auto &item : pipeline.items) {
        ++ordinalI;
        out << "\t\t";
        outputTaskNode(getTaskId(*item.p_task), ordinalI, *item.p_task);

        for (auto [nameId, localNameId] : item.inputToLocalVariables) {
            out << "\t";
            outputConnection(getInputPropId(*intermediateSpecs.at(nameId)), getTaskId(*item.p_task),
                             true);
        }
    }
    out << "\t}\n";

    // outputs
    out << "\tsubgraph cluster_outputs {\n";
    out << "\t\tlabel=\"Outputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";

    for (auto [nameId, spec] : pipeline.outputSpecs) {
        out << "\t\t";
        if (pipeline.pipethroughInputNames.find(nameId) != pipeline.pipethroughInputNames.end() ||
            itemUsedOutputs.find(nameId) != itemUsedOutputs.end()) {
            outputPropNode(getOutputPropId(spec) + "_out", spec, horizontalInputsOutputs);
        } else {
            outputPropNode(getOutputPropId(spec), spec, horizontalInputsOutputs);
        }
    }

    // processed properties
    for (auto &item : pipeline.items) {
        for (auto [nameId, localNameId] : item.outputToLocalVariables) {
            if (usedOutputs.find(nameId) != usedOutputs.end()) {
                if (pipeline.inputSpecs.find(nameId) != pipeline.inputSpecs.end()) {
                    out << "\t\t";
                    outputConnection(getTaskId(*item.p_task),
                                     getOutputPropId(*intermediateSpecs.at(nameId)) + "_out", true);
                } else {
                    out << "\t\t";
                    outputConnection(getTaskId(*item.p_task),
                                     getOutputPropId(*intermediateSpecs.at(nameId)), true);
                }
            }
        }
    }

    out << "\t}\n";

    // pipethrough
    for (auto nameId : pipeline.pipethroughInputNames) {
        out << "\t";
        outputEquivalence(getInputPropId(*intermediateSpecs.at(nameId)),
                          getOutputPropId(*intermediateSpecs.at(nameId)) + "_out", false);
    }

    // both task inputs and pipeline outputs
    for (auto [nameId, spec] : pipeline.outputSpecs) {
        if (itemUsedOutputs.find(nameId) != itemUsedOutputs.end() &&
            pipeline.pipethroughInputNames.find(nameId) == pipeline.pipethroughInputNames.end()) {
            out << "\t";
            outputEquivalence(getOutputPropId(spec), getOutputPropId(spec) + "_out", false);
        }
    }

    out << "}";
}

} // namespace Vitrae