#include "vk_utils.h"
#include "ivy/types.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>

namespace ivy::gfx {

const char *vk_result_to_string(VkResult result) {
#define VULKAN_CASE(x) case x: return #x;
    switch (result) {
            VULKAN_CASE(VK_SUCCESS)
            VULKAN_CASE(VK_NOT_READY)
            VULKAN_CASE(VK_TIMEOUT)
            VULKAN_CASE(VK_EVENT_SET)
            VULKAN_CASE(VK_EVENT_RESET)
            VULKAN_CASE(VK_INCOMPLETE)
            VULKAN_CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
            VULKAN_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
            VULKAN_CASE(VK_ERROR_INITIALIZATION_FAILED)
            VULKAN_CASE(VK_ERROR_DEVICE_LOST)
            VULKAN_CASE(VK_ERROR_MEMORY_MAP_FAILED)
            VULKAN_CASE(VK_ERROR_LAYER_NOT_PRESENT)
            VULKAN_CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
            VULKAN_CASE(VK_ERROR_FEATURE_NOT_PRESENT)
            VULKAN_CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
            VULKAN_CASE(VK_ERROR_TOO_MANY_OBJECTS)
            VULKAN_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
            VULKAN_CASE(VK_ERROR_FRAGMENTED_POOL)
            VULKAN_CASE(VK_ERROR_UNKNOWN)
            VULKAN_CASE(VK_ERROR_OUT_OF_POOL_MEMORY)
            VULKAN_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE)
            VULKAN_CASE(VK_ERROR_FRAGMENTATION)
            VULKAN_CASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
            VULKAN_CASE(VK_ERROR_SURFACE_LOST_KHR)
            VULKAN_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
            VULKAN_CASE(VK_SUBOPTIMAL_KHR)
            VULKAN_CASE(VK_ERROR_OUT_OF_DATE_KHR)
            VULKAN_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
            VULKAN_CASE(VK_ERROR_VALIDATION_FAILED_EXT)
            VULKAN_CASE(VK_ERROR_INVALID_SHADER_NV)
            VULKAN_CASE(VK_ERROR_INCOMPATIBLE_VERSION_KHR)
            VULKAN_CASE(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
            VULKAN_CASE(VK_ERROR_NOT_PERMITTED_EXT)
            VULKAN_CASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
            VULKAN_CASE(VK_THREAD_IDLE_KHR)
            VULKAN_CASE(VK_THREAD_DONE_KHR)
            VULKAN_CASE(VK_OPERATION_DEFERRED_KHR)
            VULKAN_CASE(VK_OPERATION_NOT_DEFERRED_KHR)
            VULKAN_CASE(VK_PIPELINE_COMPILE_REQUIRED_EXT)
            VULKAN_CASE(VK_RESULT_MAX_ENUM)
        default:
            return "Unknown";
    }
#undef VULKAN_CASE
}

VkExtent2D clamp(VkExtent2D x, VkExtent2D min_ext, VkExtent2D max_ext) {
    return {
        std::min(max_ext.width, std::max(min_ext.width, x.width)),
        std::min(max_ext.height, std::max(min_ext.height, x.height))
    };
}

std::vector<const char *> getInstanceExtensions() {
    // Get required extensions from glfw
    u32 numGlfwExtensions;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
    std::vector<const char *> exts(glfwExtensions, glfwExtensions + numGlfwExtensions);

    // Check if extensions are supported
    u32 numSupportedExtensions;
    VK_CHECKF(vkEnumerateInstanceExtensionProperties(nullptr, &numSupportedExtensions, nullptr));
    std::vector<VkExtensionProperties> supportedExtensions(numSupportedExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numSupportedExtensions, supportedExtensions.data());

    // Check each requested extension
    std::string unsupportedList;
    for (const char *ext : exts) {
        bool found = false;

        // Search for extension in supported vector
        for (VkExtensionProperties props : supportedExtensions) {
            if (std::string(props.extensionName) == ext) {
                found = true;
                break;
            }
        }

        // If not found, keep track of it for logging later
        if (!found) {
            if (!unsupportedList.empty()) {
                unsupportedList += ", ";
            }
            unsupportedList += ext;
        }
    }

    // Fatal error if we requested an extension that isn't supported
    if (!unsupportedList.empty()) {
        Log::fatal("The following instance extensions are not supported: %s", unsupportedList.c_str());
    }

    return exts;
}

std::vector<const char *> getDeviceExtensions() {
    return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

    return capabilities;
}

std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    u32 numFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &numFormats, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(numFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &numFormats, formats.data());

    return formats;
}

std::vector<VkPresentModeKHR> getPresentModes(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    u32 numPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &numPresentModes, nullptr);
    std::vector<VkPresentModeKHR> presentModes(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &numPresentModes, presentModes.data());

    return presentModes;
}

VkShaderModule loadShader(VkDevice device, const std::string &shader_path) {
    std::vector<char> code;

    if (std::ifstream file = std::ifstream(shader_path, std::ios::ate | std::ios::binary)) {
        u32 size = (u32) file.tellg();
        code.resize(size);
        file.seekg(0);
        file.read(code.data(), size);
    } else {
        Log::fatal("Failed to load shader '%s'", shader_path.c_str());
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<u32 *>(code.data());

    VkShaderModule shaderModule;
    VK_CHECKF(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

}
