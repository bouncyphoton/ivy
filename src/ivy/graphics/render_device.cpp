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
        Log::fatal("Instance level Vulkan API version is %.%.% which is lower than %.%.%",
                   VK_VERSION_TO_COMMA_SEPARATED_VALUES(instanceApiVersion),
                   VK_VERSION_TO_COMMA_SEPARATED_VALUES(VULKAN_API_VERSION));
    }

    Log::info("Vulkan %.%.% is supported by instance. Using Vulkan %.%.%",
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
    f32 queuePriority = 1;

    for (auto &index : uniqueIndices) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.emplace_back(queueCreateInfo);
    }

    // Enable features
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice_, &features);
    features.imageCubeArray = VK_TRUE;
    features.geometryShader = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
    createInfo.pEnabledFeatures = &features;

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

    //----------------------------------
    // Create descriptor pool
    //----------------------------------

    // TODO: figure out good pool sizes and maxSets

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.emplace_back(VkDescriptorPoolSize{
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, limits_.maxDescriptorSetInputAttachments * 2
    });
    poolSizes.emplace_back(VkDescriptorPoolSize{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, limits_.maxDescriptorSetUniformBuffers * 2
    });
    poolSizes.emplace_back(VkDescriptorPoolSize{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, limits_.maxDescriptorSetSampledImages * 2
    });

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = maxSets_;
    descriptorPoolCreateInfo.poolSizeCount = (u32) poolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

    // Make pools
    for (u32 i = 0; i < options_.numFramesInFlight; ++i) {
        VkDescriptorPool pool;
        VK_CHECKF(vkCreateDescriptorPool(device_, &descriptorPoolCreateInfo, nullptr, &pool));
        cleanupStack_.emplace([ = ]() {
            vkDestroyDescriptorPool(device_, pool, nullptr);
        });

        pools_.emplace_back(pool);
    }

    // Make descriptor set caches
    descriptorSetCaches_.resize(options_.numFramesInFlight);

    //----------------------------------
    // Create uniform buffer
    //----------------------------------

    uniformBuffers_.resize(options_.numFramesInFlight);
    uniformBufferMappedPointers_.resize(options_.numFramesInFlight);
    uniformBufferOffsets_.resize(options_.numFramesInFlight);
    uniformBufferSize_ = 1024 * 1024;

    for (u32 i = 0; i < options_.numFramesInFlight; ++i) {
        VmaAllocationCreateInfo allocCI = {};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBufferCreateInfo bufferCI = {};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = uniformBufferSize_;
        bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocation allocation;
        VmaAllocationInfo allocInfo;
        VK_CHECKF(vmaCreateBuffer(allocator_, &bufferCI, &allocCI, &uniformBuffers_[i], &allocation, &allocInfo));
        cleanupStack_.emplace([ = ]() {
            vmaDestroyBuffer(allocator_, uniformBuffers_[i], allocation);
        });

        uniformBufferMappedPointers_.at(i) = allocInfo.pMappedData;
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
    // Reset per-frame descriptor data
    //----------------------------------

    descriptorSetCaches_[swapImageIndex_].markAllAsAvailable();
    uniformBufferOffsets_[swapImageIndex_] = 0;

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
    // Debug stats for this frame
    //----------------------------------

    Log::verbose("+-- Frame stats for % -----------", swapImageIndex_);
    Log::verbose("| %/% bytes (%\\%) of the buffer were used for uniform buffers",
                 uniformBufferOffsets_[swapImageIndex_], uniformBufferSize_,
                 100.0f * (uniformBufferOffsets_[swapImageIndex_] / (f32)uniformBufferSize_));
    Log::verbose("| %/% descriptor sets were used this frame",
                 descriptorSetCaches_[swapImageIndex_].countNumUsed(), maxSets_);
    Log::verbose("| % descriptor sets are cached for this frame",
                 descriptorSetCaches_[swapImageIndex_].countTotalCached());
    Log::verbose("+-------------------------------");

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

VkQueue RenderDevice::getGraphicsQueue() {
    return graphicsQueue_;
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

SubpassLayout RenderDevice::createLayout(const LayoutBindingsMap_t &layout_bindings) {
    // Create descriptor sets layouts
    std::vector<VkDescriptorSetLayout> setLayouts;
    for (const auto &setsPair : layout_bindings) {
        // Convert unordered map of bindings to vector
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(setsPair.second.size());
        for (const auto &binding : setsPair.second) {
            bindings.emplace_back(binding.second);
        }

        // Create descriptor set layout
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
        descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.bindingCount = (u32) bindings.size();
        descriptorSetLayoutCI.pBindings = bindings.data();

        setLayouts.emplace_back();
        VK_CHECKF(vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCI, nullptr, &setLayouts.back()));
        cleanupStack_.emplace([ = ]() {
            vkDestroyDescriptorSetLayout(device_, setLayouts.back(), nullptr);
        });
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutCI = {};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.pNext = nullptr;
    layoutCI.flags = 0;
    layoutCI.setLayoutCount = (u32) setLayouts.size();
    layoutCI.pSetLayouts = setLayouts.data();
    layoutCI.pushConstantRangeCount = 0;
    layoutCI.pPushConstantRanges = nullptr;

    VkPipelineLayout layout;
    VK_CHECKF(vkCreatePipelineLayout(device_, &layoutCI, nullptr, &layout));
    cleanupStack_.emplace([ = ]() {
        vkDestroyPipelineLayout(device_, layout, nullptr);
    });

    return SubpassLayout{layout, setLayouts};
}

VkPipeline RenderDevice::createGraphicsPipeline(const std::vector<Shader> &shaders,
                                                const VertexDescription &vertex_description, VkPipelineLayout layout,
                                                VkRenderPass render_pass, u32 subpass, u32 num_color_attachments,
                                                bool has_depth_attachment, const GraphicsPipelineState &state) {
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
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    // Depth create info
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = state.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencilCreateInfo.depthWriteEnable = state.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencilCreateInfo.depthCompareOp = state.depthCompareOp;

    // Color blending
    VkPipelineColorBlendAttachmentState blendState = {};
    blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendState.blendEnable = VK_TRUE;
    blendState.srcColorBlendFactor = state.srcColorBlendFactor;
    blendState.dstColorBlendFactor = state.dstColorBlendFactor;
    blendState.colorBlendOp = state.colorBlendOp;
    blendState.srcAlphaBlendFactor = state.srcAlphaBlendFactor;
    blendState.dstAlphaBlendFactor = state.dstAlphaBlendFactor;
    blendState.alphaBlendOp = state.alphaBlendOp;

    // We don't have independent blending enabled, so these must be the same for all attachments
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(num_color_attachments, blendState);

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.attachmentCount = (u32) colorBlendAttachmentStates.size();
    colorBlendCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    // Dynamic state
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;
    dynamicStateCreateInfo.dynamicStateCount = COUNTOF(dynamicStates);

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
    ci.pDepthStencilState = has_depth_attachment ? &depthStencilCreateInfo : nullptr;
    ci.pColorBlendState = &colorBlendCreateInfo;
    ci.pDynamicState = &dynamicStateCreateInfo;
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

Framebuffer &RenderDevice::getFramebuffer(const GraphicsPass &pass) {
    VkRenderPass renderPass = pass.getVkRenderPass();
    const std::map<std::string, AttachmentInfo> &attachmentInfos = pass.getAttachmentInfos();

    // How many framebuffers this render pass needs depends on whether or not it outputs to swapchain
    u32 numFramebuffers = attachmentInfos.find(GraphicsPass::SwapchainName) == attachmentInfos.end()
                          ? 1
                          : swapchainImages_.size();

    // Create framebuffers if they don't exist
    if (framebuffers_[renderPass].empty()) {
        for (u32 frame = 0; frame < numFramebuffers; ++frame) {
            // Unordered map and vector hold almost the same data.
            // unordered_map is for keeping track of views by attachment name
            // vector is for framebuffer create info
            std::unordered_map<std::string, VkImageView> attachmentViews;
            std::unordered_map<std::string, VkImage> attachmentImages;
            std::vector<VkImageView> viewsVector;

            // Need to create resources if it's the first framebuffer
            bool firstFramebuffer = (frame == 0);

            // Get/create image view for each attachment in graphics pass
            for (const auto &infoPair : attachmentInfos) {
                const AttachmentInfo &desc = infoPair.second;
                VkImageView view = VK_NULL_HANDLE;
                VkImage image = VK_NULL_HANDLE;

                if (infoPair.first == GraphicsPass::SwapchainName) {
                    // Using swapchain image & imageview
                    view = swapchainImageViews_[frame];
                    image = swapchainImages_[frame];
                } else if (desc.texture) {
                    // The image & image for this attachment was passed to the graphics pass
                    view = desc.texture->getImageView();
                    image = desc.texture->getImage();
                } else if (!firstFramebuffer) {
                    // The image & imageview was already created
                    view = framebuffers_[renderPass].front().getView(infoPair.first);
                    image = framebuffers_[renderPass].front().getImage(infoPair.first);
                } else {
                    // Need to create image & imageview for this attachment

                    VkImageCreateInfo imageCI = {};
                    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageCI.imageType = VK_IMAGE_TYPE_2D;
                    imageCI.format = desc.description.format;
                    imageCI.extent.width = pass.getExtent().width;
                    imageCI.extent.height = pass.getExtent().height;
                    imageCI.extent.depth = 1;
                    imageCI.mipLevels = 1;
                    imageCI.arrayLayers = 1;
                    imageCI.samples = desc.description.samples;
                    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageCI.usage = desc.usage;
                    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageCI.initialLayout = desc.description.initialLayout;

                    VmaAllocationCreateInfo allocCI = {};
                    allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

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
                    imageViewCI.format = desc.description.format;
                    imageViewCI.subresourceRange.aspectMask = aspectMask;
                    imageViewCI.subresourceRange.baseMipLevel = 0;
                    imageViewCI.subresourceRange.levelCount = 1;
                    imageViewCI.subresourceRange.baseArrayLayer = 0;
                    imageViewCI.subresourceRange.layerCount = 1;

                    VK_CHECKF(vkCreateImageView(device_, &imageViewCI, nullptr, &view));
                    cleanupStack_.emplace([ = ]() {
                        vkDestroyImageView(device_, view, nullptr);
                    });
                }

                attachmentViews[infoPair.first] = view;
                attachmentImages[infoPair.first] = image;
                viewsVector.emplace_back(view);
            }

            // Create the framebuffer for this frame and render pass
            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = (u32) viewsVector.size();
            framebufferCreateInfo.pAttachments = viewsVector.data();
            framebufferCreateInfo.width = pass.getExtent().width;
            framebufferCreateInfo.height = pass.getExtent().height;
            framebufferCreateInfo.layers = pass.getNumLayers();

            VkFramebuffer framebuffer;
            VK_CHECKF(vkCreateFramebuffer(device_, &framebufferCreateInfo, nullptr, &framebuffer));
            cleanupStack_.emplace([ = ]() {
                vkDestroyFramebuffer(device_, framebuffer, nullptr);
            });

            framebuffers_[renderPass].emplace_back(Framebuffer(framebuffer, swapchainExtent_,
                                                               attachmentViews, attachmentImages));
        }
    }

    return framebuffers_[renderPass][swapImageIndex_ % numFramebuffers];
}

VkBuffer RenderDevice::createVertexBuffer(const void *data, VkDeviceSize size) {
    if (size <= 0) {
        Log::fatal("Invalid vertex buffer size: %", size);
    }

    return createBufferGPU(data, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

VkBuffer RenderDevice::createIndexBuffer(const void *data, VkDeviceSize size) {
    if (size <= 0) {
        Log::fatal("Invalid index buffer size: %", size);
    }

    return createBufferGPU(data, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

std::pair<VkImage, VkImageView> RenderDevice::createTextureGPUFromData(VkImageCreateInfo image_ci,
                                                                       VkImageViewCreateInfo image_view_ci,
                                                                       const void *data, VkDeviceSize size) {
    // Create staging buffer
    std::pair<VkBuffer, VmaAllocation> stagingBuffer;
    if (size > 0) {
        stagingBuffer = createStagingBufferCPU(data, size);
    }

    // Create image
    image_ci.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocCI = {};
    allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image;
    VmaAllocation allocation;
    VK_CHECKF(vmaCreateImage(allocator_, &image_ci, &allocCI, &image, &allocation, nullptr));
    cleanupStack_.emplace([ = ]() {
        vmaDestroyImage(allocator_, image, allocation);
    });

    if (size > 0) {
        // Copy data into image
        submitOneTimeCommands(graphicsQueue_, [ = ](CommandBuffer cmd) {
            // Transition image for copying buffer into it
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.image = image;
                barrier.subresourceRange = image_view_ci.subresourceRange;

                cmd.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    0, 0, nullptr, 0, nullptr, 1, &barrier);
            }

            // Copy buffer into it
            {
                cmd.copyBufferToImage(stagingBuffer.first, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      image_view_ci.subresourceRange.aspectMask, image_ci.extent.width,
                                      image_ci.extent.height, image_ci.extent.depth, image_ci.arrayLayers);
            }

            // Transition image for reading in shaders
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.image = image;
                barrier.subresourceRange = image_view_ci.subresourceRange;

                cmd.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                    0, 0, nullptr, 0, nullptr, 1, &barrier);
            }
        });

        // Destroy staging buffer, we're done using it
        vmaDestroyBuffer(allocator_, stagingBuffer.first, stagingBuffer.second);
    }

    // Create image view
    image_view_ci.image = image;

    VkImageView imageView;
    VK_CHECKF(vkCreateImageView(device_, &image_view_ci, nullptr, &imageView));
    cleanupStack_.emplace([ = ]() {
        vkDestroyImageView(device_, imageView, nullptr);
    });

    return { image, imageView };
}

// TODO: mipmapped textures

VkSampler RenderDevice::createSampler(VkFilter mag_filter, VkFilter min_filter, VkSamplerAddressMode u_wrap,
                                      VkSamplerAddressMode v_wrap, VkSamplerAddressMode w_wrap) {
    // TODO: anisotropic filtering

    VkSamplerCreateInfo samplerCI = {};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter = mag_filter;
    samplerCI.minFilter = min_filter;
    samplerCI.addressModeU = u_wrap;
    samplerCI.addressModeV = v_wrap;
    samplerCI.addressModeW = w_wrap;
    samplerCI.anisotropyEnable = VK_FALSE;
    samplerCI.maxAnisotropy = 1.0f;
    samplerCI.compareEnable = VK_FALSE;
    samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.mipLodBias = 0.0f;
    samplerCI.minLod = 0.0f;
    samplerCI.maxLod = 0.0f;
    samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCI.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    VK_CHECKF(vkCreateSampler(device_, &samplerCI, nullptr, &sampler));
    cleanupStack_.emplace([ = ]() {
        vkDestroySampler(device_, sampler, nullptr);
    });

    return sampler;
}

VkDescriptorSet RenderDevice::getVkDescriptorSet(const GraphicsPass &pass, const DescriptorSet &set) {
    DescriptorSetCache &cache = descriptorSetCaches_.at(swapImageIndex_);
    u32 subpassIdx = set.getSubpassIndex();
    u32 setIdx = set.getSetIndex();

    // Get the layout for this set in this subpass
    VkDescriptorSetLayout layout = pass.getSubpass(subpassIdx).getSetLayout(setIdx);

    // See if the cache can give us an unused descriptor set with the same layout
    VkDescriptorSet dstSet = cache.findSetWithLayout(layout);

    if (!dstSet) {
        // We didn't find a descriptor set with the right layout, we need to allocate a new one
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pools_[swapImageIndex_];
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        Log::verbose("Allocating new descriptor set for frame %, subpass %, set %", swapImageIndex_, subpassIdx, setIdx);

        // TODO: handle case where we can't allocate new descriptor set
        VK_CHECKF(vkAllocateDescriptorSets(device_, &allocInfo, &dstSet));

        // Add it to the cache so we can find it next time we need a descriptor set with this layout
        cache.addToCache(layout, dstSet);
    }

    // We don't cache by data yet, so we always write to the descriptor set
    // TODO: cache by descriptor set data to prevent unnecessary updates

    Framebuffer &framebuffer = getFramebuffer(pass);
    std::vector<VkWriteDescriptorSet> writes;

    //----------------------------------
    // Input attachments
    //----------------------------------

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(set.getInputAttachmentInfos().size() + set.getCombinedImageSamplerInfos().size());

    for (const InputAttachmentDescriptorInfo &desc : set.getInputAttachmentInfos()) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler = VK_NULL_HANDLE;
        imageInfo.imageView = framebuffer.getView(desc.attachmentName);
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.emplace_back(imageInfo);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = dstSet;
        write.dstBinding = desc.binding;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        write.pImageInfo = &imageInfos.back();
        writes.emplace_back(write);
    }

    //----------------------------------
    // Uniform buffers
    //----------------------------------

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(set.getUniformBufferInfos().size());

    const u8 *srcPtr = set.getUniformBufferData().data();
    u8 *dstPtr = reinterpret_cast<u8 *>(uniformBufferMappedPointers_.at(swapImageIndex_));

    VkDeviceSize &dstOffset = uniformBufferOffsets_.at(swapImageIndex_);
    u32 minAlignment = (u32) limits_.minUniformBufferOffsetAlignment;

    // Create descriptor writes that reference uniform buffer data
    VkBuffer buffer = uniformBuffers_.at(swapImageIndex_);
    for (const UniformBufferDescriptorInfo &info : set.getUniformBufferInfos()) {
        if (dstOffset + info.dataRange > uniformBufferSize_) {
            Log::fatal("This uniform buffer for descriptor set % in subpass % will overrun the buffer! "
                       "Too much data was set via uniform buffers this frame.",
                       set.getSetIndex(), set.getSubpassIndex());
        }

        // Copy data into buffer
        std::memcpy(dstPtr + dstOffset, srcPtr + info.dataOffset, info.dataRange);

        // Set buffer info
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = dstOffset;
        bufferInfo.range = info.dataRange;
        bufferInfos.emplace_back(bufferInfo);

        // Add to dstOffset, taking alignment into account
        dstOffset += info.dataRange;
        if (dstOffset % minAlignment != 0) {
            dstOffset += minAlignment - (dstOffset % minAlignment);
        }

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = dstSet;
        write.dstBinding = info.binding;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bufferInfos.back();

        writes.emplace_back(write);
    }

    //----------------------------------
    // Combined image samplers
    //----------------------------------

    for (const CombinedImageSamplerDescriptorInfo &info : set.getCombinedImageSamplerInfos()) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = info.view;
        imageInfo.sampler = info.sampler;
        imageInfos.emplace_back(imageInfo);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = dstSet;
        write.dstBinding = info.binding;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfos.back();

        writes.emplace_back(write);
    }

    // TODO: support other descriptor types

    // Write to descriptor set
    vkUpdateDescriptorSets(device_, writes.size(), writes.data(), 0, nullptr);

    return dstSet;
}

VkFormat RenderDevice::getFirstSupportedFormat(const std::vector<VkFormat> &formats,
                                               VkFormatFeatureFlags feature, VkImageTiling tiling) {
    for (VkFormat format : formats) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &properties);

        if ((tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & feature) == feature) ||
            (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & feature) == feature)) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
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
        Log::info("| %", properties.deviceName);
        Log::info("|   Version:  %.%.%", VK_VERSION_TO_COMMA_SEPARATED_VALUES(properties.apiVersion));
        Log::info("|   Num Exts: %", numDeviceExtensions);
        Log::info("|   Discrete: %", properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "yes" : "no");
        Log::info("|   Graphics: %", graphicsFamilyIndex.first ? "yes" : "no");
        Log::info("|   Compute:  %", computeFamilyIndex.first ? "yes" : "no");
        Log::info("|   Present:  %", presentFamilyIndex.first ? "yes" : "no");
        Log::info("|   Suitable: %", suitable ? "yes" : "no");
        Log::info("+-------------------------------");
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        Log::fatal("Failed to find a suitable physical device");
    }

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    limits_ = properties.limits;

    Log::info("Using physical device: %", properties.deviceName);
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

        Log::fatal("Unknown desired present mode: %", static_cast<u32>(options_.desiredPresentMode));
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

VkBuffer RenderDevice::createBufferGPU(const void *data, VkDeviceSize size, VkBufferUsageFlagBits usage) {
    // Create our buffer and memory
    VkBufferCreateInfo bufferCI = {};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = size;
    bufferCI.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI = {};
    allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocInfo;
    VK_CHECKF(vmaCreateBuffer(allocator_, &bufferCI, &allocCI, &buffer, &allocation, &allocInfo));
    cleanupStack_.emplace([ = ]() {
        vmaDestroyBuffer(allocator_, buffer, allocation);
    });

    VkMemoryPropertyFlags memFlags;
    vmaGetMemoryTypeProperties(allocator_, allocInfo.memoryType, &memFlags);
    if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
        // If the memory happens to be mappable, no need for staging buffer
        void *mappedData;
        vmaMapMemory(allocator_, allocation, &mappedData);
        std::memcpy(mappedData, data, (size_t) size);
        vmaUnmapMemory(allocator_, allocation);
    } else {
        // Otherwise, we need a staging buffer
        std::pair<VkBuffer, VmaAllocation> stagingBuffer = createStagingBufferCPU(data, size);

        // Copy staging buffer into vertex buffer
        submitOneTimeCommands(graphicsQueue_, [ = ](CommandBuffer cmd) {
            cmd.copyBuffer(buffer, stagingBuffer.first, size);
        });

        // Destroy staging buffer
        vmaDestroyBuffer(allocator_, stagingBuffer.first, stagingBuffer.second);
    }

    return buffer;
}

std::pair<VkBuffer, VmaAllocation> RenderDevice::createStagingBufferCPU(const void *data, VkDeviceSize size) {
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

    memcpy(stagingAllocInfo.pMappedData, data, static_cast<size_t>(size));

    return { stagingBuffer, stagingAllocation };
}

}
