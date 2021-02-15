#include "resource_manager.h"
#include "ivy/log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

namespace ivy {

ResourceManager::ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory)
    : device_(render_device), resourceDirectory_(resource_directory + "/") {
    if (!std::filesystem::is_directory(resourceDirectory_)) {
        Log::fatal("Invalid resource directory: '%s'", resourceDirectory_.c_str());
    }

    // Load default cube
    meshDatas_.emplace("cube", gfx::Geometry(device_,
    std::vector<gfx::VertexP3C3> {
        gfx::VertexP3C3({-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}),
        gfx::VertexP3C3({ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}),
        gfx::VertexP3C3({-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}),
        gfx::VertexP3C3({ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}),

        gfx::VertexP3C3({-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}),
        gfx::VertexP3C3({ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}),
        gfx::VertexP3C3({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}),
        gfx::VertexP3C3({ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}),
    }, std::vector<u32> {
        0, 1, 2, 2, 1, 3,
        2, 3, 4, 4, 3, 5,
        4, 5, 6, 6, 5, 7,
        6, 7, 0, 0, 7, 1,
        1, 7, 3, 3, 7, 5,
        6, 0, 4, 4, 0, 2
    }));
}

void ResourceManager::loadResource(const std::string &resource_path) {
    std::filesystem::path filePath = std::filesystem::path(resourceDirectory_ + resource_path).lexically_normal();

    if (!std::filesystem::is_regular_file(filePath)) {
        Log::fatal("Invalid resource location: '%s'", filePath.c_str());
    }

    if (meshDatas_.find(resource_path) != meshDatas_.end()) {
        Log::fatal("Tried to load already loaded resource: '%s'",  resource_path.c_str());
    }

    std::vector<gfx::VertexP3C3> vertices;
    std::vector<u32> indices;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath.c_str(), aiProcess_Triangulate);
    if (!scene) {
        Log::fatal("Failed to read resource '%s': %s", filePath.c_str(), importer.GetErrorString());
    }

    // TODO: load multiple meshes per model, currently we just load the first one
    for (u32 m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh *mesh = scene->mMeshes[m];

        vertices.reserve(mesh->mNumVertices);
        for (u32 v = 0; v < mesh->mNumVertices; ++v) {
            vertices.emplace_back(gfx::VertexP3C3(
                                      glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z),
                                      glm::vec3(1)
                                  ));
        }

        indices.reserve(mesh->mNumFaces * 3);
        for (u32 f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];
            for (u32 i = 0; i < face.mNumIndices; ++i) {
                indices.emplace_back(face.mIndices[i]);
            }
        }

        // TODO: material
    }

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
