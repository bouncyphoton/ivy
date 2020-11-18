#ifndef IVY_VK_UTILS_H
#define IVY_VK_UTILS_H

#include "ivy/log.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace ivy {

/**
 * \brief Stringify a VkResult
 */
const char *vk_result_to_string(VkResult result);

/**
 * \brief Clamp a VkExtent2D
 */
VkExtent2D clamp(VkExtent2D x, VkExtent2D min_ext, VkExtent2D max_ext);

/**
 * \brief Get a vector of required instance extensions, will error if at least one extension is unsupported
 * \return Vector of supported instance extensions
 */
std::vector<const char *> getInstanceExtensions();

/**
 * \brief Get a vector of required device extensions
 * \return Vector of device extensions
 */
std::vector<const char *> getDeviceExtensions();

/**
 * \brief Get the surface capabilities for a given physical device and surface
 * \param physical_device
 * \param surface
 * \return Surface capabilities
 */
VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/**
 * \brief Get vector of supported swapchain format-color space pairs for a surface
 * \param physical_device
 * \param surface
 * \return Vector of color formats supported by surface
 */
std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/**
 * \brief Get present modes for a given physical device and surface
 * \param physical_device
 * \param surface
 * \return Vector of present modes
 */
std::vector<VkPresentModeKHR> getPresentModes(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

}

/**
 * \brief Check a VkResult, fatal error if not VK_SUCCESS
 */
#define VK_CHECKF(x) do { VkResult result = (x); if (result != VK_SUCCESS) { \
            ivy::Log::fatal("%s:%d: [%s] %s", __FILE__, __LINE__, vk_result_to_string(result), #x); } } while (0)

/**
 * \brief Check a VkResult, warn if not VK_SUCCESS
 */
#define VK_CHECKW(x) do { VkResult result = (x); if (result != VK_SUCCESS) { \
            ivy::Log::warn("%s:%d: [%s] %s", __FILE__, __LINE__, vk_result_to_string(result), #x); } } while (0)

/**
 * \brief Separate a VK_VERSION into major, minor, patch
 */
#define VK_VERSION_TO_COMMA_SEPARATED_VALUES(x) \
    VK_VERSION_MAJOR(x), VK_VERSION_MINOR(x), VK_VERSION_PATCH(x)

#endif // IVY_VK_UTILS_H
