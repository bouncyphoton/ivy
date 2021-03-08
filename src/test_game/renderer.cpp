#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include "ivy/graphics/texture2d.h"
#include "ivy/entity/components/transform.h"
#include "ivy/entity/components/model.h"
#include "ivy/entity/components/camera.h"
#include <glm/gtc/matrix_transform.hpp>

// TODO: multiple render passes
// TODO: compute pass

using namespace ivy;

struct MVP {
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 normal;
};

struct LightingPerFrame {
    alignas(16) glm::mat4 invProj;
    alignas(16) glm::mat4 invView;
    alignas(8) glm::vec2 resolution;
    alignas(4) u32 debugMode;
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

    // Build our graphics pass
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
                    .addColorAttachment(gfx::GraphicsPass::SwapchainName, 0)
                    .addInputAttachmentDescriptor(0, 0, "diffuse")
                    .addInputAttachmentDescriptor(0, 1, "normal")
                    .addInputAttachmentDescriptor(0, 2, "depth")
                    .addUniformBufferDescriptor(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
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
    gfx::GraphicsPass &pass = passes_.front();

    cmd.executeGraphicsPass(device_, pass, [&]() {
        u32 subpassIdx = 0;

        // This data will be used in multiple subpasses
        MVP mvpData = {};
        // TODO: get width and height from output attachment
        f32 frameWidth = static_cast<f32>(device_.getSwapchainExtent().width);
        f32 frameHeight = static_cast<f32>(device_.getSwapchainExtent().height);
        f32 aspectRatio = frameWidth / frameHeight;

        // Subpass 0, g-buffer
        {
            // Bind graphics pipeline
            cmd.bindGraphicsPipeline(pass, subpassIdx);

            // Set MVP data on CPU
            mvpData.proj = glm::mat4(1);
            mvpData.view = glm::mat4(1);

            // Set projection and view matrix from first entity with camera component
            auto cameraIt = std::find_if(entities.begin(), entities.end(), [](const Entity & e) {
                return e.getComponent<Camera>() && e.getComponent<Transform>();
            });
            if (cameraIt != entities.end()) {
                Camera camera = *cameraIt->getComponent<Camera>();
                Transform transform = *cameraIt->getComponent<Transform>();

                mvpData.proj = glm::perspective(camera.getFovY(), aspectRatio, camera.getNearPlane(), camera.getFarPlane());
                mvpData.view = glm::lookAt(transform.getPosition(), transform.getPosition() + transform.getForward(), Transform::UP);
            } else {
                Log::warn("No entities in the scene have a Camera and Transform component");
            }

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
                        gfx::DescriptorSet mvpSet(pass, subpassIdx, 0);
                        mvpSet.setUniformBuffer(0, mvpData);
                        cmd.setDescriptorSet(device_, pass, mvpSet);

                        // Material data
                        const gfx::Material &mat = mesh.getMaterial();
                        gfx::DescriptorSet materialSet(pass, subpassIdx, 1);
                        materialSet.setTexture(0, mat.getDiffuseTexture());
                        cmd.setDescriptorSet(device_, pass, materialSet);

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
            cmd.bindGraphicsPipeline(pass, subpassIdx);

            // Set our input attachments in descriptor set and bind it
            {
                gfx::DescriptorSet inputAttachmentsSet(pass, subpassIdx, 0);
                inputAttachmentsSet.setInputAttachment(0, "diffuse");
                inputAttachmentsSet.setInputAttachment(1, "normal");
                inputAttachmentsSet.setInputAttachment(2, "depth");
                cmd.setDescriptorSet(device_, pass, inputAttachmentsSet);
            }

            // Set per frame descriptors
            {
                LightingPerFrame perFrame = {};
                perFrame.invProj = glm::inverse(mvpData.proj);
                perFrame.invView = glm::inverse(mvpData.view);
                perFrame.resolution = glm::vec2(frameWidth, frameHeight);
                perFrame.debugMode = static_cast<u32>(debug_mode);

                gfx::DescriptorSet perFrameSet(pass, subpassIdx, 1);
                perFrameSet.setUniformBuffer(0, perFrame);
                cmd.setDescriptorSet(device_, pass, perFrameSet);
            }

            // Draw our fullscreen triangle
            cmd.draw(3, 1, 0, 0);
        }
    });

    device_.endFrame();
}
