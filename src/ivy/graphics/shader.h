#ifndef IVY_SHADER_H
#define IVY_SHADER_H

namespace ivy::gfx {

class Shader {
public:
    enum class StageEnum {
        VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
        FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
        // TODO: geometry and compute
    };

    Shader(StageEnum stage, const std::string &path) : stage_(stage), path_(path) {}

    [[nodiscard]] VkShaderStageFlagBits getStage() const {
        return static_cast<VkShaderStageFlagBits>(stage_);
    }

    [[nodiscard]] std::string getShaderPath() const {
        return path_;
    }

private:
    StageEnum stage_;
    std::string path_;
};

}

#endif //IVY_SHADER_H
