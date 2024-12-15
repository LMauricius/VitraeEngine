#pragma once

#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Util/Hashing.hpp"

#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"
#include "dynasma/standalone.hpp"
#include "dynasma/util/dynamic_typing.hpp"

#include <map>
#include <optional>
#include <vector>

namespace Vitrae
{

template <TaskChild BasicTask> class Method : public dynasma::PolymorphicBase
{
  public:
    /*
    Management
    */

    struct MethodParams
    {
        std::vector<dynasma::FirmPtr<BasicTask>> tasks;
        std::vector<dynasma::FirmPtr<Method>> fallbackMethods;
        String friendlyName = "";
    };

    struct MethodManagerSeed
    {
        using Asset = Method;

        std::variant<MethodParams> kernel;

        inline std::size_t load_cost() const { return 1; }
    };

    using MethodManager = dynasma::AbstractManager<MethodManagerSeed>;

    // Default constructors
    Method() = default;
    Method(const Method &other) = default;
    Method(Method &&other) = default;

    // Constructor
    Method(const MethodParams &params) : m_friendlyName(params.friendlyName)
    {
        /*
        TODO: Better idea than this hack

        For now, let's assume that property aliasing won't affect the set of filterSpecs+outputSpecs
        of Tasks. Ideally, the output tasks shouldn't be affected by aliasing,
        but depending on alias setup, some outputSpecs might get turned into filterSpecs.

        That's the case for AdaptTasks for now, so we just use this hack
        */

        PropertyAliases dummyAliases;

        for (auto task : params.tasks) {
            for (auto outputId : task->getOutputSpecs(dummyAliases).getSpecNameIds()) {
                m_tasksPerOutput[outputId] = task;
            }
            for (auto outputId : task->getFilterSpecs(dummyAliases).getSpecNameIds()) {
                m_tasksPerOutput[outputId] = task;
            }
        }
        for (auto method : params.fallbackMethods) {
            m_fallbackMethods.push_back(method);
        }

        m_hash = 0;
        for (auto [nameId, p_task] : m_tasksPerOutput) {
            m_hash = combinedHashes<3>(
                {{m_hash, std::hash<StringId>{}(nameId), reinterpret_cast<std::size_t>(&*p_task)}});
        }
        for (auto method : m_fallbackMethods) {
            m_hash = combinedHashes<2>({{m_hash, method->getHash()}});
        }
    }

    // Destructor
    ~Method() = default;

    // Default assignments
    Method &operator=(const Method &other) = default;
    Method &operator=(Method &&other) = default;

    std::size_t memory_cost() const
    {
        std::size_t calcd = sizeof(*this);
        for (auto task : m_tasksPerOutput) {
            calcd += task.second->memory_cost();
        }
        for (auto method : m_fallbackMethods) {
            calcd += method->memory_cost();
        }
        return calcd;
    }

    /**
     * @returns the task generating the desired output, if found
     */
    std::optional<dynasma::FirmPtr<BasicTask>> getTask(StringId outputNameId) const
    {
        auto it = m_tasksPerOutput.find(outputNameId);
        if (it != m_tasksPerOutput.end()) {
            return (*it).second;
        }
        for (auto method : m_fallbackMethods) {
            auto task = method->getTask(outputNameId);
            if (task.has_value()) {
                return task;
            }
        }
        return std::nullopt;
    }

    std::size_t getHash() const { return m_hash; }
    StringView getFriendlyName() const { return m_friendlyName; }

  protected:
    StableMap<StringId, dynasma::FirmPtr<BasicTask>> m_tasksPerOutput;
    std::vector<dynasma::FirmPtr<Method>> m_fallbackMethods;
    std::size_t m_hash;
    String m_friendlyName;
};

template <TaskChild BasicTask> class MethodCombinator
{
    StableMap<std::size_t, dynasma::FirmPtr<Method<BasicTask>>> m_hash2combinedMethods;

  public:
    template <std::same_as<dynasma::FirmPtr<Method<BasicTask>>>... MethodPtrs>
    dynasma::FirmPtr<Method<BasicTask>> getCombinedMethod(MethodPtrs... p_methods)
    {
        std::size_t combinedMethodHash =
            combinedHashes<sizeof...(MethodPtrs)>({{p_methods->getHash()...}});

        if (auto it = m_hash2combinedMethods.find(combinedMethodHash);
            it != m_hash2combinedMethods.end()) {
            return (*it).second;
        } else {
            auto combinedMethod =
                dynasma::makeStandalone<Method<BasicTask>>(typename Method<BasicTask>::MethodParams{
                    .tasks = {},
                    .fallbackMethods = {p_methods...},
                    .friendlyName = (String() + (String(p_methods->getFriendlyName()) + ...))});
            m_hash2combinedMethods.emplace(combinedMethodHash, combinedMethod);
            return combinedMethod;
        }
    }
};

} // namespace Vitrae

namespace std
{
template <typename BasicTask> struct hash<Vitrae::Method<BasicTask>>
{
    std::size_t operator()(const Vitrae::Method<BasicTask> &method) const
    {
        return method.getHash();
    }
};
} // namespace std