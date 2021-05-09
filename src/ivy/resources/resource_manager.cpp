#include "resource_manager.h"
#include "ivy/log.h"
#include "ivy/graphics/vertex.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

#include <meshoptimizer.h>

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

    std::vector<std::vector<gfx::Mesh>> lodMeshes(NUM_LOD);

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath.generic_string().c_str(),
                                             aiProcess_Triangulate | aiProcess_GenUVCoords |
                                             aiProcess_GenNormals | aiProcess_FlipUVs |
                                             aiProcess_CalcTangentSpace);
    if (!scene) {
        Log::warn("Failed to read resource %: %", filePath, importer.GetErrorString());
        return false;
    }

    // Process meshes
    for (u32 m = 0; m < scene->mNumMeshes; ++m) {
        std::vector<gfx::VertexP3N3T3B3UV2> vertices;
        std::vector<u32> indices;

        aiMesh *mesh = scene->mMeshes[m];

        // Process vertices
        vertices.reserve(mesh->mNumVertices);
        for (u32 v = 0; v < mesh->mNumVertices; ++v) {
            vertices.emplace_back();
            gfx::VertexP3N3T3B3UV2 &vert = vertices.back();
            vert.position = glm::vec3(
                                mesh->mVertices[v].x,
                                mesh->mVertices[v].y,
                                mesh->mVertices[v].z
                            );

            vert.normal = glm::vec3(
                              mesh->mNormals[v].x,
                              mesh->mNormals[v].y,
                              mesh->mNormals[v].z
                          );

            // If there's no UV, tangent and bi-tangent can't be generated
            // TODO: figure out solution or don't

            if (mesh->mTangents) {
                vert.tangent = glm::vec3(
                                   mesh->mTangents[v].x,
                                   mesh->mTangents[v].y,
                                   mesh->mTangents[v].z
                               );
            }

            if (mesh->mBitangents) {
                vert.bitangent = glm::vec3(
                                     mesh->mBitangents[v].x,
                                     mesh->mBitangents[v].y,
                                     mesh->mBitangents[v].z
                                 );
            }

            if (mesh->mTextureCoords[0]) {
                vert.uv = glm::vec2(
                              mesh->mTextureCoords[0][v].x,
                              mesh->mTextureCoords[0][v].y
                          );
            }
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
            aiString texPath;
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
            diffuseTexture = &getTexture(relativeDirectory + texPath.C_Str()).get();
        }

        // Normal
        const gfx::Texture *normalTexture = textureNormal_;
        if (aiMat->GetTextureCount(aiTextureType_NORMALS) > 0) {
            aiString texPath;
            aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath);
            normalTexture = &getTexture(relativeDirectory + texPath.C_Str()).get();
        }

        // Ambient occlusion
        const gfx::Texture *occlusionTexture = textureWhite_;
        if (aiMat->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0) {
            aiString texPath;
            aiMat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath);
            occlusionTexture = &getTexture(relativeDirectory + texPath.C_Str()).get();
        }

        // Roughness
        const gfx::Texture *roughnessTexture = textureWhite_;
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0) {
            aiString texPath;
            aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath);
            roughnessTexture = &getTexture(relativeDirectory + texPath.C_Str()).get();
        }

        // Metal
        const gfx::Texture *metallicTexture = textureBlackTransparent_;
        if (aiMat->GetTextureCount(aiTextureType_METALNESS) > 0) {
            aiString texPath;
            aiMat->GetTexture(aiTextureType_METALNESS, 0, &texPath);
            metallicTexture = &getTexture(relativeDirectory + texPath.C_Str()).get();
        }

        // If ao, roughness, and metallic are in same texture
        aiString texPath;
        if (aiMat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &texPath) == aiReturn_SUCCESS) {
            loadTextureFromFile(relativeDirectory + texPath.C_Str(), true);

            // Guess what channels hold what data
            occlusionTexture = &getTexture(relativeDirectory + texPath.C_Str() + "_r").get();
            roughnessTexture = &getTexture(relativeDirectory + texPath.C_Str() + "_g").get();
            metallicTexture = &getTexture(relativeDirectory + texPath.C_Str() + "_b").get();
        }

        // Optimize mesh
        std::vector<u32> remap(indices.size());
        meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(), vertices.data(), vertices.size(),
                                    sizeof(vertices[0]));

        std::vector<u32> optimizedIndices(indices.size());
        meshopt_remapIndexBuffer(optimizedIndices.data(), indices.data(), indices.size(), remap.data());

        std::vector<gfx::VertexP3N3T3B3UV2> optimizedVertices(vertices.size());
        meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), vertices.size(), sizeof(vertices[0]),
                                  remap.data());

        // Save optimized mesh
        lodMeshes[0].emplace_back(
            gfx::Geometry(device_, optimizedVertices, optimizedIndices),
            gfx::Material(*diffuseTexture, *normalTexture, *occlusionTexture, *roughnessTexture, *metallicTexture)
        );

        // Generate LODs
        std::vector<gfx::VertexP3> vertexPositions(optimizedVertices.size());
        for (u32 i = 0; i < optimizedVertices.size(); ++i) {
            vertexPositions[i].position = optimizedVertices[i].position;
        }

        u32 lastIndexCount = optimizedIndices.size();
        for (u32 i = 1; i < NUM_LOD; ++i) {
            // Half the number of indices each lod level
            u32 targetIndices = (u32)((f32)optimizedIndices.size() * std::pow(0.5f, i));
            f32 targetError = 1e-2f;

            std::vector<u32> lodIndices(optimizedIndices.size());
            lodIndices.resize(meshopt_simplify(lodIndices.data(), optimizedIndices.data(), optimizedIndices.size(),
                                               &vertexPositions[0].position.x, vertexPositions.size(), sizeof(vertexPositions[0]),
                                               targetIndices, targetError));

            if (lodIndices.size() == lastIndexCount || lodIndices.empty()) {
                // Re-use mesh from previous LOD
                lodMeshes[i].emplace_back(lodMeshes[i - 1].back());
            } else {
                // Create new mesh but reuse vertex buffer from LOD0, we're just changing indices
                lodMeshes[i].emplace_back(gfx::Geometry(device_, lodMeshes[0].back().getGeometry(), lodIndices),
                                          gfx::Material(*diffuseTexture, *normalTexture, *occlusionTexture, *roughnessTexture, *metallicTexture));
            }

            lastIndexCount = lodIndices.size();
        }
    }

    modelMeshes_.emplace(model_path, std::make_unique<std::vector<std::vector<gfx::Mesh>>>(lodMeshes));
    return true;
}

bool ResourceManager::loadTextureFromFile(const std::string &texture_path, bool split_channels) {
    std::string full = resourceDirectory_ + texture_path;
    std::replace(full.begin(), full.end(), '\\', '/');
    std::filesystem::path filePath = std::filesystem::path(full).lexically_normal();

    // Read image
    i32 width, height;
    u8 *data = stbi_load(filePath.string().c_str(), &width, &height, nullptr, 4);
    if (!data) {
        return false;
    }
    u32 size = width * height * 4;

    loadTexture(texture_path, width, height, VK_FORMAT_R8G8B8A8_UNORM, data, size);

    if (split_channels) {
        std::vector<char> suffix = {'r', 'g', 'b', 'a'};
        std::vector<std::vector<u8>> channels(suffix.size(), std::vector<u8>(width * height));

        // Split channels
        for (u32 i = 0; i < size; ++i) {
            channels[i % channels.size()][i / channels.size()] = data[i];
        }

        // Load textures for each channel
        for (u32 i = 0; i < suffix.size(); ++i) {
            loadTexture(texture_path + "_" + suffix[i], width, height, VK_FORMAT_R8_UNORM,
                        channels[i].data(), channels[i].size());
        }
    }

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
