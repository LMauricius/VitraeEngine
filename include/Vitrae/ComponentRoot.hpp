#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Data/UniqueAnyPtr.hpp"
#include "Vitrae/Pipelines/Method.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "Vitrae/Util/UniqueId.hpp"

#include "assimp/material.h"
#include "dynasma/cachers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <any>
#include <map>
#include <memory>
#include <span>
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

    /*
    === Components ===
    */

    /**
    Sets the component of a particular type and
    takes its ownership.
    @param comp The component pointer to set
    */
    template <class T> void setComponent(Unique<T> comp)
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
    @return The component of a particular type T
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
    === AssImp mesh buffers ===
    */

    /**
     * A function that extracts a buffer from an aiMesh
     * @tparam aiType The type of the buffer element
     * @param extMesh The mesh to extract the buffer from
     * @returns a pointer to array of data from an aiMesh,
     * or nullptr if data cannot be found
     */
    template <class aiType>
    using AiMeshBufferExtractor = std::function<const aiType *(const aiMesh &extMesh)>;

    /**
     * Information about an aiMesh buffer
     * @tparam aiType The type of the buffer element
     */
    template <class aiType> struct AiMeshBufferInfo
    {
        /// The name of the vertex component
        StringId name;

        /// The extractor function
        AiMeshBufferExtractor<aiType> extractor;
    };

    /**
     * @tparam aiType The type of the buffer element
     * @return Span of AiMeshBufferInfo for the specified type.
     */
    template <class aiType> std::span<const AiMeshBufferInfo<aiType>> getAiMeshBufferInfos() const
    {
        auto &myvar = this->getMeshBufferInfoList<aiType>();
        return std::span(myvar);
    }

    /**
     * Adds a new AiMeshBufferInfo to the mesh buffer info list.
     * @tparam aiType The type of the buffer element
     * @param newInfo The AiMeshBufferInfo to add
     */
    template <class aiType> void addAiMeshBufferInfo(const AiMeshBufferInfo<aiType> &newInfo)
    {
        this->getMeshBufferInfoList<aiType>().push_back(newInfo);
    }

    void addAiMaterialPropertyAliases(aiShadingMode aiMode, const PropertyAliases &newInfo);
    const PropertyAliases &getAiMaterialPropertyAliases(aiShadingMode aiMode) const;

    struct AiMaterialTextureInfo
    {
        StringId textureNameId;
        aiTextureType aiTextureId;
        dynasma::LazyPtr<Texture> defaultTexture;
    };

    void addAiMaterialTextureInfo(AiMaterialTextureInfo newInfo);
    std::span<const AiMaterialTextureInfo> getAiMaterialTextureInfos() const;

    /**
     * Information about an aiMaterial property
     */
    struct AiMaterialPropertyInfo
    {
        /// The name of the property
        StringId nameId;

        /// The extractor function
        std::function<std::optional<Variant>(const aiMaterial &extMat)> extractor;
    };

    /**
     * @return Span of AiMaterialPropertyInfo.
     */
    std::span<const AiMaterialPropertyInfo> getAiMaterialPropertyInfos() const
    {
        return std::span(mMaterialPropertyInfos);
    }

    /**
     * Adds a new AiMaterialPropertyInfo to the list.
     * @param newInfo The AiMaterialPropertyInfo to add
     */
    void addAiMaterialPropertyInfo(const AiMaterialPropertyInfo &newInfo)
    {
        this->mMaterialPropertyInfos.push_back(newInfo);
    }

    /*
    === Streams ===
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
        return mCustomComponents.at(getClassID<T>());
    }

    template <class aiType> using MeshBufferInfoList = std::vector<AiMeshBufferInfo<aiType>>;

    template <class aiType> MeshBufferInfoList<aiType> &getMeshBufferInfoList()
    {
        auto &listPtr = m_aiMeshInfoLists[getClassID<MeshBufferInfoList<aiType>>()];
        if (!listPtr) {
            listPtr = new MeshBufferInfoList<aiType>();
        }
        return *(listPtr.template get<MeshBufferInfoList<aiType>>());
    }
    template <class aiType> const MeshBufferInfoList<aiType> &getMeshBufferInfoList() const
    {
        auto &listPtr = m_aiMeshInfoLists[getClassID<MeshBufferInfoList<aiType>>()];
        if (!listPtr) {
            listPtr = new MeshBufferInfoList<aiType>();
        }
        return *(listPtr.template get<MeshBufferInfoList<aiType>>());
    }

    StableMap<size_t, UniqueAnyPtr> mCustomComponents;
    std::vector<dynasma::AbstractPool *> m_memoryPools;
    mutable StableMap<size_t, UniqueAnyPtr> m_aiMeshInfoLists;
    StableMap<aiShadingMode, PropertyAliases> mAiMaterialAliases;
    std::vector<AiMaterialTextureInfo> mAiMaterialTextureInfos;
    std::vector<AiMaterialPropertyInfo> mMaterialPropertyInfos;

    std::ostream *mErrStream, *mInfoStream, *mWarningStream;
};
} // namespace Vitrae