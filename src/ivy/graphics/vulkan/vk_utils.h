#ifndef IVY_VK_UTILS_H
#define IVY_VK_UTILS_H

#include "ivy/log.h"
#include <vulkan/vulkan.h>

namespace ivy {

/**
 * \brief Stringify a VkResult
 */
const char *vk_result_to_string(VkResult result);

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
