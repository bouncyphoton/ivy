#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include "ivy/entity/components/transform.h"
#include "ivy/entity/components/model.h"
#include "ivy/entity/components/camera.h"
#include <glm/gtc/matrix_transform.hpp>

// TODO: textures
// TODO: multiple render passes
// TODO: compute pass

using namespace ivy;

struct MVP {
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 model;
};

Renderer::Renderer(gfx::RenderDevice &render_device)
    : device_(render_device) {
    LOG_CHECKPOINT();

    // TODO: search for supported depth attachments
    // TODO: shader files should be a part of resource manager

    // Build our graphics pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .addAttachmentSwapchain()
        .addAttachment("albedo", VK_FORMAT_R8G8B8A8_UNORM)
        .addAttachment("position", VK_FORMAT_R16G16B16A16_SFLOAT)
        .addAttachment("depth", VK_FORMAT_D32_SFLOAT_S8_UINT)
        .addSubpass("gbuffer_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/gbuffer.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/gbuffer.frag.spv")
                    .addVertexDescription(gfx::VertexP3C3::getBindingDescriptions(), gfx::VertexP3C3::getAttributeDescriptions())
                    .addColorAttachment("albedo", 0)
                    .addColorAttachment("position", 1)
                    .addDepthAttachment("depth")
                    .addDescriptor(0, 0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    .build()
                   )
        .addSubpass("lighting_pass",
                    gfx::SubpassBuilder()
                    .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/lighting.vert.spv")
                    .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/lighting.frag.spv")
                    .addInputAttachment("albedo", 0, 0)
                    .addInputAttachment("position", 0, 1)
                    .addColorAttachment(gfx::GraphicsPass::SwapchainName, 0)
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

void Renderer::render(const std::vector<Entity> &entities) {
    device_.beginFrame();
    gfx::CommandBuffer cmd = device_.getCommandBuffer();
    gfx::GraphicsPass &pass = passes_.front();

    cmd.executeGraphicsPass(device_, pass, [&]() {
        u32 subpassIdx = 0;

        // Subpass 0, g-buffer
        {
            // Bind graphics pipeline
            cmd.bindGraphicsPipeline(pass, subpassIdx);

            // TODO: it would make sense to separate per-frame and per-draw descriptor sets
            // TODO: get aspect ratio for output attachment
            f32 aspectRatio = device_.getSwapchainExtent().width / (f32) device_.getSwapchainExtent().height;

            // Set MVP data on CPU
            MVP mvpData = {};
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
                    // Set the model matrix
                    mvpData.model = transform->getModelMatrix();

                    // Put MVP data in a descriptor set and bind it
                    gfx::DescriptorSet mvpSet(pass, subpassIdx, 0);
                    mvpSet.setUniformBuffer(0, mvpData);
                    cmd.setDescriptorSet(device_, pass, mvpSet);

                    // Draw our model
                    model->draw(cmd);
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
            gfx::DescriptorSet inputAttachmentsSet(pass, subpassIdx, 0);
            inputAttachmentsSet.setInputAttachment(0, "albedo");
            inputAttachmentsSet.setInputAttachment(1, "position");
            cmd.setDescriptorSet(device_, pass, inputAttachmentsSet);

            // Draw our fullscreen triangle
            cmd.draw(3, 1, 0, 0);
        }
    });

    device_.endFrame();
}
