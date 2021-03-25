#include "resource_manager.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>

namespace ivy {

ResourceManager::ResourceManager(gfx::RenderDevice &render_device, const std::string &resource_directory)
    : device_(render_device), resourceDirectory_(resource_directory + "/") {
    if (!std::filesystem::is_directory(resourceDirectory_)) {
        Log::fatal("Invalid resource directory: '%'", resourceDirectory_);
    }

    // Load default textures
    {
        u8 pixels[] = {255, 255, 255, 255};
        loadTexture("*white", 1, 1, VK_FORMAT_R8G8B8A8_UNORM, pixels, sizeof(pixels));
        textureWhite_ = textures_.find("*white")->second.get();
    }
    {
        u8 pixels[] = {0, 0, 0, 255};
        loadTexture("*blackOpaque", 1, 1, VK_FORMAT_R8G8B8A8_UNORM, pixels, sizeof(pixels));
        textureBlackOpaque_ = textures_.find("*blackOpaque")->second.get();
    }
    {
        u8 pixels[] = {0, 0, 0, 0};
        loadTexture("*blackTransparent", 1, 1, VK_FORMAT_R8G8B8A8_UNORM, pixels, sizeof(pixels));
        textureBlackTransparent_ = textures_.find("*blackTransparent")->second.get();
    }
    {
        u8 pixels[] = {127, 127, 255, 255};
        loadTexture("*normal", 1, 1, VK_FORMAT_R8G8B8A8_UNORM, pixels, sizeof(pixels));
        textureNormal_ = textures_.find("*normal")->second.get();
    }
    {
        u8 pixels[] = {255, 0, 255, 255,
                       0, 0, 0, 255,
                       0, 0, 0, 255,
                       255, 0, 255, 255
                      };
        loadTexture("*missing", 2, 2, VK_FORMAT_R8G8B8A8_UNORM, pixels, sizeof(pixels));
        textureMissing_ = textures_.find("*missing")->second.get();
    }
}

ModelResource ResourceManager::getModel(const std::string &model_name) {
    auto it = modelMeshes_.find(model_name);
    if (it == modelMeshes_.end()) {
        if (!loadModelFromFile(model_name)) {
            Log::fatal("Failed to get model '%'", model_name);
        }

        it = modelMeshes_.find(model_name);
    }

    return ModelResource(*it->second);
}

TextureResource ResourceManager::getTexture(const std::string &texture_name) {
    auto it = textures_.find(texture_name);
    if (it == textures_.end()) {
        if (!loadTextureFromFile(texture_name)) {
            Log::warn("Failed to get texture '%'", texture_name);
            return TextureResource(*textureMissing_);
        }

        it = textures_.find(texture_name);
    }

    return TextureResource(*it->second);
}

bool ResourceManager::loadModelFromFile(const std::string &model_path) {
    std::filesystem::path filePath = std::filesystem::path(resourceDirectory_ + model_path).lexically_normal();
    std::string relativeDirectory = std::filesystem::path(model_path).parent_path().string() + "/";

    if (!std::filesystem::is_regular_file(filePath)) {
        Log::warn("Invalid resource location: '%'", filePath);
        return false;
    }

    if (modelMeshes_.find(model_path) != modelMeshes_.end()) {
        Log::warn("Tried to load already loaded resource: '%'", model_path);
        return false;
    }

    std::vector<gfx::Mesh> meshes;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath.generic_string().c_str(),
                                             aiProcess_Triangulate | aiProcess_GenUVCoords |
                                             aiProcess_GenNormals);
    if (!scene) {
        Log::warn("Failed to read resource '%': %", filePath, importer.GetErrorString());
        return false;
    }

    // Process meshes
    for (u32 m = 0; m < scene->mNumMeshes; ++m) {
        std::vector<gfx::VertexP3N3UV2> vertices;
        std::vector<u32> indices;

        aiMesh *mesh = scene->mMeshes[m];

        // Process vertices
        vertices.reserve(mesh->mNumVertices);
        for (u32 v = 0; v < mesh->mNumVertices; ++v) {
            vertices.emplace_back(gfx::VertexP3N3UV2(
                                      glm::vec3(
                                          mesh->mVertices[v].x,
                                          mesh->mVertices[v].y,
                                          mesh->mVertices[v].z),
                                      glm::vec3(
                                          mesh->mNormals[v].x,
                                          mesh->mNormals[v].y,
                                          mesh->mNormals[v].z
                                      ),
                                      glm::vec2(
                                          mesh->mTextureCoords[0][v].x,
                                          mesh->mTextureCoords[0][v].y
                                      )
                                  ));
        }

        // Process indices
        indices.reserve(mesh->mNumFaces * 3);
        for (u32 f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];
            for (u32 i = 0; i < face.mNumIndices; ++i) {
                indices.emplace_back(face.mIndices[i]);
            }
        }

        // Get material for this mesh
        aiMaterial *aiMat = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse
        const gfx::Texture *diffuseTexture = textureWhite_;
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString diffusePath;
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);
            diffuseTexture = &getTexture(relativeDirectory + diffusePath.C_Str()).get();
        }

        // Save mesh
        meshes.emplace_back(
            gfx::Geometry(device_, vertices, indices),
            gfx::Material(*diffuseTexture)
        );
    }

    loadModel(model_path, meshes);
    return true;
}

void ResourceManager::loadModel(const std::string &name, const std::vector<gfx::Mesh> &meshes) {
    modelMeshes_.emplace(name, std::make_unique<std::vector<gfx::Mesh>>(meshes));
}

bool ResourceManager::loadTextureFromFile(const std::string &texture_path) {
    std::filesystem::path filePath = std::filesystem::path(resourceDirectory_ + texture_path).lexically_normal();

    // Read image
    i32 width, height;
    u8 *data = stbi_load(filePath.string().c_str(), &width, &height, nullptr, 4);
    if (!data) {
        return false;
    }
    u32 size = width * height * 4;

    loadTexture(texture_path, width, height, VK_FORMAT_R8G8B8A8_UNORM, data, size);

    // Free image data
    stbi_image_free(data);
    return true;
}

void ResourceManager::loadTexture(const std::string &name, u32 width, u32 height, VkFormat format, u8 *data, u32 size) {
    textures_.emplace(name, std::make_unique<gfx::Texture>(
                          gfx::TextureBuilder(device_)
                          .setExtent2D(width, height)
                          .setFormat(format)
                          .setImageAspect(VK_IMAGE_ASPECT_COLOR_BIT)
                          .setData(data, size)
                          .build()));
}

}
