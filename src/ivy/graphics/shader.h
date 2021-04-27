#ifndef IVY_SHADER_H
#define IVY_SHADER_H

#include <vulkan/vulkan.h>
#include <string>

namespace ivy::gfx {

/**
 * \brief Holds data for shader creation
 */
class Shader {
public:
    enum class StageEnum {
        VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
        FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
        GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
        COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT
    };

    Shader(StageEnum stage, const std::string &path) : stage_(stage), path_(path) {}

    /**
     * \brief Get the shader stage that this shader represents
     * \return VkShaderStageFlagBits
     */
    [[nodiscard]] VkShaderStageFlagBits getStage() const {
        return static_cast<VkShaderStageFlagBits>(stage_);
    }

    /**
     * \brief Get the path to the shader bytecode
     * \return Path to shader bytecode
     */
    [[nodiscard]] std::string getShaderPath() const {
        return path_;
    }

private:
    StageEnum stage_;
    std::string path_;
};

}

#endif //IVY_SHADER_H
