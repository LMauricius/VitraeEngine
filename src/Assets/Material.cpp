#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Renderer.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

namespace Vitrae
{

Material::Material(const AssimpLoadParams &params) : m_root(params.root)
{
    TextureManager &textureManager = params.root.getComponent<TextureManager>();

    std::filesystem::path parentDirPath = params.sceneFilepath.parent_path();

    // get shading type
    aiShadingMode aiMode;
    if (params.p_extMaterial->Get(AI_MATKEY_SHADING_MODEL, aiMode) != aiReturn_SUCCESS) {
        aiMode = aiShadingMode_Phong;
    }

    m_externalAliases = params.root.getAiMaterialParamAliases(aiMode);
    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);

    // Get all textures
    for (auto &textureInfo : params.root.getAiMaterialTextureInfos()) {
        if (params.p_extMaterial->GetTextureCount(textureInfo.aiTextureId) > 0) {
            aiString path;
            aiReturn res = params.p_extMaterial->GetTexture(textureInfo.aiTextureId, 0, &path);

            if (res == aiReturn_SUCCESS) {
                String relconvPath =
                    searchAndReplace(searchAndReplace(path.C_Str(), "\\", "/"), "//", "/");

                m_properties.emplace(textureInfo.textureNameId,
                                     textureManager
                                         .register_asset({Texture::FileLoadParams{
                                             .root = params.root,
                                             .filepath = parentDirPath / relconvPath,
                                             .useMipMaps = true}})
                                         .getLoaded());
            }
        } else {
            m_properties.emplace(textureInfo.textureNameId, textureInfo.defaultTexture.getLoaded());
        }
    }

    // get all properties
    for (auto &propertyInfo : params.root.getAiMaterialPropertyInfos()) {
        std::optional<Variant> value = propertyInfo.extractor(*params.p_extMaterial);
        if (value.has_value()) {
            m_properties[propertyInfo.nameId] = std::move(value.value());
        }
    }
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

void Material::setTexture(StringView colorName, dynasma::FirmPtr<Texture> texture,
                          StringView coordPropertyName)
{
    // erase alias for texture sample
    m_tobeInternalAliases.erase("sample_" + std::string(colorName));

    // add alias for texture coordinate
    m_tobeInternalAliases["coord_" + std::string(colorName)] = std::string(coordPropertyName);

    m_aliases = ParamAliases({{&m_externalAliases}}, m_tobeInternalAliases);

    // set texture
    m_root.getComponent<Renderer>().specifyTextureSampler(colorName);
    m_properties["tex_" + std::string(colorName)] = std::move(texture);
}

void Material::setTexture(StringView colorName, glm::vec4 uniformColor)
{
    // erase alias for texture coordinate
    m_tobeInternalAliases.erase("coord_" + std::string(colorName));

    // add alias for texture sample
    m_tobeInternalAliases["sample_" + std::string(colorName)] = "color_" + std::string(colorName);

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