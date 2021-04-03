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
    /**
     * \brief The maximum LOD level
     */
    static constexpr u32 MAX_LOD = 8;

    /**
     * \brief The number of LOD levels
     */
    static constexpr u32 NUM_LOD = MAX_LOD + 1;

    explicit ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory);

    /**
     * \brief Get a handle to a 3D model. If the model has not been loaded then the resource manager
     * will load it from the filesystem (path: resource_directory/model_name)
     * \param model_name The name of the model resource
     * \return A resource handle to the model
     */
    ModelResource getModel(const std::string &model_name);

    /**
     * \brief Get a handle to a texture. If the texture has not been loaded then the resource manager
     * will load it from the filesystem (path: resource_directory/texture_name).
     * \param texture_name The name of the texture resource
     * \return A resource handle to the texture
     */
    TextureResource getTexture(const std::string &texture_name);

private:
    /**
     * \brief Load a model into the resource manager
     * \param model_path The path to the resource relative to the resource directory
     * \return Whether or not the model was loaded successfully
     */
    bool loadModelFromFile(const std::string &model_path);

    bool loadTextureFromFile(const std::string &texture_path);

    void loadTexture(const std::string &name, u32 width, u32 height, VkFormat format, u8 *data, u32 size);

    gfx::RenderDevice &device_;
    std::string resourceDirectory_;

    std::unordered_map<std::string, std::unique_ptr<std::vector<std::vector<gfx::Mesh>>>> modelMeshes_;
    std::unordered_map<std::string, std::unique_ptr<gfx::Texture>> textures_;

    gfx::Texture *textureWhite_;
    gfx::Texture *textureBlackOpaque_;
    gfx::Texture *textureBlackTransparent_;
    gfx::Texture *textureNormal_;
    gfx::Texture *textureMissing_;
};

}

#endif // IVY_RESOURCE_MANAGER_H
