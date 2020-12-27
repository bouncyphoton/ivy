#include "render_device.h"
#include "vk_utils.h"
#include "ivy/consts.h"
#include "ivy/platform/platform.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <set>

namespace ivy::gfx {

constexpr u32 VULKAN_API_VERSION = VK_API_VERSION_1_1;

RenderDevice::RenderDevice(const Options &options, const Platform &platform)
    : options_(options) {
    LOG_CHECKPOINT();

    //----------------------------------
    // Vulkan version checking
    //----------------------------------

    u32 instanceApiVersion;
    vkEnumerateInstanceVersion(&instanceApiVersion);

    if (instanceApiVersion < VULKAN_API_VERSION) {
        Log::fatal("Instance level Vulkan API version is %d.%d.%d which is lower than %d.%d.%d",
                   VK_VERSION_TO_COMMA_SEPARATED_VALUES(instanceApiVersion),
                   VK_VERSION_TO_COMMA_SEPARATED_VALUES(VULKAN_API_VERSION));
    }

    Log::info("Vulkan %d.%d.%d is supported by instance. Using Vulkan %d.%d.%d",
              VK_VERSION_TO_COMMA_SEPARATED_VALUES(instanceApiVersion),
              VK_VERSION_TO_COMMA_SEPARATED_VALUES(VULKAN_API_VERSION));

    //----------------------------------
    // App info
    //----------------------------------

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pEngineName = consts::ENGINE_NAME;
    appInfo.apiVersion = VULKAN_API_VERSION;

    //----------------------------------
    // Instance creation
    //----------------------------------

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    if constexpr (consts::DEBUG) {
        Log::info("Using validation layers");
        const char *layers[] = {
            "VK_LAYER_KHRONOS_validation"
        };

        instanceCreateInfo.enabledLayerCount = COUNTOF(layers);
        instanceCreateInfo.ppEnabledLayerNames = layers;
    }

    std::vector<const char *> extensions = getInstanceExtensions();
    instanceCreateInfo.enabledExtensionCount = (u32)extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    VK_CHECKF(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));
    cleanupStack_.emplace([ = ]() {
        vkDestroyInstance(instance_, nullptr);
    });

    //----------------------------------
    // Surface creation
    //----------------------------------

    VK_CHECKF(glfwCreateWindowSurface(instance_, platform.getGlfwWindow(), nullptr, &surface_));
    cleanupStack_.emplace([ = ]() {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    });

    //----------------------------------
    // Physical device selection
    //----------------------------------

    choosePhysicalDevice();

    //----------------------------------
    // Logical device creation
    //----------------------------------

    std::set<u32> uniqueIndices = { graphicsFamilyIndex_, computeFamilyIndex_, presentFamilyIndex_ };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1;

    for (auto &index : uniqueIndices) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.emplace_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();

    std::vector<const char *> deviceExtensions = getDeviceExtensions();
    createInfo.enabledExtensionCount = (u32)deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VK_CHECKF(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_));
    cleanupStack_.emplace([ = ]() {
        vkDestroyDevice(device_, nullptr);
    });

    //----------------------------------
    // Get our queues
    //----------------------------------

    vkGetDeviceQueue(device_, graphicsFamilyIndex_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, computeFamilyIndex_, 0, &computeQueue_);
    vkGetDeviceQueue(device_, presentFamilyIndex_, 0, &presentQueue_);

    //----------------------------------
    // Vulkan Memory Allocator
    //----------------------------------

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VULKAN_API_VERSION;
    allocatorInfo.physicalDevice = physicalDevice_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;

    VK_CHECKF(vmaCreateAllocator(&allocatorInfo, &allocator_));
    cleanupStack_.emplace([ = ]() {
        vmaDestroyAllocator(allocator_);
    });

    //----------------------------------
    // Create our swapchain
    //----------------------------------

    createSwapchain();

    //----------------------------------
    // Create command pool and buffers
    //----------------------------------

    // Command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = graphicsFamilyIndex_;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECKF(vkCreateCommandPool(device_, &commandPoolCreateInfo, nullptr, &commandPool_));
    cleanupStack_.emplace([ = ]() {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
    });

    // Command buffers
    commandBuffers_.resize(swapchainImageViews_.size());
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool_;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = commandBuffers_.size();
    VK_CHECKF(vkAllocateCommandBuffers(device_, &commandBufferAllocateInfo, commandBuffers_.data()));
    cleanupStack_.emplace([ = ]() {
        vkFreeCommandBuffers(device_, commandPool_, commandBuffers_.size(), commandBuffers_.data());
    });

    //----------------------------------
    // Create sync objects
    //----------------------------------

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    imageAvailableSemaphores_.resize(options_.numFramesInFlight);
    renderFinishedSemaphores_.resize(options_.numFramesInFlight);
    inFlightFences_.resize(options_.numFramesInFlight);

    for (u32 i = 0; i < options_.numFramesInFlight; ++i) {
        VK_CHECKF(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores_[i]));
        VK_CHECKF(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores_[i]));
        VK_CHECKF(vkCreateFence(device_, &fenceCreateInfo, nullptr, &inFlightFences_[i]));

        cleanupStack_.emplace([ = ]() {
            vkDestroyFence(device_, inFlightFences_[i], nullptr);
            vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
            vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
        });
    }
}

RenderDevice::~RenderDevice() {
    LOG_CHECKPOINT();

    vkDeviceWaitIdle(device_);

    while (!cleanupStack_.empty()) {
        cleanupStack_.top()();
        cleanupStack_.pop();
    }
}

void RenderDevice::beginFrame() {
    //----------------------------------
    // Wait for frame to finish
    //----------------------------------

    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    //----------------------------------
    // Get image from swapchain
    //----------------------------------

    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE,
                          &swapImageIndex_);

    //----------------------------------
    // Begin recording command buffer
    //----------------------------------

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECKF(vkBeginCommandBuffer(commandBuffers_[swapImageIndex_], &commandBufferBeginInfo));
}

void RenderDevice::endFrame() {
    VK_CHECKF(vkEndCommandBuffer(commandBuffers_[swapImageIndex_]));

    //----------------------------------
    // Queue submission and sync
    //----------------------------------

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores_[currentFrame_];
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[swapImageIndex_];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_];

    VK_CHECKF(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]));

    //----------------------------------
    // Presentation
    //----------------------------------

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &swapImageIndex_;

    VK_CHECKW(vkQueuePresentKHR(presentQueue_, &presentInfo));

    currentFrame_ = (currentFrame_ + 1) % options_.numFramesInFlight;
}

CommandBuffer RenderDevice::getCommandBuffer() {
    return CommandBuffer(commandBuffers_[swapImageIndex_]);
}

void RenderDevice::submitOneTimeCommands(VkQueue queue, const std::function<void(CommandBuffer)> &record_func) {
    // TODO: create command pool for short-lived command buffers

    // Create command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = commandPool_;

    VkCommandBuffer commandBuffer;
    VK_CHECKF(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Record commands to command buffer
    record_func(CommandBuffer(commandBuffer));

    // Submit command buffer to queue
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

VkRenderPass RenderDevice::createRenderPass(const std::vector<VkAttachmentDescription> &attachments,
                                            const std::vector<VkSubpassDescription> &subpasses,
                                            const std::vector<VkSubpassDependency> &dependencies) {
    VkRenderPassCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0;
    ci.attachmentCount = (u32) attachments.size();
    ci.pAttachments = attachments.data();
    ci.subpassCount = (u32) subpasses.size();
    ci.pSubpasses = subpasses.data();
    ci.dependencyCount = (u32) dependencies.size();
    ci.pDependencies = dependencies.data();

    VkRenderPass renderPass;
    VK_CHECKF(vkCreateRenderPass(device_, &ci, nullptr, &renderPass));
    cleanupStack_.emplace([ = ]() {
        vkDestroyRenderPass(device_, renderPass, nullptr);
    });

    return renderPass;
}

VkPipelineLayout RenderDevice::createLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings = {}) {
    // Create descriptor set layouts
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCI.bindingCount = (u32) bindings.size();
    descriptorSetLayoutCI.pBindings = bindings.data();

    VkDescriptorSetLayout setLayout;
    VK_CHECKF(vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCI, nullptr, &setLayout));
    cleanupStack_.emplace([ = ]() {
        vkDestroyDescriptorSetLayout(device_, setLayout, nullptr);
    });

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutCI = {};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.pNext = nullptr;
    layoutCI.flags = 0;
    layoutCI.setLayoutCount = 1;
    layoutCI.pSetLayouts = &setLayout;
    layoutCI.pushConstantRangeCount = 0;
    layoutCI.pPushConstantRanges = nullptr;

    VkPipelineLayout layout;
    VK_CHECKF(vkCreatePipelineLayout(device_, &layoutCI, nullptr, &layout));
    cleanupStack_.emplace([ = ]() {
        vkDestroyPipelineLayout(device_, layout, nullptr);
    });

    return layout;
}

VkPipeline RenderDevice::createGraphicsPipeline(const std::vector<Shader> &shaders,
                                                const VertexDescription &vertex_description, VkPipelineLayout layout,
                                                VkRenderPass render_pass, u32 subpass, u32 num_color_attachments) {
    // Generate shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaders.size());
    for (u32 i = 0; i < shaders.size(); ++i) {
        shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i].stage = shaders[i].getStage();
        shaderStages[i].module = loadShader(device_, shaders[i].getShaderPath());
        shaderStages[i].pName = "main";
    }

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = vertex_description.getBindings().size();
    vertexInputCreateInfo.pVertexBindingDescriptions = vertex_description.getBindings().data();
    vertexInputCreateInfo.vertexAttributeDescriptionCount = vertex_description.getAttributes().size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertex_description.getAttributes().data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Viewport
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32) swapchainExtent_.width;
    viewport.height = (f32) swapchainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.lineWidth = 1.0f;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    // TODO: Depth/stencil, check renderpass
    // VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    // depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    // depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    // depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

    // Color blending
    VkPipelineColorBlendAttachmentState blendState = {};
    blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendState.blendEnable = VK_FALSE;

    // We don't have independent blending enabled, so these must be the same for all attachments
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(num_color_attachments, blendState);

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.attachmentCount = (u32) colorBlendAttachmentStates.size();
    colorBlendCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    VkGraphicsPipelineCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0;
    ci.stageCount = (u32) shaderStages.size();
    ci.pStages = shaderStages.data();
    ci.pVertexInputState = &vertexInputCreateInfo;
    ci.pInputAssemblyState = &inputAssemblyCreateInfo;
    ci.pTessellationState = nullptr;
    ci.pViewportState = &viewportCreateInfo;
    ci.pRasterizationState = &rasterizationCreateInfo;
    ci.pMultisampleState = &multisampleCreateInfo;
    // ci.pDepthStencilState = &depthStencilCreateInfo;
    ci.pColorBlendState = &colorBlendCreateInfo;
    ci.pDynamicState = nullptr; // TODO: dynamic state
    ci.layout = layout;
    ci.renderPass = render_pass;
    ci.subpass = subpass;
    ci.basePipelineHandle = VK_NULL_HANDLE;
    ci.basePipelineIndex = -1;

    VkPipeline graphicsPipeline;
    VK_CHECKF(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &ci, nullptr, &graphicsPipeline));
    cleanupStack_.emplace([ = ]() {
        vkDestroyPipeline(device_, graphicsPipeline, nullptr);
    });

    for (VkPipelineShaderStageCreateInfo stage : shaderStages) {
        vkDestroyShaderModule(device_, stage.module, nullptr);
    }

    return graphicsPipeline;
}

Framebuffer RenderDevice::getFramebuffer(const GraphicsPass &pass) {
    VkRenderPass renderPass = pass.getVkRenderPass();

    // Create framebuffers if they don't exist
    if (swapchainFramebuffers_[renderPass].size() != swapchainImageViews_.size()) {
        for (VkImageView &swapchainImageView : swapchainImageViews_) {
            const std::unordered_map<std::string, AttachmentDescription> &descriptions = pass.getAttachmentDescriptions();
            std::vector<VkImageView> attachmentViews;

            for (const auto &attachment : descriptions) {
                if (attachment.first == GraphicsPass::SwapchainName) {
                    attachmentViews.emplace_back(swapchainImageView);
                } else {
                    const AttachmentDescription &desc = attachment.second;

                    // TODO: support non-2D attachments (imageCI.imageType, imageCI.extent, imageViewCI.viewType) ?
                    // TODO: support non-swapchain-sized attachments ?

                    VkImageCreateInfo imageCI = {};
                    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageCI.imageType = VK_IMAGE_TYPE_2D;
                    imageCI.format = desc.vkAttachmentDescription.format;
                    imageCI.extent.width = swapchainExtent_.width;
                    imageCI.extent.height = swapchainExtent_.height;
                    imageCI.extent.depth = 1;
                    imageCI.mipLevels = 1;
                    imageCI.arrayLayers = 1;
                    imageCI.samples = desc.vkAttachmentDescription.samples;
                    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageCI.usage = desc.usage;
                    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageCI.initialLayout = desc.vkAttachmentDescription.initialLayout;

                    VmaAllocationCreateInfo allocCI = {};
                    allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

                    VkImage image;
                    VmaAllocation allocation;
                    VK_CHECKF(vmaCreateImage(allocator_, &imageCI, &allocCI, &image, &allocation, nullptr));
                    cleanupStack_.emplace([ = ]() {
                        vmaDestroyImage(allocator_, image, allocation);
                    });

                    VkImageAspectFlags aspectMask = 0;
                    if (desc.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    }
                    if (desc.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    }

                    VkImageViewCreateInfo imageViewCI = {};
                    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    imageViewCI.image = image;
                    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    imageViewCI.format = desc.vkAttachmentDescription.format;
                    imageViewCI.subresourceRange.aspectMask = aspectMask;
                    imageViewCI.subresourceRange.baseMipLevel = 0;
                    imageViewCI.subresourceRange.levelCount = 1;
                    imageViewCI.subresourceRange.baseArrayLayer = 0;
                    imageViewCI.subresourceRange.layerCount = 1;

                    attachmentViews.emplace_back();
                    VK_CHECKF(vkCreateImageView(device_, &imageViewCI, nullptr, &attachmentViews.back()));
                    cleanupStack_.emplace([ = ]() {
                        vkDestroyImageView(device_, attachmentViews.back(), nullptr);
                    });
                }
            }

            // loop over graphics pass attachmentViews

            // if name == GraphicsPass::Swapchain
            // attachmentViews.emplace_back(swapchainImageView);
            // else
            // vkCreateImageView... cleanupstack

            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = (u32) attachmentViews.size();
            framebufferCreateInfo.pAttachments = attachmentViews.data();
            framebufferCreateInfo.width = swapchainExtent_.width;
            framebufferCreateInfo.height = swapchainExtent_.height;
            framebufferCreateInfo.layers = 1;

            VkFramebuffer framebuffer;
            VK_CHECKF(vkCreateFramebuffer(device_, &framebufferCreateInfo, nullptr, &framebuffer));
            cleanupStack_.emplace([ = ]() {
                vkDestroyFramebuffer(device_, framebuffer, nullptr);
            });

            swapchainFramebuffers_[renderPass].emplace_back(Framebuffer(framebuffer, swapchainExtent_));
        }
    }

    return swapchainFramebuffers_[renderPass][swapImageIndex_];
}

VkBuffer RenderDevice::createVertexBuffer(void *data, VkDeviceSize size) {
    if (size <= 0) {
        Log::fatal("Invalid vertex buffer size: %d", size);
    }

    // Create our vertex buffer and memory
    VkBufferCreateInfo vertexBufferCI = {};
    vertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferCI.size = size;
    vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertexBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vertexAllocCI = {};
    vertexAllocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer vertexBuffer;
    VmaAllocation vertexAllocation;
    VmaAllocationInfo vertexAllocInfo;
    VK_CHECKF(vmaCreateBuffer(allocator_, &vertexBufferCI, &vertexAllocCI, &vertexBuffer, &vertexAllocation,
                              &vertexAllocInfo));
    cleanupStack_.emplace([ = ]() {
        vmaDestroyBuffer(allocator_, vertexBuffer, vertexAllocation);
    });

    VkMemoryPropertyFlags memFlags;
    vmaGetMemoryTypeProperties(allocator_, vertexAllocInfo.memoryType, &memFlags);
    if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
        // If the memory happens to be mappable, no need for staging buffer
        void *mappedData;
        vmaMapMemory(allocator_, vertexAllocation, &mappedData);
        memcpy(mappedData, data, (size_t) size);
        vmaUnmapMemory(allocator_, vertexAllocation);
    } else {
        // Otherwise, we need a staging buffer
        VkBufferCreateInfo stagingBufferCI = {};
        stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferCI.size = size;
        stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocCI = {};
        stagingAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stagingAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        VmaAllocationInfo stagingAllocInfo;
        VK_CHECKF(vmaCreateBuffer(allocator_, &stagingBufferCI, &stagingAllocCI, &stagingBuffer, &stagingAllocation,
                                  &stagingAllocInfo));

        // Copy data into staging buffer
        memcpy(stagingAllocInfo.pMappedData, data, (size_t) size);

        // Copy staging buffer into vertex buffer
        submitOneTimeCommands(graphicsQueue_, [ = ](CommandBuffer cmd) {
            cmd.copyBuffer(vertexBuffer, stagingBuffer, size);
        });

        // Destroy staging buffer
        vmaDestroyBuffer(allocator_, stagingBuffer, stagingAllocation);
    }

    return vertexBuffer;
}

void RenderDevice::choosePhysicalDevice() {
    u32 numPhysicalDevices;
    VK_CHECKF(vkEnumeratePhysicalDevices(instance_, &numPhysicalDevices, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    VK_CHECKW(vkEnumeratePhysicalDevices(instance_, &numPhysicalDevices, physicalDevices.data()));

    if (numPhysicalDevices == 0) {
        Log::fatal("No physical devices were found");
    }

    std::vector<const char *> requiredExtensions = getDeviceExtensions();

    Log::info("+-- Physical Devices -----------");
    for (VkPhysicalDevice &physicalDevice : physicalDevices) {
        // Get properties
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        // Get device extensions
        u32 numDeviceExtensions;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numDeviceExtensions, nullptr);
        std::vector<VkExtensionProperties> deviceExtensions(numDeviceExtensions);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numDeviceExtensions, deviceExtensions.data());

        // We want a device that supports our api version
        bool suitable = true;
        suitable = suitable && properties.apiVersion >= VULKAN_API_VERSION;

        // Check if the device has the extensions we require
        for (const char *requiredExt : requiredExtensions) {
            bool extensionFound = false;

            // See if we can find the current required extension in the device extensions list
            for (VkExtensionProperties &ext : deviceExtensions) {
                if (std::string(ext.extensionName) == requiredExt) {
                    extensionFound = true;
                    break;
                }
            }

            suitable = suitable && extensionFound;
        }

        // If extensions found, check if swapchain is ok for our uses
        if (suitable) {
            suitable = suitable && !getPresentModes(physicalDevice, surface_).empty();

            // Color attachment flag always supported, we need to check for transfer
            u32 usageFlags = getSurfaceCapabilities(physicalDevice, surface_).supportedUsageFlags;
            suitable = suitable && (usageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        }

        // Get device queue families
        u32 numQueueFamilies;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueFamilies.data());

        // Check if the device has queue families that support operations we need
        std::pair<bool, u32> graphicsFamilyIndex;
        std::pair<bool, u32> computeFamilyIndex;
        std::pair<bool, u32> presentFamilyIndex;
        for (u32 i = 0; i < numQueueFamilies; ++i) {
            // Check graphics support
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamilyIndex = { true, i };
            }

            // Check compute support
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeFamilyIndex = { true, i };
            }

            // Check present support
            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface_, &presentSupport);
            if (presentSupport) {
                presentFamilyIndex = { true, i };
            }
        }

        suitable = suitable && graphicsFamilyIndex.first && computeFamilyIndex.first && presentFamilyIndex.first;

        // If it's suitable
        if (suitable) {
            // If physical device is not set yet OR current device is a discrete GPU, set it!
            if (physicalDevice_ == VK_NULL_HANDLE || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                physicalDevice_ = physicalDevice;
                graphicsFamilyIndex_ = graphicsFamilyIndex.second;
                computeFamilyIndex_ = computeFamilyIndex.second;
                presentFamilyIndex_ = presentFamilyIndex.second;
            }
        }

        // Log information about the device
        Log::info("| %s", properties.deviceName);
        Log::info("|   Version:  %d.%d.%d", VK_VERSION_TO_COMMA_SEPARATED_VALUES(properties.apiVersion));
        Log::info("|   Num Exts: %d", numDeviceExtensions);
        Log::info("|   Discrete: %s", properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "yes" : "no");
        Log::info("|   Graphics: %s", graphicsFamilyIndex.first ? "yes" : "no");
        Log::info("|   Compute:  %s", computeFamilyIndex.first ? "yes" : "no");
        Log::info("|   Present:  %s", presentFamilyIndex.first ? "yes" : "no");
        Log::info("|   Suitable: %s", suitable ? "yes" : "no");
        Log::info("+-------------------------------");
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        Log::fatal("Failed to find a suitable physical device");
    }

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    Log::info("Using physical device: %s", properties.deviceName);
}

void RenderDevice::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities = getSurfaceCapabilities(physicalDevice_, surface_);

    // Clamp our desired image count to minimum
    u32 imageCount = std::max(options_.numFramesInFlight, capabilities.minImageCount);
    if (capabilities.maxImageCount != 0) {
        // If there's an upper maximum, clamp it
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    // Get the extent for our swapchain
    swapchainExtent_ = capabilities.currentExtent;
    if (swapchainExtent_.width == UINT32_MAX && swapchainExtent_.height == UINT32_MAX) {
        VkExtent2D renderExtent = { options_.renderWidth, options_.renderHeight };

        swapchainExtent_ = clamp(renderExtent, capabilities.minImageExtent, capabilities.maxImageExtent);
    }

    // Get how alpha channel should be composited for swapchain
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    // Prefer VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
        compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
        compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    //----------------------------------
    // Image formats
    //----------------------------------

    std::vector<VkSurfaceFormatKHR> formats = getSurfaceFormats(physicalDevice_, surface_);

    // Default to first surface format
    VkSurfaceFormatKHR surfaceFormat = formats[0];

    // Try to find a more preferable surface format
    for (const VkSurfaceFormatKHR &f : formats) {
        // We would prefer RGBA8_UNORM and SRGB color space
        if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }

    swapchainFormat_ = surfaceFormat.format;

    //----------------------------------
    // Present modes
    //----------------------------------

    std::vector<VkPresentModeKHR> presentModes = getPresentModes(physicalDevice_, surface_);

    // Choose present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Always supported
    VkPresentModeKHR desiredPresentMode = [&]() {
        // Turn present mode enum into VkPresentMode
        switch (options_.desiredPresentMode) {
            case Options::PresentModeEnum::IMMEDIATE:
                return VK_PRESENT_MODE_IMMEDIATE_KHR;
            case Options::PresentModeEnum::MAILBOX:
                return VK_PRESENT_MODE_MAILBOX_KHR;
            case Options::PresentModeEnum::FIFO:
                return VK_PRESENT_MODE_FIFO_KHR;
        }

        Log::fatal("Unknown desired present mode: %d", static_cast<u32>(options_.desiredPresentMode));
    }
    ();

    for (VkPresentModeKHR m : presentModes) {
        // Prefer mailbox if it's available
        if (m == desiredPresentMode) {
            presentMode = m;
            break;
        }
    }

    //----------------------------------
    // Image sharing
    //----------------------------------

    // Default to assuming graphics and present are part of the same queue family
    VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    u32 numQueueFamilies = 0;
    u32 *pQueueFamilies = nullptr;

    // Graphics and present are part of separate queue families!
    u32 queueFamilies[] = { graphicsFamilyIndex_, presentFamilyIndex_ };
    if (presentFamilyIndex_ != graphicsFamilyIndex_) {
        imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        numQueueFamilies = COUNTOF(queueFamilies);
        pQueueFamilies = queueFamilies;
    }

    //----------------------------------
    // Create swapchain
    //----------------------------------

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface_;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent_;
    swapchainCreateInfo.imageArrayLayers = 1; // non-stereoscopic for now...
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainCreateInfo.imageSharingMode = imageSharingMode;
    swapchainCreateInfo.queueFamilyIndexCount = numQueueFamilies;
    swapchainCreateInfo.pQueueFamilyIndices = pQueueFamilies;
    swapchainCreateInfo.preTransform = capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = compositeAlpha;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE; // Allow for swapchain to not own all of its pixels

    VK_CHECKF(vkCreateSwapchainKHR(device_, &swapchainCreateInfo, nullptr, &swapchain_));
    cleanupStack_.emplace([ = ]() {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    });

    //----------------------------------
    // Create image views
    //----------------------------------

    u32 numSwapchainImages;
    VK_CHECKF(vkGetSwapchainImagesKHR(device_, swapchain_, &numSwapchainImages, nullptr));
    swapchainImages_.resize(numSwapchainImages);
    VK_CHECKW(vkGetSwapchainImagesKHR(device_, swapchain_, &numSwapchainImages, swapchainImages_.data()));

    swapchainImageViews_.resize(numSwapchainImages);
    for (u32 i = 0; i < numSwapchainImages; ++i) {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages_[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapchainFormat_;

        // Our swapchain images are color targets without mipmapping or multiple layers
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VK_CHECKF(vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &swapchainImageViews_[i]));
        cleanupStack_.emplace([ = ]() {
            vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
        });
    }
}

}
