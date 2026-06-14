#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/PixelTyping.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/UniqueAnyPtr.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Params/ParamAliases.hpp"
#include "Vitrae/Util/UniqueId.hpp"

#include "assimp/material.h"

#include <glm/glm.hpp>

#include <functional>
#include <span>
#include <vector>

class aiMesh;

namespace Vitrae
{

class AssimpConvCollection
{
  public:
    AssimpConvCollection();

    // ---- AssImp mesh buffers --------------------------------------------------------------------

    /**
     * Information about an aiMesh buffer conversion
     * @tparam aiType The type of the desired buffer element
     */
    template <class aiType> struct MeshBufferConv
    {
        /**
         * A function that extracts a buffer from an aiMesh
         * @param extMesh The mesh to extract the buffer from
         * @returns a pointer to array of data from an aiMesh,
         * or nullptr if data cannot be found
         */
        using ExtractorF = std::function<const aiType *(const aiMesh &extMesh)>;

        /// The name of the vertex component
        StringId name;

        /// The extractor function
        ExtractorF extractor;
    };

    /**
     * Adds a new AiMeshBufferConv to the mesh buffer info list.
     * @tparam aiType The type of the buffer element
     * @param newInfo The AiMeshBufferConv to add
     */
    template <class aiType> void addMeshBufferConv(const MeshBufferConv<aiType> &newInfo);

    /**
     * @tparam aiType The type of the buffer element
     * @return Span of AiMeshBufferConv for the specified type.
     */
    template <class aiType> std::span<const MeshBufferConv<aiType>> getMeshBufferConvs() const;

    // ---- AssImp shading modes -------------------------------------------------------------------

    /**
     * Adds a new shading mode conversion
     */
    void addShadingModeConv(aiShadingMode aiMode, const ParamAliases &newInfo);

    /**
     * @return The aliases for the specified shading mode
     */
    const ParamAliases &getShadingModeParamAliases(aiShadingMode aiMode) const;

    // ---- AssImp materials -----------------------------------------------------------------------

    /**
     * Conversion information for an aiMaterial texture
     */
    template <class TextureT> struct TextureConv
    {
        /// The name of the texture color sample (such as 'diffuse')
        String sampleName;

        /// The type of the texture (eg. aiTextureType_DIFFUSE)
        aiTextureType aiTextureId;

        /// The color which will be set as the texture if the texture is missing
        PixelValueType<TextureT::PIXEL_TYPE> defaultColor;
    };

    /**
     * Conversion information for an aiMaterial property
     */
    struct MaterialPropertyConv
    {
        /**
         * A function that extracts a property from an aiMaterial
         * @param extMat The material to extract the property from
         * @returns the property value or an empty optional if the property cannot be found
         */
        using ExtractorF = std::function<std::optional<Variant>(const aiMaterial &extMat)>;

        /// The name of the property
        StringId nameId;

        /// The extractor function
        ExtractorF extractor;
    };

    /**
     * Adds a new TextureConv to the list
     */
    template <class TextureT> void addMaterialTexture(TextureConv<TextureT> newInfo);

    /**
     * @return Span of TextureConv
     */
    template <class TextureT> std::span<const TextureConv<TextureT>> getTextureConvs() const;

    /**
     * Adds a new MaterialPropertyConv to the list.
     * @param newInfo The MaterialPropertyConv to add
     */
    void addMaterialPropertyConv(const MaterialPropertyConv &newInfo);

    /**
     * @return Span of MaterialPropertyConv.
     */
    std::span<const MaterialPropertyConv> getMaterialPropertyConvs() const;

  protected:
    // Just for keeping a unique counter
    struct MeshBufferConvIDToken
    {};
    template <class aiType> using MeshBufferConvList = std::vector<MeshBufferConv<aiType>>;

    // Just for keeping a unique counter
    struct TextureConvIDToken
    {};
    template <class TextureT> using TextureConvList = std::vector<TextureConv<TextureT>>;

    template <class aiType> MeshBufferConvList<aiType> &getMeshBufferConvList();
    template <class aiType> const MeshBufferConvList<aiType> *getMeshBufferConvListPtr() const;

    template <class TextureT> TextureConvList<TextureT> &getTextureConvList();
    template <class TextureT> const TextureConvList<TextureT> *getTextureConvListPtr() const;

    std::vector<UniqueAnyPtr> m_aiMeshConvLists;
    std::vector<UniqueAnyPtr> m_aiTextureConvLists;
    StableMap<aiShadingMode, ParamAliases> m_aiShadingModeAliases;
    std::vector<MaterialPropertyConv> m_aiMaterialPropertyConvs;
};

// ==== AssimpConv implementations =================================================================

template <class aiType>
void AssimpConvCollection::addMeshBufferConv(const MeshBufferConv<aiType> &newInfo)
{
    this->getMeshBufferConvList<aiType>().push_back(newInfo);
}

template <class aiType>
std::span<const AssimpConvCollection::MeshBufferConv<aiType>> AssimpConvCollection::
    getMeshBufferConvs() const
{
    if (auto mptr = this->getMeshBufferConvListPtr<aiType>())
        return std::span(*mptr);
    else
        return {};
}

template <class TextureT>
void AssimpConvCollection::addMaterialTexture(TextureConv<TextureT> newInfo)
{
    this->getTextureConvList<TextureT>().push_back(newInfo);
}

template <class TextureT>
std::span<const AssimpConvCollection::TextureConv<TextureT>> AssimpConvCollection::getTextureConvs()
    const
{
    if (auto mptr = this->getTextureConvListPtr<TextureT>())
        return std::span(*mptr);
    else
        return {};
}

template <class aiType>
AssimpConvCollection::MeshBufferConvList<aiType> &AssimpConvCollection::getMeshBufferConvList()
{
    std::size_t ind = getScopedClassID<MeshBufferConvIDToken, MeshBufferConvList<aiType>>();

    if (ind >= m_aiMeshConvLists.size()) {
        m_aiMeshConvLists.resize(ind + 1);
        m_aiMeshConvLists[ind] = new MeshBufferConvList<aiType>();
    }

    return *(m_aiMeshConvLists[ind].template get<MeshBufferConvList<aiType>>());
}

template <class aiType>
const AssimpConvCollection::MeshBufferConvList<aiType> *AssimpConvCollection::
    getMeshBufferConvListPtr() const
{
    std::size_t ind = getScopedClassID<MeshBufferConvIDToken, MeshBufferConvList<aiType>>();
    if (ind < m_aiMeshConvLists.size()) {
        auto &listPtr = m_aiMeshConvLists[ind];
        if (listPtr)
            return listPtr.template get<MeshBufferConvList<aiType>>();
    }
    return nullptr;
}

template <class TextureT>
AssimpConvCollection::TextureConvList<TextureT> &AssimpConvCollection::getTextureConvList()
{
    std::size_t ind = getScopedClassID<TextureConvIDToken, TextureConvList<TextureT>>();

    if (ind >= m_aiTextureConvLists.size()) {
        m_aiTextureConvLists.resize(ind + 1);
        m_aiTextureConvLists[ind] = new TextureConvList<TextureT>();
    }

    return *(m_aiTextureConvLists[ind].template get<TextureConvList<TextureT>>());
}

template <class TextureT>
const AssimpConvCollection::TextureConvList<TextureT> *AssimpConvCollection::getTextureConvListPtr()
    const
{
    std::size_t ind = getScopedClassID<TextureConvIDToken, TextureConvList<TextureT>>();
    if (ind < m_aiTextureConvLists.size()) {
        auto &listPtr = m_aiTextureConvLists[ind];
        if (listPtr)
            return listPtr.template get<TextureConvList<TextureT>>();
    }
    return nullptr;
}

} // namespace Vitrae