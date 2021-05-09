#include "renderer_rt.h"
#include "ivy/log.h"
#include "ivy/scene/components/camera.h"
#include "ivy/scene/components/transform.h"
#include "ivy/scene/components/model.h"
#include <glm/glm.hpp>

using namespace ivy;

struct PerFrameRT {
    alignas(16) glm::mat4 invProjection;
    alignas(16) glm::mat4 invView;
    alignas(4) u32 numIndices;
};

RendererRT::RendererRT(gfx::RenderDevice &render_device)
    : device_(render_device) {

    sampler_ = device_.createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

    texture_ = gfx::TextureBuilder(device_)
               .setExtent2D(device_.getSwapchainExtent().width, device_.getSwapchainExtent().height)
               .setFormat(VK_FORMAT_R16G16B16A16_UNORM)
               .setImageAspect(VK_IMAGE_ASPECT_COLOR_BIT)
               // Want to be able to use as storage target in compute
               .setAdditionalUsage(VK_IMAGE_USAGE_STORAGE_BIT)
               .build();

    rtPass_ = gfx::ComputePassBuilder(device_)
              .setShader("../assets/shaders/compute.comp.spv")
              .addStorageImageDescriptor(0, 0)
              .addUniformBufferDescriptor(0, 1)
              .addStorageBufferDescriptor(0, 2) // Vertices
              .addStorageBufferDescriptor(0, 3) // Indices
              .build();

    presentPass_ = gfx::GraphicsPassBuilder(device_)
                   .addAttachmentSwapchain()
                   .addSubpass("processRT", gfx::SubpassBuilder()
                               .addColorAttachment(gfx::GraphicsPass::SwapchainName, 0)
                               .addShader(gfx::Shader::StageEnum::VERTEX, "../assets/shaders/fullscreen.vert.spv")
                               .addShader(gfx::Shader::StageEnum::FRAGMENT, "../assets/shaders/postprocess.frag.spv")
                               .addTextureDescriptor(0, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
                               .build())
                   .build();
}

void RendererRT::render(Scene &scene) {
    device_.beginFrame();
    gfx::CommandBuffer cmd = device_.getCommandBuffer();

    // Transition for writing
    cmd.transitionImage(*texture_,
                        // layout
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_GENERAL,
                        // stage
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        // access
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT
                       );

    // Ray trace compute shader
    cmd.executeComputePass(*rtPass_, [&]() {
        EntityHandle cameraEntity = scene.findEntityWithAllComponents<Camera, Transform>();
        if (!cameraEntity) {
            Log::warn("No camera found in scene");
            return;
        }

        Camera    camera          = *cameraEntity->getComponent<Camera>();
        Transform cameraTransform = *cameraEntity->getComponent<Transform>();
        f32 aspectRatio = texture_->getExtent().width / (f32) texture_->getExtent().height;

        glm::mat4 proj = glm::perspective(camera.getFovY(), aspectRatio, camera.getNearPlane(), camera.getFarPlane());
        glm::mat4 view = glm::lookAt(cameraTransform.getPosition(),
                                     cameraTransform.getPosition() + cameraTransform.getForward(), Transform::UP);

        const auto &meshes = scene.findEntitiesWithTag("cornell_box").front()->getComponent<Model>()->getMeshes();

        // TODO: have all the vertices in one buffer, all the indices in one buffer, etc.
        // currently, we just cycle through the meshes and render one per frame (epilepsy warning)
        static u32 c = 0;
        const gfx::Geometry &mesh = meshes.at(c).getGeometry();
        c = (c + 1) % meshes.size();

        PerFrameRT perFrame = {};
        perFrame.invProjection = glm::inverse(proj);
        perFrame.invView = glm::inverse(view);
        perFrame.numIndices = mesh.getNumIndices();

        gfx::DescriptorSet set(*rtPass_, 0);
        set.setStorageImage(0, *texture_);
        set.setUniformBuffer(1, perFrame);
        set.setStorageBuffer(2, mesh.getVertexBuffer(), mesh.getVertexBufferSize());
        set.setStorageBuffer(3, mesh.getIndexBuffer(), mesh.getIndexBufferSize());
        cmd.setDescriptorSet(device_, *rtPass_, set);

        cmd.dispatch(texture_->getExtent().width, texture_->getExtent().height);
    });

    // Transition for reading from shader
    cmd.transitionImage(*texture_,
                        // layout
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        // stage
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        // access
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_SHADER_READ_BIT
                       );

    // Process image for present
    cmd.executeGraphicsPass(device_, *presentPass_, [&]() {
        cmd.bindGraphicsPipeline(*presentPass_, 0);

        gfx::DescriptorSet set(*presentPass_, 0, 0);
        set.setTexture(0, *texture_, sampler_);
        cmd.setDescriptorSet(device_, *presentPass_, set);

        cmd.draw(3, 1, 0, 0);
    });

    device_.endFrame();
}
