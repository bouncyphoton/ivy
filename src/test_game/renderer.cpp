#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include "ivy/scene/components/transform.h"
#include "ivy/scene/components/model.h"
#include "ivy/scene/components/camera.h"
#include "ivy/scene/components/light.h"
#include <glm/gtc/matrix_transform.hpp>

// TODO: compute pass

using namespace ivy;

struct PerLightDirectionalShadowPass {
    alignas(16) glm::mat4 viewProjection;
};

struct PerLightPointShadowPass {
    alignas(16) glm::mat4 viewProjectionMatrices[6];
    alignas(16) glm::vec3 lightPosition;
    alignas(8) glm::vec2 nearAndFarPlanes;
    alignas(4) u32 lightIndex;
};

struct PerMeshShadowPass {
    alignas(16) glm::mat4 model;
};

struct PerMeshGBufferPass {
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 normal;
};

struct PerFrameLightingPass {
    alignas(16) glm::mat4 invProj;
    alignas(16) glm::mat4 invView;
    alignas(8) glm::vec2 resolution;
    alignas(4) u32 debugMode;
};

struct PerLightLightingPass {
    alignas(16) glm::mat4 viewProjection;
    alignas(16) glm::vec4 directionAndShadowBias; // xyz = direction/position, w = shadow bias
    alignas(16) glm::vec4 colorAndIntensity;      // xyz = color, w = intensity
    alignas(16) glm::vec4 shadowViewportNormalized; // shadowViewportNormalized or (near, far, 0, 0)

    // these could be packed into one
    alignas(4) u32 lightType;
    alignas(4) u32 shadowIndex;
};

Renderer::Renderer(gfx::RenderDevice &render_device)
    : device_(render_device) {
    LOG_CHECKPOINT();

    // TODO: shader files should be a part of resource manager

    linearSampler_ = device_.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
    nearestSampler_ = device_.createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    // Find best format for depth
    VkFormat depthFormat = device_.getFirstSupportedFormat({
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT},
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Create directional shadow texture that we can render into
    directionalLightShadowAtlas_ = gfx::TextureBuilder(device_)
                                   .setExtent2D(shadowMapSizeDirectional_, shadowMapSizeDirectional_)
                                   .setFormat(depthFormat)
                                   .setImageAspect(VK_IMAGE_ASPECT_DEPTH_BIT)
                                   .setAdditionalUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                                   .build();

    // Create point shadow cubemap textures
    pointLightShadowAtlas_ = gfx::TextureBuilder(device_)
                             .setExtentCubemap(shadowMapSizePoint_)
                             .setFormat(depthFormat)
                             .setImageAspect(VK_IMAGE_ASPECT_DEPTH_BIT)
                             .setAdditionalUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                             .setArrayLength(maxShadowCastingPointLights_)
                             .build();

    // Directional light shadow map pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .setExtent(shadowMapSizeDirectional_, shadowMapSizeDirectional_)
        .addAttachment("depth", *directionalLightShadowAtlas_)
        .addSubpass("shadow_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/shadow_directional.vert.spv")
                    // trivial fragment shader
                    .addVertexDescription(gfx::VertexP3N3UV2::getBindingDescriptions(),
                                          gfx::VertexP3N3UV2::getAttributeDescriptions())
                    .addUniformBufferDescriptor(0, 0, VK_SHADER_STAGE_VERTEX_BIT)
                    .addUniformBufferDescriptor(1, 0, VK_SHADER_STAGE_VERTEX_BIT)
                    .addDepthAttachment("depth")
                    .build()
                   )
        .build()
    );

    // Point light shadow map pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .setExtent(shadowMapSizePoint_, shadowMapSizePoint_)
        .addAttachment("depth", *pointLightShadowAtlas_)
        .addSubpass("shadow_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/shadow_point.vert.spv")
                    .addShader(gfx::Shader::StageEnum::GEOMETRY, "../assets/shaders/shadow_point.geom.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/shadow_point.frag.spv")
                    .addVertexDescription(gfx::VertexP3N3UV2::getBindingDescriptions(),
                                          gfx::VertexP3N3UV2::getAttributeDescriptions())
                    .addUniformBufferDescriptor(0, 0, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                    .addUniformBufferDescriptor(1, 0, VK_SHADER_STAGE_VERTEX_BIT)
                    .addDepthAttachment("depth")
                    .build()
                   )
        .build()
    );

    // Main gbuffer and shading pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .addAttachmentSwapchain()
        .addAttachment("diffuse", VK_FORMAT_R8G8B8A8_UNORM)
        .addAttachment("normal", VK_FORMAT_R16G16B16A16_SFLOAT)
        .addAttachment("depth", depthFormat)
        .addSubpass("gbuffer_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/gbuffer.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/gbuffer.frag.spv")
                    .addVertexDescription(gfx::VertexP3N3UV2::getBindingDescriptions(),
                                          gfx::VertexP3N3UV2::getAttributeDescriptions())
                    .addColorAttachment("diffuse", 0)
                    .addColorAttachment("normal", 1)
                    .addDepthAttachment("depth")
                    .addUniformBufferDescriptor(0, 0, VK_SHADER_STAGE_VERTEX_BIT)
                    .addTextureDescriptor(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build()
                   )
        .addSubpass("lighting_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/lighting.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/lighting.frag.spv")
                    .setColorBlending(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE)
                    .addColorAttachment(gfx::GraphicsPass::SwapchainName, 0)
                    .addInputAttachmentDescriptor(0, 0, "diffuse")
                    .addInputAttachmentDescriptor(0, 1, "normal")
                    .addInputAttachmentDescriptor(0, 2, "depth")
                    .addUniformBufferDescriptor(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .addTextureDescriptor(1, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .addTextureDescriptor(1, 2, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .addUniformBufferDescriptor(2, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build()
                   )
        .addSubpassDependency(gfx::GraphicsPass::SwapchainName, "gbuffer_pass",
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                              0,
                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        .addSubpassDependency("gbuffer_pass", "lighting_pass",
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                              VK_ACCESS_SHADER_READ_BIT)
        .build()
    );
}

Renderer::~Renderer() {
    LOG_CHECKPOINT();
}

void Renderer::render(Scene &scene, DebugMode debug_mode) {
    device_.beginFrame();
    gfx::CommandBuffer cmd = device_.getCommandBuffer();
    gfx::GraphicsPass &shadowPassDirectional = passes_.at(0);
    gfx::GraphicsPass &shadowPassPoint = passes_.at(1);
    gfx::GraphicsPass &lightingPass = passes_.at(2);

    // Find camera in entities
    EntityHandle cameraEntity = scene.findEntityWithAllComponents<Camera, Transform>();
    Camera camera;
    Transform cameraTransform;
    if (!cameraEntity) {
        Log::warn("No entities in the scene have a Camera and Transform component");
    } else {
        camera = *cameraEntity->getComponent<Camera>();
        cameraTransform = *cameraEntity->getComponent<Transform>();
    }

    // Transition point shadow map back from previous frame for writing
    {
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        memoryBarrier.srcAccessMask = 0;
        memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.image = pointLightShadowAtlas_->getImage();
        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        memoryBarrier.subresourceRange.levelCount = 1;
        memoryBarrier.subresourceRange.layerCount = pointLightShadowAtlas_->getLayers();

        cmd.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                            0, 0, nullptr, 0, nullptr,
                            1, &memoryBarrier);
    }

    // Point light shadow pass
    cmd.executeGraphicsPass(device_, shadowPassPoint, [&]() {
        cmd.bindGraphicsPipeline(shadowPassPoint, 0);
        cmd.setViewport(0, 0, shadowMapSizePoint_, shadowMapSizePoint_);

        std::vector<EntityHandle> lightEntities = scene.findEntitiesWithAllComponents<Transform, PointLight>();
        std::vector<EntityHandle> shadowCasters = scene.findEntitiesWithAllComponents<Transform, Model>();

        // TODO: sort by distance from camera

        // Render shadow maps
        numShadowsPoint_ = 0;
        for (auto &lightEntity : lightEntities) {
            if (numShadowsPoint_ >= maxShadowCastingPointLights_) {
                break;
            }

            Transform *lightTransform = lightEntity->getComponent<Transform>();
            PointLight *light = lightEntity->getComponent<PointLight>();
            if (!light->castsShadows()) {
                continue;
            }

            // View projection matrices
            glm::vec3 p = lightTransform->getPosition();
            glm::mat4 vpMatrices[6] = {
                glm::lookAt(p, p + glm::vec3( 1,  0,  0), glm::vec3(0, -1, 0)),
                glm::lookAt(p, p + glm::vec3(-1,  0,  0), glm::vec3(0, -1, 0)),
                glm::lookAt(p, p + glm::vec3( 0,  1,  0), glm::vec3(0, 0, 1)),
                glm::lookAt(p, p + glm::vec3( 0, -1,  0), glm::vec3(0, 0, -1)),
                glm::lookAt(p, p + glm::vec3( 0,  0,  1), glm::vec3(0, -1, 0)),
                glm::lookAt(p, p + glm::vec3( 0,  0, -1), glm::vec3(0, -1, 0)),
            };

            glm::mat4 projection = glm::perspective(glm::half_pi<f32>(), 1.0f, light->getNearPlane(), light->getFarPlane());

            // Send view projection matrix for face to shader
            PerLightPointShadowPass perLight = {};
            for (u32 i = 0; i < COUNTOF(vpMatrices); ++i) {
                perLight.viewProjectionMatrices[i] = projection * vpMatrices[i];
            }
            perLight.lightPosition = p;
            perLight.nearAndFarPlanes = glm::vec2(light->getNearPlane(), light->getFarPlane());
            perLight.lightIndex = numShadowsPoint_;

            gfx::DescriptorSet perLightSet(shadowPassPoint, 0, 0);
            perLightSet.setUniformBuffer(0, perLight);
            cmd.setDescriptorSet(device_, shadowPassPoint, perLightSet);

            // Render into shadow map
            // TODO: set a max range
            for (auto &caster : shadowCasters) {
                Transform *transform = caster->getComponent<Transform>();
                Model *model = caster->getComponent<Model>();

                PerMeshShadowPass perMesh = {};
                perMesh.model = transform->getModelMatrix();

                // Iterate over meshes in model at max lod
                for (const gfx::Mesh &mesh : model->getMeshes(ResourceManager::MAX_LOD)) {
                    // Send to shader
                    gfx::DescriptorSet perMeshSet(shadowPassPoint, 0, 1);
                    perMeshSet.setUniformBuffer(0, perMesh);
                    cmd.setDescriptorSet(device_, shadowPassPoint, perMeshSet);

                    // Draw this mesh
                    mesh.getGeometry().draw(cmd);
                }
            }

            ++numShadowsPoint_;
        }
    });

    // Transition point shadow map for reading
    {
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        memoryBarrier.image = pointLightShadowAtlas_->getImage();
        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        memoryBarrier.subresourceRange.levelCount = 1;
        memoryBarrier.subresourceRange.layerCount = pointLightShadowAtlas_->getLayers();

        cmd.pipelineBarrier(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                            1, &memoryBarrier);
    }

    // Transition directional shadow map back from previous frame for writing
    {
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        memoryBarrier.srcAccessMask = 0;
        memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.image = directionalLightShadowAtlas_->getImage();
        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        memoryBarrier.subresourceRange.levelCount = 1;
        memoryBarrier.subresourceRange.layerCount = 1;

        cmd.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                            0, 0, nullptr, 0, nullptr,
                            1, &memoryBarrier);
    }

    // Directional light shadow pass
    cmd.executeGraphicsPass(device_, shadowPassDirectional, [&]() {
        cmd.bindGraphicsPipeline(shadowPassDirectional, 0);

        std::vector<EntityHandle> lightEntities = scene.findEntitiesWithAnyComponents<DirectionalLight>();

        // Count number of shadow casting lights
        numShadowsDirectional_ = 0;
        for (auto &lightEntity : lightEntities) {
            DirectionalLight *light = lightEntity->getComponent<DirectionalLight>();
            if (light && light->castsShadows()) {
                ++numShadowsDirectional_;
            }
        }

        // No directional lights, return
        if (numShadowsDirectional_ == 0) {
            return;
        }

        shadowsPerSideDirectional_ = (u32) std::ceil(std::sqrt(numShadowsDirectional_));
        shadowSizeDirectional_ = (u32) shadowMapSizeDirectional_ / shadowsPerSideDirectional_;
        u32 shadowIdx = 0;

        // Render shadow maps
        for (auto &lightEntity : lightEntities) {
            DirectionalLight *light = lightEntity->getComponent<DirectionalLight>();
            if (light && !light->castsShadows()) {
                continue;
            }

            // Calculate viewport
            glm::vec4 viewport = getShadowViewport(shadowIdx);
            cmd.setViewport(viewport.x, viewport.y, viewport.z, viewport.w);

            // Light data
            PerLightDirectionalShadowPass perLight = {};
            perLight.viewProjection = light->calculateViewProjectionMatrix(cameraTransform, camera);

            // Send to shader
            gfx::DescriptorSet perLightSet(shadowPassDirectional, 0, 0);
            perLightSet.setUniformBuffer(0, perLight);
            cmd.setDescriptorSet(device_, shadowPassDirectional, perLightSet);

            // Go over entities and draw
            for (EntityHandle &entity : scene.findEntitiesWithAllComponents<Transform, Model>()) {
                Transform *transform = entity->getComponent<Transform>();
                Model *model = entity->getComponent<Model>();

                PerMeshShadowPass perMesh = {};
                perMesh.model = transform->getModelMatrix();

                // Iterate over meshes in model
                for (const gfx::Mesh &mesh : model->getMeshes()) {
                    // Send to shader
                    gfx::DescriptorSet perMeshSet(shadowPassDirectional, 0, 1);
                    perMeshSet.setUniformBuffer(0, perMesh);
                    cmd.setDescriptorSet(device_, shadowPassDirectional, perMeshSet);

                    // Draw this mesh
                    mesh.getGeometry().draw(cmd);
                }
            }

            ++shadowIdx;
        }
    });

    // Transition directional shadow map for reading
    {
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        memoryBarrier.image = directionalLightShadowAtlas_->getImage();
        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        memoryBarrier.subresourceRange.levelCount = 1;
        memoryBarrier.subresourceRange.layerCount = 1;

        cmd.pipelineBarrier(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                            1, &memoryBarrier);
    }

    // Main lighting pass
    cmd.executeGraphicsPass(device_, lightingPass, [&]() {
        u32 subpassIdx = 0;

        // This data will be used in multiple subpasses
        PerMeshGBufferPass mvpData = {};
        f32 frameWidth = static_cast<f32>(lightingPass.getExtent().width);
        f32 frameHeight = static_cast<f32>(lightingPass.getExtent().height);
        f32 aspectRatio = frameWidth / frameHeight;

        // Subpass 0, g-buffer
        {
            // Bind graphics pipeline
            cmd.bindGraphicsPipeline(lightingPass, subpassIdx);

            // Set MVP data on CPU
            mvpData.proj = glm::perspective(camera.getFovY(), aspectRatio, camera.getNearPlane(), camera.getFarPlane());
            mvpData.view = glm::lookAt(cameraTransform.getPosition(),
                                       cameraTransform.getPosition() + cameraTransform.getForward(), Transform::UP);

            // Go over entities and draw
            for (EntityHandle &entity : scene.findEntitiesWithAllComponents<Transform, Model>()) {
                Transform *transform = entity->getComponent<Transform>();
                Model *model         = entity->getComponent<Model>();

                // Set the model and normal matrix
                mvpData.model = transform->getModelMatrix();
                mvpData.normal = glm::inverse(glm::transpose(mvpData.model));

                // Iterate over meshes in model
                for (const gfx::Mesh &mesh : model->getMeshes()) {
                    // Put MVP data in a descriptor set and bind it
                    gfx::DescriptorSet mvpSet(lightingPass, subpassIdx, 0);
                    mvpSet.setUniformBuffer(0, mvpData);
                    cmd.setDescriptorSet(device_, lightingPass, mvpSet);

                    // Material data
                    const gfx::Material &mat = mesh.getMaterial();
                    gfx::DescriptorSet materialSet(lightingPass, subpassIdx, 1);
                    materialSet.setTexture(0, mat.getDiffuseTexture(), linearSampler_);
                    cmd.setDescriptorSet(device_, lightingPass, materialSet);

                    // Draw this mesh
                    mesh.getGeometry().draw(cmd);
                }
            }
        }

        // Subpass 1, lighting
        {
            ++subpassIdx;
            cmd.nextSubpass();

            // Bind graphics pipeline
            cmd.bindGraphicsPipeline(lightingPass, subpassIdx);

            // Set our input attachments in descriptor set and bind it
            {
                gfx::DescriptorSet inputAttachmentsSet(lightingPass, subpassIdx, 0);
                inputAttachmentsSet.setInputAttachment(0, "diffuse");
                inputAttachmentsSet.setInputAttachment(1, "normal");
                inputAttachmentsSet.setInputAttachment(2, "depth");
                cmd.setDescriptorSet(device_, lightingPass, inputAttachmentsSet);
            }

            // Set per frame descriptors
            {
                PerFrameLightingPass perFrame = {};
                perFrame.invProj = glm::inverse(mvpData.proj);
                perFrame.invView = glm::inverse(mvpData.view);
                perFrame.resolution = glm::vec2(frameWidth, frameHeight);
                perFrame.debugMode = static_cast<u32>(debug_mode);

                gfx::DescriptorSet perFrameSet(lightingPass, subpassIdx, 1);
                perFrameSet.setUniformBuffer(0, perFrame);
                perFrameSet.setTexture(1, *directionalLightShadowAtlas_, nearestSampler_);
                perFrameSet.setTexture(2, *pointLightShadowAtlas_, nearestSampler_);
                cmd.setDescriptorSet(device_, lightingPass, perFrameSet);
            }

            // TODO: this depends on the order of lights being the same

            // Draw lights
            u32 dirShadowIdx = 0;
            u32 pntShadowIdx = 0;
            for (auto &lightEntity : scene.findEntitiesWithAnyComponents<DirectionalLight, PointLight>()) {
                DirectionalLight *dirLight  = lightEntity->getComponent<DirectionalLight>();
                PointLight       *pntLight  = lightEntity->getComponent<PointLight>();
                Transform        *transform = lightEntity->getComponent<Transform>();

                PerLightLightingPass perLight = {};

                if (dirLight) {
                    perLight.viewProjection = dirLight->calculateViewProjectionMatrix(cameraTransform, camera);
                    perLight.directionAndShadowBias = glm::vec4(dirLight->getDirection(), dirLight->getShadowBias());
                    perLight.colorAndIntensity = glm::vec4(dirLight->getColor(), dirLight->getIntensity());
                    perLight.lightType = LightType::DIRECTIONAL;
                    if (dirLight->castsShadows()) {
                        perLight.shadowViewportNormalized = getShadowViewport(dirShadowIdx) / (f32) shadowMapSizeDirectional_;
                        perLight.shadowIndex = dirShadowIdx;
                        ++dirShadowIdx;
                    } else {
                        perLight.shadowIndex = -1;
                    }
                } else if (pntLight && transform) {
                    // TODO: better names for variables that are shared/interpreted differently depending on light type

                    perLight.directionAndShadowBias = glm::vec4(transform->getPosition(), pntLight->getShadowBias());
                    perLight.colorAndIntensity = glm::vec4(pntLight->getColor(), pntLight->getIntensity());
                    perLight.shadowViewportNormalized = glm::vec4(pntLight->getNearPlane(), pntLight->getFarPlane(), 0, 0);
                    perLight.lightType = LightType::POINT;
                    if (pntLight->castsShadows() && pntShadowIdx < maxShadowCastingPointLights_) {
                        perLight.shadowIndex = pntShadowIdx;
                        ++pntShadowIdx;
                    } else {
                        perLight.shadowIndex = -1;
                    }
                } else {
                    continue;
                }

                gfx::DescriptorSet perLightSet(lightingPass, subpassIdx, 2);
                perLightSet.setUniformBuffer(0, perLight);
                cmd.setDescriptorSet(device_, lightingPass, perLightSet);

                // Draw our fullscreen triangle
                cmd.draw(3, 1, 0, 0);

                // If we're using a debug mode, only draw once
                // Otherwise, we additively blend debug rendering for each light
                if (debug_mode != DebugMode::FULL) {
                    break;
                }
            }
        }
    });

    device_.endFrame();
}

glm::vec4 Renderer::getShadowViewport(ivy::u32 shadow_idx) const {
    return glm::vec4(
               (shadow_idx % shadowsPerSideDirectional_) * shadowSizeDirectional_,
               (shadow_idx / shadowsPerSideDirectional_) * shadowSizeDirectional_,
               shadowSizeDirectional_,
               shadowSizeDirectional_
           );
}
