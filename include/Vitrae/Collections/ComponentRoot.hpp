#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/UniqueAnyPtr.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/UniqueId.hpp"

#include "dynasma/pool.hpp"

#include <type_traits>
#include <vector>

class aiMesh;

namespace Vitrae
{
class Material;
class Mesh;
class Model;
class ShaderTask;

/*
A HUB of multiple asset managers and other components.
One ComponentRoot must be used for all related resources
*/
class ComponentRoot
{
  public:
    ComponentRoot();
    ~ComponentRoot();

    // ---- Generic components ---------------------------------------------------------------------

    /**
     * Sets the component of a particular type and takes its ownership.
     * @tparam T The component type
     * @param comp The component pointer to set. Has to be derived from T
     */
    template <class T> void setComponent(Unique<T> &&comp);
    /**
     * Sets the component of a particular type and takes its ownership.
     * @tparam T The component type
     * @param comp The component pointer to set. Has to be derived from T
     */
    template <class T> void setComponent(T *p_comp);

    /// @returns The component of a particular type T
    template <class T> T &getComponent() const;

    /**
     * @brief Attempts to unload not-firmly-referenced assets to free memory
     * @param bytenum the number of bytes to attempt to free from memory
     * @returns the number of bytes freed
     * @note Which types of assets are freed in which order/amount is not specified
     */
    std::size_t cleanMemoryPools(std::size_t bytenum);

    /*
    ---- Streams -----------------------------------------------------------------------------------
    */

    std::ostream &getErrStream() const;
    std::ostream &getInfoStream() const;
    std::ostream &getWarningStream() const;
    void setErrStream(std::ostream &os);
    void setInfoStream(std::ostream &os);
    void setWarningStr(std::ostream &os);

  protected:
    // Just for keeping a unique counter
    struct ComponentIDToken
    {};

    /*
    Returns a variable to store the manager of a particular type.
    Is specialized to return member variables for defaultly supported types.
    */
    template <class T> UniqueAnyPtr &getGenericStorageVariable();
    template <class T> const UniqueAnyPtr &getAssignedGenericStorageVariable() const;

    std::vector<UniqueAnyPtr> mCustomComponents;
    std::vector<dynasma::AbstractPool *> m_memoryPools;

    std::ostream *mErrStream, *mInfoStream, *mWarningStream;
};

// ==== ComponentRoot implementations ==============================================================

template <class T> void ComponentRoot::setComponent(Unique<T> &&comp)
{
    UniqueAnyPtr &myvar = getGenericStorageVariable<T>();
    if constexpr (std::derived_from<T, dynasma::AbstractPool>) {
        auto it = std::find(m_memoryPools.begin(), m_memoryPools.end(), myvar.get<T>());
        if (it != m_memoryPools.end()) {
            *it = comp.get();
        } else {
            m_memoryPools.push_back(comp.get());
        }
    }
    myvar = std::move(comp);
}

template <class T> void ComponentRoot::setComponent(T *p_comp)
{
    using UnderlyingT = std::remove_cv_t<T>;

    UniqueAnyPtr &myvar = getGenericStorageVariable<UnderlyingT>();
    if constexpr (std::derived_from<UnderlyingT, dynasma::AbstractPool>) {
        auto it = std::find(m_memoryPools.begin(), m_memoryPools.end(), myvar.get<UnderlyingT>());
        if (it != m_memoryPools.end()) {
            *it = p_comp;
        } else {
            m_memoryPools.push_back(p_comp);
        }
    }
    myvar = std::move(Unique<UnderlyingT>(p_comp));
}

template <class T> T &ComponentRoot::getComponent() const
{
    using UnderlyingT = std::remove_cv_t<T>;

    const UniqueAnyPtr &myvar = getAssignedGenericStorageVariable<UnderlyingT>();
    return *(myvar.get<UnderlyingT>());
}

inline std::ostream &ComponentRoot::getErrStream() const
{
    return *mErrStream;
}
inline std::ostream &ComponentRoot::getInfoStream() const
{
    return *mInfoStream;
}
inline std::ostream &ComponentRoot::getWarningStream() const
{
    return *mWarningStream;
}
inline void ComponentRoot::setErrStream(std::ostream &os)
{
    mErrStream = &os;
}
inline void ComponentRoot::setInfoStream(std::ostream &os)
{
    mInfoStream = &os;
}
inline void ComponentRoot::setWarningStr(std::ostream &os)
{
    mWarningStream = &os;
}

template <class T> UniqueAnyPtr &ComponentRoot::getGenericStorageVariable()
{
    std::size_t ind = getScopedClassID<ComponentIDToken, T>();

    if (ind >= mCustomComponents.size()) {
        mCustomComponents.resize(ind + 1);
    }

    return mCustomComponents[ind];
}

template <class T> const UniqueAnyPtr &ComponentRoot::getAssignedGenericStorageVariable() const
{
    std::size_t ind = getScopedClassID<ComponentIDToken, T>();

    if (ind >= mCustomComponents.size() || !mCustomComponents[ind]) {
        throw std::out_of_range("Component " + std::string(TYPE_INFO<T *>.getShortTypeName()) +
                                " not registered");
    }

    return mCustomComponents[ind];
}

} // namespace Vitrae