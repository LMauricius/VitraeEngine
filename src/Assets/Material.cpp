#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/AssimpConv.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Data/Monostates.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Renderer.hpp"
#include "Vitrae/Setup/TextureFiltering.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <assimp/GltfMaterial.h>

namespace Vitrae
{

namespace
{
TextureFilteringParams extractFiltering(const aiMaterial &mat, aiTextureType type, std::size_t ind)
{
    /// TODO: No default, global settings for sampling
    TextureFilteringParams ret = FilteringCommon::TRILINEAR_TILED;

    // --- Wrapping ---

    static constexpr WrappingType mapMode2wrap[] = {
        /*[aiTextureMapMode_Wrap]   = */ WrappingType::REPEAT,
        /*[aiTextureMapMode_Clamp]  = */ WrappingType::CLAMP,
        /*[aiTextureMapMode_Mirror] = */ WrappingType::MIRROR,
        /*[aiTextureMapMode_Decal]  = */ WrappingType::BORDER_COLOR,
    };
    static constexpr std::size_t sizeMapMode2wrap = sizeof(mapMode2wrap) / sizeof(mapMode2wrap[0]);

    bool isDecalMap = false;

    // Use Get(), not GetTexture(): GetTexture defaults mapmode to Wrap, hiding "absent"
    if (int gotMode; mat.Get(AI_MATKEY_MAPPINGMODE_U(type, ind), gotMode) == aiReturn_SUCCESS &&
                     gotMode >= 0 && gotMode < sizeMapMode2wrap) {
        ret.horWrap = mapMode2wrap[gotMode];
        isDecalMap |= (gotMode == aiTextureMapMode_Decal);
    }
    if (int gotMode; mat.Get(AI_MATKEY_MAPPINGMODE_V(type, ind), gotMode) == aiReturn_SUCCESS &&
                     gotMode >= 0 && gotMode < sizeMapMode2wrap) {
        ret.verWrap = mapMode2wrap[gotMode];
        isDecalMap |= (gotMode == aiTextureMapMode_Decal);
    }

    // Decals are transparent outside [0,1]
    if (isDecalMap)
        ret.borderColor = glm::vec4(0.0f);
    else
        ret.borderColor = UNUSED;

    // --- Filtering ---

    // Filter constants
    // The glTF importer writes them, and uses OpenGL enums,
    // which aren't available here because the engine is renderer-agnostic
    // Values are glTF sampler filters (= OpenGL enums); Assimp doesn't expose names for them
    enum class GltfConstant : int {
        NEAREST = 9728,
        LINEAR = 9729,
        NEAREST_MIPMAP_NEAREST = 9984,
        LINEAR_MIPMAP_NEAREST = 9985,
        NEAREST_MIPMAP_LINEAR = 9986,
        LINEAR_MIPMAP_LINEAR = 9987,
    };

    // Magnification
    if (int gotFilter;
        mat.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(type, ind), gotFilter) == aiReturn_SUCCESS) {
        switch ((GltfConstant)gotFilter) {
        case GltfConstant::NEAREST:
            ret.magFilter = FilterType::NEAREST;
            break;
        case GltfConstant::LINEAR:
            ret.magFilter = FilterType::LINEAR;
            break;
        default:
            // unused
            break;
        }
    }

    // Minification + mipmaps
    if (int gotFilter;
        mat.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MIN(type, ind), gotFilter) == aiReturn_SUCCESS) {
        switch ((GltfConstant)gotFilter) {
        case GltfConstant::NEAREST:
            ret.minFilter = FilterType::NEAREST;
            ret.useMipMaps = false;
            ret.mipmapFilter = UNUSED;
            break;
        case GltfConstant::LINEAR:
            ret.minFilter = FilterType::LINEAR;
            ret.useMipMaps = false;
            ret.mipmapFilter = UNUSED;
            break;
        case GltfConstant::NEAREST_MIPMAP_NEAREST:
            ret.minFilter = FilterType::NEAREST;
            ret.useMipMaps = true;
            ret.mipmapFilter = FilterType::NEAREST;
            break;
        case GltfConstant::LINEAR_MIPMAP_NEAREST:
            ret.minFilter = FilterType::LINEAR;
            ret.useMipMaps = true;
            ret.mipmapFilter = FilterType::NEAREST;
            break;
        case GltfConstant::NEAREST_MIPMAP_LINEAR:
            ret.minFilter = FilterType::NEAREST;
            ret.useMipMaps = true;
            ret.mipmapFilter = FilterType::LINEAR;
            break;
        case GltfConstant::LINEAR_MIPMAP_LINEAR:
            ret.minFilter = FilterType::LINEAR;
            ret.useMipMaps = true;
            ret.mipmapFilter = FilterType::LINEAR;
            break;
        }
    }

    return ret;
}
} // namespace

Material::Material(const SetupParams &params)
    : m_root(params.root), m_externalAliases(params.aliases), m_properties(params.properties)
{
    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);
}

Material::Material(const AssimpLoadParams &params) : m_root(params.root)
{
    AssimpConvCollection &convs = params.root.getComponent<AssimpConvCollection>();

    std::filesystem::path parentDirPath = params.sceneFilepath.parent_path();

    forTextureTypes([&]<class Texture> {
        TextureManager<Texture> &textureManager =
            params.root.getComponent<TextureManager<Texture>>();

        // Get all textures
        for (auto &textureConv : convs.getTextureConvs<Texture>()) {
            if (params.p_extMaterial->GetTextureCount(textureConv.aiTextureId) > 0) {
                aiString path;
                aiReturn res = params.p_extMaterial->GetTexture(textureConv.aiTextureId, 0, &path);

                if (res == aiReturn_SUCCESS) {
                    String relconvPath =
                        searchAndReplace(searchAndReplace(path.C_Str(), "\\", "/"), "//", "/");

                    // add alias for texture coordinate
                    m_tobeInternalAliases["coord_" + textureConv.sampleName] =
                        StandardParam::coord_base.name;

                    // add alias for texture color
                    m_tobeInternalAliases["color_" + textureConv.sampleName] =
                        "sample_" + textureConv.sampleName;

                    // set sampler
                    m_root.getComponent<Renderer>().specifyTextureSampler(
                        textureConv.sampleName, TYPE_INFO<dynasma::FirmPtr<Texture>>);

                    // set texture
                    m_properties["tex_" + textureConv.sampleName] =
                        textureManager
                            .register_asset({typename Texture::FileLoadParams{
                                .root = params.root,
                                .filepath = parentDirPath / relconvPath,
                                .filtering = extractFiltering(*params.p_extMaterial,
                                                              textureConv.aiTextureId, 0),
                            }})
                            .getLoaded();
                } else {
                    m_properties["color_" + textureConv.sampleName] = textureConv.defaultColor;
                }
            } else {
                m_properties["color_" + textureConv.sampleName] = textureConv.defaultColor;
            }
        }
    });

    // get all properties
    for (auto &propertyInfo : convs.getMaterialPropertyConvs()) {
        std::optional<Variant> value = propertyInfo.extractor(*params.p_extMaterial);
        if (value.has_value()) {
            m_properties[propertyInfo.nameId] = std::move(value.value());
        }
    }

    // get shading type
    aiShadingMode aiMode;
    if (params.p_extMaterial->Get(AI_MATKEY_SHADING_MODEL, aiMode) != aiReturn_SUCCESS) {
        aiMode = aiShadingMode_Phong;
    }

    m_externalAliases = convs.getShadingModeParamAliases(aiMode);
    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);
}

Material::~Material() {}

std::size_t Material::memory_cost() const
{
    /// TODO: caculate real cost
    return sizeof(Material);
}

void Material::setParamAliases(const ParamAliases &aliases)
{
    m_externalAliases = aliases;
    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);
}

void Material::setProperty(StringId key, const Variant &value)
{
    m_properties[key] = value;
}

void Material::setProperty(StringId key, Variant &&value)
{
    m_properties[key] = std::move(value);
}

void Material::setTexturePtr(StringView colorName, const Variant &texture,
                             StringView coordPropertyName)
{
    // add alias for texture coordinate
    m_tobeInternalAliases["coord_" + std::string(colorName)] = std::string(coordPropertyName);

    // add alias for texture color
    m_tobeInternalAliases["color_" + std::string(colorName)] = "sample_" + std::string(colorName);

    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);

    // set texture
    m_root.getComponent<Renderer>().specifyTextureSampler(colorName, texture.getAssignedTypeInfo());
    m_properties["tex_" + std::string(colorName)] = std::move(texture);
}

void Material::setTexturePtr(StringView colorName, Variant &&texture, StringView coordPropertyName)
{
    // add alias for texture coordinate
    m_tobeInternalAliases["coord_" + std::string(colorName)] = std::string(coordPropertyName);

    // add alias for texture color
    m_tobeInternalAliases["color_" + std::string(colorName)] = "sample_" + std::string(colorName);

    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);

    // set texture
    m_root.getComponent<Renderer>().specifyTextureSampler(colorName, texture.getAssignedTypeInfo());
    m_properties["tex_" + std::string(colorName)] = std::move(texture);
}

void Material::setTextureColor(StringView colorName, const Variant &uniformColor)
{
    // erase alias for texture coordinate
    m_tobeInternalAliases.erase("coord_" + std::string(colorName));

    // erase alias for texture sample
    m_tobeInternalAliases.erase("color_" + std::string(colorName));

    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);

    // set color of all samples
    m_properties["color_" + std::string(colorName)] = uniformColor;
}

void Material::setTextureColor(StringView colorName, Variant &&uniformColor)
{
    // erase alias for texture coordinate
    m_tobeInternalAliases.erase("coord_" + std::string(colorName));

    // erase alias for texture sample
    m_tobeInternalAliases.erase("color_" + std::string(colorName));

    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);

    // set color of all samples
    m_properties["color_" + std::string(colorName)] = uniformColor;
}

const ParamAliases &Material::getParamAliases() const
{
    return m_aliases;
}

const StableMap<StringId, Variant> &Material::getProperties() const
{
    return m_properties;
}

} // namespace Vitrae