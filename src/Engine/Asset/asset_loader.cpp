#include "Engine/Asset/asset_loader.hpp"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

namespace Engine { namespace Asset {

AssetLoader::AssetLoader() {
    // Default search paths
    searchPaths_.push_back("assets/");
    searchPaths_.push_back("../assets/");
    shaderPath_ = "assets/shaders/";
}

AssetLoader::~AssetLoader() = default;

void AssetLoader::addSearchPath(const std::string& path) {
    searchPaths_.push_back(path);
}

void AssetLoader::setShaderPath(const std::string& path) {
    shaderPath_ = path;
}

std::string AssetLoader::resolvePath(const std::string& relativePath) const {
    // If path exists as-is, return it
    if (std::filesystem::exists(relativePath))
        return relativePath;

    // Try search paths
    for (const auto& base : searchPaths_) {
        std::string candidate = base + "/" + relativePath;
        if (std::filesystem::exists(candidate))
            return candidate;
        // Try without double slash
        if (base.back() == '/')
            candidate = base + relativePath;
        else
            candidate = base + "/" + relativePath;
        if (std::filesystem::exists(candidate))
            return candidate;
    }

    // Fallback: return the original path and let the caller handle the error
    return relativePath;
}

AssetHandle AssetLoader::loadMesh(const std::string& path) {
    // Minimal OBJ-like loader (placeholder)
    // In a real engine this would use tinygltf or assimp.
    std::string fullPath = resolvePath(path);
    AssetHandle h{nextHandleId_++, AssetType::Mesh};

    MeshData data;
    // Simple hardcoded cube for demo purposes
    // (Real implementation would parse GLTF/OBJ)
    data.positions = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    };
    data.normals = {
         0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,
         0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
    };
    data.texcoords = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    };
    data.indices = {
        0, 1, 2, 2, 3, 0,  // front
        4, 5, 6, 6, 7, 4,  // back
    };

    meshes_[h.id] = std::move(data);
    if (loadCallback_) loadCallback_(h);
    return h;
}

AssetHandle AssetLoader::loadTexture(const std::string& path) {
    std::string fullPath = resolvePath(path);
    AssetHandle h{nextHandleId_++, AssetType::Texture};

    TextureData data;
    // Placeholder: 1x1 white texture
    // Real implementation would use stb_image
    data.width = 1;
    data.height = 1;
    data.channels = 4;
    data.pixels = {255, 255, 255, 255};

    textures_[h.id] = std::move(data);
    if (loadCallback_) loadCallback_(h);
    return h;
}

AssetHandle AssetLoader::loadShader(const std::string& path, const std::string& entryPoint) {
    std::string fullPath = resolvePath(shaderPath_ + path);
    AssetHandle h{nextHandleId_++, AssetType::Shader};

    auto code = readBinaryFile(fullPath);
    shaders_[h.id] = std::move(code);

    if (loadCallback_) loadCallback_(h);
    return h;
}

const MeshData* AssetLoader::getMeshData(AssetHandle handle) const {
    auto it = meshes_.find(handle.id);
    return (it != meshes_.end()) ? &it->second : nullptr;
}

const TextureData* AssetLoader::getTextureData(AssetHandle handle) const {
    auto it = textures_.find(handle.id);
    return (it != textures_.end()) ? &it->second : nullptr;
}

const std::vector<char>* AssetLoader::getShaderData(AssetHandle handle) const {
    auto it = shaders_.find(handle.id);
    return (it != shaders_.end()) ? &it->second : nullptr;
}

bool AssetLoader::isLoaded(AssetHandle handle) const {
    switch (handle.type) {
        case AssetType::Mesh: return meshes_.find(handle.id) != meshes_.end();
        case AssetType::Texture: return textures_.find(handle.id) != textures_.end();
        case AssetType::Shader: return shaders_.find(handle.id) != shaders_.end();
        default: return false;
    }
}

std::string AssetLoader::readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("AssetLoader: failed to open '" + path + "'");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::vector<char> AssetLoader::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("AssetLoader: failed to open '" + path + "'");
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(size));
    return buffer;
}

} }
