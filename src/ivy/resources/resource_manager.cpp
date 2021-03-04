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
        Log::fatal("Invalid resource directory: '%'", resourceDirectory_);
    }

    // Load default cube
    loadModel("cube", {
        gfx::Geometry(device_,
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
        })
    });
}

ModelResource ResourceManager::getModel(const std::string &model_name) {
    auto it = modelDatas_.find(model_name);
    if (it == modelDatas_.end()) {
        if (!loadModelFromFile(model_name)) {
            Log::fatal("Failed to get model '%'", model_name);
        }

        it = modelDatas_.find(model_name);
    }

    return ModelResource(*it->second);
}

bool ResourceManager::loadModelFromFile(const std::string &resource_path) {
    std::filesystem::path filePath = std::filesystem::path(resourceDirectory_ + resource_path).lexically_normal();

    if (!std::filesystem::is_regular_file(filePath)) {
        Log::warn("Invalid resource location: '%'", filePath);
        return false;
    }

    if (modelDatas_.find(resource_path) != modelDatas_.end()) {
        Log::warn("Tried to load already loaded resource: '%'",  resource_path);
        return false;
    }

    std::vector<gfx::Geometry> geometries;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath.generic_string().c_str(), aiProcess_Triangulate);
    if (!scene) {
        Log::warn("Failed to read resource '%': %", filePath, importer.GetErrorString());
        return false;
    }

    for (u32 m = 0; m < scene->mNumMeshes; ++m) {
        std::vector<gfx::VertexP3C3> vertices;
        std::vector<u32> indices;

        aiMesh *mesh = scene->mMeshes[m];

        vertices.reserve(mesh->mNumVertices);
        for (u32 v = 0; v < mesh->mNumVertices; ++v) {
            vertices.emplace_back(gfx::VertexP3C3(
                                      glm::vec3(
                                          mesh->mVertices[v].x,
                                          mesh->mVertices[v].y,
                                          mesh->mVertices[v].z),
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

        geometries.emplace_back(device_, vertices, indices);
    }

    loadModel(resource_path, geometries);
    return true;
}

void ResourceManager::loadModel(const std::string &name, const std::vector<gfx::Geometry> &meshes) {
    modelDatas_.emplace(name, std::make_unique<std::vector<gfx::Geometry>>(meshes));
}

}
