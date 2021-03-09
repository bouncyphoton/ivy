#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include "ivy/graphics/texture2d.h"
#include "ivy/entity/components/transform.h"
#include "ivy/entity/components/model.h"
#include "ivy/entity/components/camera.h"
#include "ivy/entity/components/light.h"
#include <glm/gtc/matrix_transform.hpp>

// TODO: compute pass

using namespace ivy;

struct PerLightShadowPass {
    alignas(16) glm::mat4 viewProjection;
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
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
    alignas(4) f32 intensity;
    alignas(4) f32 shadowBias;
};

Renderer::Renderer(gfx::RenderDevice &render_device)
    : device_(render_device) {
    LOG_CHECKPOINT();

    // TODO: shader files should be a part of resource manager

    // Find best format for depth
    VkFormat depthFormat = device_.getFirstSupportedFormat({
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT},
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Shadow map pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .setExtent(2048, 2048)
        .addAttachment("depth", depthFormat, VK_IMAGE_USAGE_SAMPLED_BIT)
        .addSubpass("shadow_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/shadow.vert.spv")
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

    // Create a sampler for our shadow map
    shadowSampler_ = device_.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                           VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                                           VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

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
                    // TODO: additive blending
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/lighting.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/lighting.frag.spv")
                    .addColorAttachment(gfx::GraphicsPass::SwapchainName, 0)
                    .addInputAttachmentDescriptor(0, 0, "diffuse")
                    .addInputAttachmentDescriptor(0, 1, "normal")
                    .addInputAttachmentDescriptor(0, 2, "depth")
                    .addUniformBufferDescriptor(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .addTextureDescriptor(1, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
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

void Renderer::render(const std::vector<Entity> &entities, DebugMode debug_mode) {
    device_.beginFrame();
    gfx::CommandBuffer cmd = device_.getCommandBuffer();
    gfx::GraphicsPass &shadowPass = passes_.at(0);
    gfx::GraphicsPass &lightingPass = passes_.at(1);

    // Find camera in entities
    Camera camera;
    Transform cameraTransform;

    // Set projection and view matrix from first entity with camera component
    auto cameraIt = std::find_if(entities.begin(), entities.end(), [](const Entity & e) {
        return e.getComponent<Camera>() && e.getComponent<Transform>();
    });
    if (cameraIt != entities.end()) {
        camera = *cameraIt->getComponent<Camera>();
        cameraTransform = *cameraIt->getComponent<Transform>();
    } else {
        Log::warn("No entities in the scene have a Camera and Transform component");
    }

    // Transition shadow map back from previous frame for writing
    {
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        memoryBarrier.srcAccessMask = 0;
        memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.image = device_.getFramebuffer(shadowPass).getImage("depth");
        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        memoryBarrier.subresourceRange.levelCount = 1;
        memoryBarrier.subresourceRange.layerCount = 1;

        cmd.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                            0, 0, nullptr, 0, nullptr,
                            1, &memoryBarrier);
    }

    // Shadow pass
    cmd.executeGraphicsPass(device_, shadowPass, [&]() {
        cmd.bindGraphicsPipeline(shadowPass, 0);

        // TODO: share shadow map across lights

        // TODO: get by component type or something ?
        for (const auto &lightEntity : entities) {
            DirectionalLight *light = lightEntity.getComponent<DirectionalLight>();
            if (!light || !light->castsShadows()) {
                continue;
            }

            // Light data
            PerLightShadowPass perLight = {};
            perLight.viewProjection = light->calculateViewProjectionMatrix(cameraTransform, camera);

            // Send to shader
            gfx::DescriptorSet perLightSet(shadowPass, 0, 0);
            perLightSet.setUniformBuffer(0, perLight);
            cmd.setDescriptorSet(device_, shadowPass, perLightSet);

            // Go over entities and draw
            for (const auto &entity : entities) {
                Transform *transform = entity.getComponent<Transform>();
                Model *model = entity.getComponent<Model>();

                if (transform && model) {
                    PerMeshShadowPass perMesh = {};
                    perMesh.model = transform->getModelMatrix();

                    // Iterate over meshes in model
                    for (const gfx::Mesh &mesh : model->getMeshes()) {
                        // Send to shader
                        gfx::DescriptorSet perMeshSet(shadowPass, 0, 1);
                        perMeshSet.setUniformBuffer(0, perMesh);
                        cmd.setDescriptorSet(device_, shadowPass, perMeshSet);

                        // Draw this mesh
                        mesh.getGeometry().draw(cmd);
                    }
                }
            }
        }
    });

    // Transition shadow map for reading
    {
        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        memoryBarrier.image = device_.getFramebuffer(shadowPass).getImage("depth");
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
            for (const auto &entity : entities) {
                Transform *transform = entity.getComponent<Transform>();
                Model *model         = entity.getComponent<Model>();

                if (transform && model) {
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
                        materialSet.setTexture(0, mat.getDiffuseTexture());
                        cmd.setDescriptorSet(device_, lightingPass, materialSet);

                        // Draw this mesh
                        mesh.getGeometry().draw(cmd);
                    }
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
                perFrameSet.setTexture(1, device_.getFramebuffer(shadowPass).getView("depth"), shadowSampler_);
                cmd.setDescriptorSet(device_, lightingPass, perFrameSet);
            }

            // Draw lights
            for (const auto &lightEntity : entities) {
                DirectionalLight *light = lightEntity.getComponent<DirectionalLight>();
                if (!light) {
                    continue;
                }

                PerLightLightingPass perLight = {};
                perLight.viewProjection = light->calculateViewProjectionMatrix(cameraTransform, camera);
                perLight.direction = light->getDirection();
                perLight.color = light->getColor();
                perLight.intensity = light->getIntensity();
                perLight.shadowBias = light->getShadowBias();

                gfx::DescriptorSet perLightSet(lightingPass, subpassIdx, 2);
                perLightSet.setUniformBuffer(0, perLight);
                cmd.setDescriptorSet(device_, lightingPass, perLightSet);

                // Draw our fullscreen triangle
                cmd.draw(3, 1, 0, 0);
            }
        }
    });

    device_.endFrame();
}
