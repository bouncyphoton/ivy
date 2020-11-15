#include "render_context.h"
#include "vk_utils.h"
#include "ivy/types.h"
#include "ivy/consts.h"
#include "ivy/utils/utils.h"
#include "ivy/platform/platform.h"

#include <GLFW/glfw3.h>

namespace ivy {

constexpr u32 VULKAN_API_VERSION = VK_API_VERSION_1_1;

RenderContext::RenderContext(const Platform &platform) {
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
}

RenderContext::~RenderContext() {
    LOG_CHECKPOINT();

    vkDestroySurfaceKHR(instance_, surface_, nullptr);

    vkDestroyInstance(instance_, nullptr);
}

std::vector<const char *> RenderContext::getInstanceExtensions() {
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

}
