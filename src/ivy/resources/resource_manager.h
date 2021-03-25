#ifndef IVY_RESOURCE_MANAGER_H
#define IVY_RESOURCE_MANAGER_H

#include "ivy/resources/model_resource.h"
#include "ivy/resources/texture_resource.h"
#include "ivy/graphics/mesh.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace ivy {

class ResourceManager {
public:
    explicit ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory);

    ModelResource getModel(const std::string &model_name);

    TextureResource getTexture(const std::string &texture_name);

private:
    /**
     * \brief Load a model into the resource manager
     * \param model_path The path to the resource relative to the resource directory
     * \return Whether or not the model was loaded successfully
     */
    bool loadModelFromFile(const std::string &model_path);

    void loadModel(const std::string &name, const std::vector<gfx::Mesh> &meshes);

    bool loadTextureFromFile(const std::string &texture_path);

    void loadTexture(const std::string &name, u32 width, u32 height, VkFormat format, u8 *data, u32 size);

    gfx::RenderDevice &device_;
    std::string resourceDirectory_;

    std::unordered_map<std::string, std::unique_ptr<std::vector<gfx::Mesh>>> modelMeshes_;
    std::unordered_map<std::string, std::unique_ptr<gfx::Texture>> textures_;

    gfx::Texture *textureWhite_;
    gfx::Texture *textureBlackOpaque_;
    gfx::Texture *textureBlackTransparent_;
    gfx::Texture *textureNormal_;
    gfx::Texture *textureMissing_;
};

}

#endif // IVY_RESOURCE_MANAGER_H
