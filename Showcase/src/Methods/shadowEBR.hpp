#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Function.hpp"

#include "dynasma/standalone.hpp"

#include <iostream>

using namespace Vitrae;

// "Edge Bilinear Range"
struct MethodsShadowEBR : MethodCollection
{
    inline MethodsShadowEBR(ComponentRoot &root) : MethodCollection(root)
    {
        /*
        VERTEX SHADING
        */
        p_vertexMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        FRAGMENT SHADING
        */

        /*auto p_alias =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "position_shadow_view",
                                     .typeInfo = StandardShaderPropertyTypes::FRAGMENT_OUTPUT},
                        PropertySpec{.name = "normal_view",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "ShadowMapSize",
                                     .typeInfo = Variant::getTypeInfo<glm::vec2>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "shadow_alias",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .snippet = R"(
                    void shadowAlias(
                        in vec4 position_shadow_view, in vec3 normal_view,
                        in vec2 ShadowMapSize,
                        out vec3 alias)
                    {
                        vec2 proper_coordinate = (position_shadow_view.xy * 0.5 + 0.5) *
           ShadowMapSize; vec2 diff = (gl_FragCoord.xy - proper_coordinate); vec2 normal_2d =
           normalize(normal_view.xy); float normal_based_offset = dot(normal_2d, diff) + 0.5; alias
           = vec3(normal_2d.xy * sign(normal_based_offset),
           normal_based_offset*normal_based_offset);
                    }
                )",
                .functionName = "shadowAlias"}});*/

        auto p_viewNormal2D =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "position_shadow_view",
                                     .typeInfo = StandardShaderPropertyTypes::FRAGMENT_OUTPUT},
                        PropertySpec{.name = "normal_view",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "ShadowMapSize",
                                     .typeInfo = Variant::getTypeInfo<glm::vec2>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "shadow_alias",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .snippet = R"(
                    void viewNormal2D(
                        in vec3 normal_view,
                        out vec2 normal_view2d)
                    {
                        normal_view2d = normalize(normal_view.xy);
                    }
                )",
                .functionName = "viewNormal2D"}});

        auto p_shadowLightFactor =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "tex_shadow",
                                     .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Texture>>()},
                        PropertySpec{.name = "tex_shadow_alias",
                                     .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Texture>>()},
                        PropertySpec{.name = "position_shadow",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "ebr_strength",
                                     .typeInfo = Variant::getTypeInfo<float>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "light_shadow_factor",
                                     .typeInfo = Variant::getTypeInfo<float>()},
                    },
                .snippet = R"(
                    float inShadowTexel(sampler2D tex_shadow, ivec2 pos, float posz, float offset) {
                        if (texelFetch(tex_shadow, pos, 0).r < posz + offset) {
                            return 0.0;
                        }
                        else {
                            return 1.0;
                        }
                    }

                    void lightShadowFactor(
                        in sampler2D tex_shadow, sampler2D tex_shadow_alias, in vec3 position_shadow,
                        in float ebr_strength,
                        out float light_shadow_factor)
                    {
                        vec2 shadowSize = textureSize(tex_shadow, 0);
                        vec3 alias = texture(tex_shadow_alias, position_shadow.xy).xyz;
                        vec2 diff = normalize(alias.xy) * sqrt(alias.z) / shadowSize;
                        vec2 aa_coord_shadow = position_shadow.xy + diff * ebr_strength;
                        float offset = 0.5 / shadowSize.x;
                        vec2 texelPos = aa_coord_shadow * shadowSize;
                        vec2 topLeftTexelPos = floor(aa_coord_shadow * shadowSize);
                        ivec2 topLeftTexelPosI = ivec2(topLeftTexelPos);
                        vec2 bilinOffset = texelPos - topLeftTexelPos;

                        light_shadow_factor = (
                            inShadowTexel(
                                tex_shadow, topLeftTexelPosI + ivec2(0,0), position_shadow.z, offset
                            ) * (1.0 - bilinOffset.x) + inShadowTexel(
                                tex_shadow, topLeftTexelPosI + ivec2(1,0), position_shadow.z, offset
                            ) * bilinOffset.x
                        ) * (1.0 - bilinOffset.y) + (
                            inShadowTexel(
                                tex_shadow, topLeftTexelPosI + ivec2(0,1), position_shadow.z, offset
                            ) * (1.0 - bilinOffset.x) + inShadowTexel(
                                tex_shadow, topLeftTexelPosI + ivec2(1,1), position_shadow.z, offset
                            ) * bilinOffset.x
                        ) * bilinOffset.y;
                        
                        /*if (light_shadow_factor >= 0.5) {
                            light_shadow_factor = 1.0;
                        }
                        else {
                            light_shadow_factor = 0.0;
                        }*/
                    }
                )",
                .functionName = "lightShadowFactor"}});

        p_fragmentMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_viewNormal2D, p_shadowLightFactor}, .friendlyName = "ShadowEBR"});

        /*
        COMPOSING
        */

        auto p_shadowClear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                                             .displayOutputPropertyName = "shadow_cleared"}});

        auto p_shadowRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .sceneInputPropertyName = "scene",
                .vertexPositionOutputPropertyName = "position_shadow_view",
                .displayInputPropertyName = "shadow_cleared",
                .displayOutputPropertyName = "rendered_shadow",
                .cullingMode = ComposeSceneRender::CullingMode::Frontface,
                .rasterizingMode = ComposeSceneRender::RasterizingMode::DerivationalFillCenters,
            }});
        auto p_shadowTexture =
            dynasma::makeStandalone<ComposeFrameToTexture>(ComposeFrameToTexture::SetupParams{
                .root = root,
                .frameInputPropertyName = "rendered_shadow",
                .depthTextureOutputPropertyName = "tex_shadow",
                .outputTexturePropertySpecs =
                    {
                        ComposeFrameToTexture::OutputTexturePropertySpec{
                            "tex_shadow_normal",
                            PropertySpec{"normal_view2d", Variant::getTypeInfo<glm::vec2>()}},
                    },
                .size = StringId("ShadowMapSize"),
                .channelType = Texture::ChannelType::VEC2_SNORM8,
                .horWrap = Texture::WrappingType::BORDER_COLOR,
                .verWrap = Texture::WrappingType::BORDER_COLOR,
                .minFilter = Texture::FilterType::LINEAR,
                .magFilter = Texture::FilterType::LINEAR,
                .useMipMaps = false});

        // compose method
        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_shadowClear, p_shadowRender, p_shadowTexture},
                .friendlyName = "ShadowEBR"});
    }
};