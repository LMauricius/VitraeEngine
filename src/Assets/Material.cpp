#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

namespace Vitrae
{

Material::Material(const AssimpLoadParams &params)
{
    TextureManager &textureManager = params.root.getComponent<TextureManager>();

    std::filesystem::path parentDirPath = params.sceneFilepath.parent_path();

    // get shading type
    aiShadingMode aiMode;
    if (params.p_extMaterial->Get(AI_MATKEY_SHADING_MODEL, aiMode) != aiReturn_SUCCESS) {
        aiMode = aiShadingMode_Phong;
    }
    const ComponentRoot::AiMaterialShadingInfo &shadingInfo =
        params.root.getAiMaterialShadingInfo(aiMode);

    m_vertexMethod = shadingInfo.vertexMethod;
    m_fragmentMethod = shadingInfo.fragmentMethod;

    // Get all textures
    for (auto &textureInfo : params.root.getAiMaterialTextureInfos()) {
        if (params.p_extMaterial->GetTextureCount(textureInfo.aiTextureId) > 0) {
            aiString path;
            aiReturn res = params.p_extMaterial->GetTexture(textureInfo.aiTextureId, 0, &path);

            if (res == aiReturn_SUCCESS) {
                String relconvPath =
                    searchAndReplace(searchAndReplace(path.C_Str(), "\\", "/"), "//", "/");

                m_textures[textureInfo.textureNameId] = textureManager.register_asset(
                    {Texture::FileLoadParams{.root = params.root,
                                             .filepath = parentDirPath / relconvPath,
                                             .useMipMaps = true}});
            }
        } else {
            m_textures[textureInfo.textureNameId] = textureInfo.defaultTexture.getLoaded();
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

void Material::setMethods(dynasma::FirmPtr<Method<ShaderTask>> vertexMethod,
                          dynasma::FirmPtr<Method<ShaderTask>> fragmentMethod)
{
    m_vertexMethod = vertexMethod;
    m_fragmentMethod = fragmentMethod;
}

void Material::setProperty(StringId key, const Variant &value)
{
    m_properties[key] = value;
}

void Material::setProperty(StringId key, Variant &&value)
{
    m_properties[key] = std::move(value);
}

dynasma::FirmPtr<Method<ShaderTask>> Material::getVertexMethod() const
{
    return m_vertexMethod;
}

dynasma::FirmPtr<Method<ShaderTask>> Material::getFragmentMethod() const
{
    return m_fragmentMethod;
}

const StableMap<StringId, dynasma::FirmPtr<Texture>> &Material::getTextures() const
{
    return m_textures;
}

const StableMap<StringId, Variant> &Material::getProperties() const
{
    return m_properties;
}

} // namespace Vitrae