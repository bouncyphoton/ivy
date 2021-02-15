#include "resource_manager.h"
#include "ivy/log.h"
#include <filesystem>

namespace ivy {

ResourceManager::ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory)
    : device_(render_device), resourceDirectory_(resource_directory + "/") {
    if (!std::filesystem::is_directory(resourceDirectory_)) {
        Log::fatal("Invalid resource directory: '%s'", resourceDirectory_.c_str());
    }
}

void ResourceManager::loadResource(const std::string &resource_path) {
    if (!std::filesystem::is_regular_file(resourceDirectory_ + resource_path)) {
        Log::fatal("Invalid resource location: '%s'",
                   std::filesystem::path(resourceDirectory_ + resource_path).lexically_normal().c_str());
    }

    if (meshDatas_.find(resource_path) != meshDatas_.end()) {
        Log::fatal("Tried to load already loaded resource: '%s'",  resource_path.c_str());
    }

    // TODO: actually load instead of hard coding

    std::vector<gfx::VertexP3C3> vertices = {
        gfx::VertexP3C3({-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}),
        gfx::VertexP3C3({ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}),
        gfx::VertexP3C3({-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}),
        gfx::VertexP3C3({ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}),

        gfx::VertexP3C3({-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}),
        gfx::VertexP3C3({ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}),
        gfx::VertexP3C3({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}),
        gfx::VertexP3C3({ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}),
    };
    std::vector<u32> indices = {
        0, 1, 2, 2, 1, 3,
        2, 3, 4, 4, 3, 5,
        4, 5, 6, 6, 5, 7,
        6, 7, 0, 0, 7, 1,
        1, 7, 3, 3, 7, 5,
        6, 0, 4, 4, 0, 2
    };

    meshDatas_.emplace(resource_path, gfx::Geometry(device_, vertices, indices));
}

MeshResource ResourceManager::getMesh(const std::string &mesh_name) {
    auto it = meshDatas_.find(mesh_name);
    if (it == meshDatas_.end()) {
        Log::fatal("Tried to get unloaded resource: '%s'\n", mesh_name.c_str());
    }

    return MeshResource(it->second);
}

}
