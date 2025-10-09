#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/UniqueAnyPtr.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/UniqueId.hpp"

#include "dynasma/pool.hpp"

#include <vector>

class aiMesh;

namespace Vitrae
{
class Texture;
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
    template <class T> void setComponent(Unique<T> &&comp)
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
    template <class T> void setComponent(T *p_comp)
    {
        UniqueAnyPtr &myvar = getGenericStorageVariable<T>();
        if constexpr (std::derived_from<T, dynasma::AbstractPool>) {
            auto it = std::find(m_memoryPools.begin(), m_memoryPools.end(), myvar.get<T>());
            if (it != m_memoryPools.end()) {
                *it = p_comp;
            } else {
                m_memoryPools.push_back(p_comp);
            }
        }
        myvar = std::move(Unique<T>(p_comp));
    }

    /**
     * @return The component of a particular type T
     */
    template <class T> T &getComponent() const
    {
        const UniqueAnyPtr &myvar = getGenericStorageVariable<T>();
        return *(myvar.get<T>());
    }

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

    inline std::ostream &getErrStream() const { return *mErrStream; }
    inline std::ostream &getInfoStream() const { return *mInfoStream; }
    inline std::ostream &getWarningStream() const { return *mWarningStream; }
    inline void setErrStream(std::ostream &os) { mErrStream = &os; }
    inline void setInfoStream(std::ostream &os) { mInfoStream = &os; }
    inline void setWarningStr(std::ostream &os) { mWarningStream = &os; }

  protected:
    /*
    Returns a variable to store the manager of a particular type.
    Is specialized to return member variables for defaultly supported types.
    */
    template <class T> UniqueAnyPtr &getGenericStorageVariable()
    {
        return mCustomComponents[getClassID<T>()];
    }
    template <class T> const UniqueAnyPtr &getGenericStorageVariable() const
    {
        try {
            return mCustomComponents.at(getClassID<T>());
        }
        catch (std::out_of_range) {
            throw std::out_of_range("Component " + std::string(TYPE_INFO<T *>.getShortTypeName()) +
                                    " not registered");
        }
    }

    StableMap<size_t, UniqueAnyPtr> mCustomComponents;
    std::vector<dynasma::AbstractPool *> m_memoryPools;

    std::ostream *mErrStream, *mInfoStream, *mWarningStream;
};
} // namespace Vitrae