#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/AssimpConv.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Renderer.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

namespace Vitrae
{

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

                    // set texture
                    m_root.getComponent<Renderer>().specifyTextureSampler(textureConv.sampleName);
                    m_properties["tex_" + textureConv.sampleName] =
                        textureManager
                            .register_asset({typename Texture::FileLoadParams{
                                .root = params.root,
                                .filepath = parentDirPath / relconvPath,
                                .filtering{
                                    .useMipMaps = true,
                                },
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
    m_root.getComponent<Renderer>().specifyTextureSampler(colorName);
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
    m_root.getComponent<Renderer>().specifyTextureSampler(colorName);
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