#include "render_context.h"
#include "vk_utils.h"
#include "ivy/types.h"
#include "ivy/consts.h"
#include "ivy/engine.h"
#include "ivy/utils/utils.h"
#include "ivy/platform/platform.h"
#include <GLFW/glfw3.h>
#include <set>
#include <algorithm>

namespace ivy {

constexpr u32 VULKAN_API_VERSION = VK_API_VERSION_1_1;

RenderContext::RenderContext(const Options &options, const Platform &platform)
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

    //----------------------------------
    // Surface creation
    //----------------------------------

    VK_CHECKF(glfwCreateWindowSurface(instance_, platform.getGlfwWindow(), nullptr, &surface_));

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

    //----------------------------------
    // Get our queues
    //----------------------------------

    vkGetDeviceQueue(device_, graphicsFamilyIndex_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, computeFamilyIndex_, 0, &computeQueue_);
    vkGetDeviceQueue(device_, presentFamilyIndex_, 0, &presentQueue_);

    //----------------------------------
    // Create our swapchain
    //----------------------------------

    createSwapchain();
}

RenderContext::~RenderContext() {
    LOG_CHECKPOINT();

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

void RenderContext::choosePhysicalDevice() {
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

void RenderContext::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities = getSurfaceCapabilities(physicalDevice_, surface_);

    // Clamp our desired image count to minimum
    u32 imageCount = std::max(consts::DESIRED_SWAPCHAIN_IMAGES, capabilities.minImageCount);
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
    for (VkPresentModeKHR m : presentModes) {
        // Prefer mailbox if it's available
        if (m == consts::DESIRED_PRESENT_MODE) {
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
    }
}

}
