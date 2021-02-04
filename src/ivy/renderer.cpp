#include "renderer.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include "ivy/entity/components/transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

namespace ivy {

// TODO: textures
// TODO: multiple render passes

// TODO: remove temp vertices
static std::vector<gfx::VertexP3C3> vertices = {
    gfx::VertexP3C3({-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}),
    gfx::VertexP3C3({ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}),
    gfx::VertexP3C3({-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}),
    gfx::VertexP3C3({ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}),

    gfx::VertexP3C3({-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}),
    gfx::VertexP3C3({ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}),
    gfx::VertexP3C3({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}),
    gfx::VertexP3C3({ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}),
};

static std::vector<u32> indices = {
    0, 1, 2, 2, 1, 3,
    2, 3, 4, 4, 3, 5,
    4, 5, 6, 6, 5, 7,
    6, 7, 0, 0, 7, 1,
    1, 7, 3, 3, 7, 5,
    6, 0, 4, 4, 0, 2
};

struct MVP {
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 model;
};

Renderer::Renderer(const Options &options, const Platform &platform)
    : device_(options, platform), mesh_(device_, vertices, indices) {
    LOG_CHECKPOINT();

    // TODO: make it nicer to add a depth attachment
    // TODO: search for supported depth attachments

    // Build our graphics pass
    passes_.emplace_back(
        gfx::GraphicsPassBuilder(device_)
        .addAttachmentSwapchain()
        .addAttachment("albedo", VK_FORMAT_R8G8B8A8_UNORM)
        .addAttachment("position", VK_FORMAT_R16G16B16A16_SFLOAT)
        .addAttachment("depth", VK_FORMAT_D32_SFLOAT_S8_UINT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                       VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
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

    // TEMP
    for (u32 i = 0; i < 10; ++i) {
        entities_.emplace_back();
        entities_.back().setComponent<Transform>();
    }
}

Renderer::~Renderer() {
    LOG_CHECKPOINT();
}

void Renderer::render() {
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

            // Set MVP data on CPU
            MVP mvpData = {};
            mvpData.proj = glm::perspective(glm::half_pi<f32>(), 800.0f / 600.0f, 0.1f, 100.0f);
            mvpData.view = glm::lookAt(glm::vec3(0, 1, 5), glm::vec3(0), glm::vec3(0, 1, 0));

            for (u32 i = 0; i < entities_.size(); ++i) {
                const Entity &entity = entities_[i];

                if (Transform *transform = entity.getComponent<Transform>()) {
                    // Update the position for this entity
                    f32 time = (f32) glfwGetTime();
                    f32 x = i - entities_.size() * 0.5f + 0.5f;
                    transform->setPosition(glm::vec3(x, sinf(x + time), 0));
                    transform->setOrientation(glm::vec3(0, time, 0));
                    transform->setScale(glm::vec3(cosf(x + time) * 0.5f + 0.5f));

                    // Set the model matrix
                    mvpData.model = transform->getModelMatrix();

                    // Put MVP data in a descriptor set and bind it
                    gfx::DescriptorSet mvpSet(pass, subpassIdx, 0);
                    mvpSet.setUniformBuffer(0, mvpData);
                    cmd.setDescriptorSet(device_, pass, mvpSet);

                    // Draw our mesh
                    mesh_.draw(cmd);
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

}
