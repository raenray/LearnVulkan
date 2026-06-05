#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Engine
{
namespace Asset
{

// Simple asynchronous asset loader.
// Supports GLTF meshes, textures (PNG/JPG/TGA via stb_image), and SPIR-V shaders.

enum class AssetType : uint8_t
{
    Unknown,
    Mesh,
    Texture,
    Shader,
    Material,
};

struct AssetHandle
{
    uint32_t id = 0;
    AssetType type = AssetType::Unknown;

    bool valid() const { return id != 0; }
};

struct MeshData
{
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<float> tangents;
    std::vector<uint32_t> indices;
    std::string materialId;
};

struct TextureData
{
    std::vector<uint8_t> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
};

struct MaterialData
{
    std::string albedoTexturePath;
    std::string normalTexturePath;
    std::string metallicRoughnessTexturePath;
    std::string aoTexturePath;
};

class AssetLoader
{
public:
    AssetLoader();
    ~AssetLoader();
    AssetLoader(const AssetLoader&) = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;

    // Set search paths
    void addSearchPath(const std::string& path);
    void setShaderPath(const std::string& path);

    // Synchronous load (returns immediately)
    AssetHandle loadMesh(const std::string& path);
    AssetHandle loadTexture(const std::string& path);
    AssetHandle loadShader(const std::string& path, const std::string& entryPoint = "main");

    // Access loaded data
    const MeshData* getMeshData(AssetHandle handle) const;
    const TextureData* getTextureData(AssetHandle handle) const;
    const std::vector<char>* getShaderData(AssetHandle handle) const;

    // Check if loaded
    bool isLoaded(AssetHandle handle) const;

    // Resolve full path given a relative asset path
    std::string resolvePath(const std::string& relativePath) const;

    using LoadCallback = std::function<void(AssetHandle)>;

    void setLoadCallback(LoadCallback cb) { loadCallback_ = std::move(cb); }

private:
    std::string readTextFile(const std::string& path);
    std::vector<char> readBinaryFile(const std::string& path);

    std::vector<std::string> searchPaths_;
    std::string shaderPath_;
    LoadCallback loadCallback_;

    uint32_t nextHandleId_ = 1;

    std::unordered_map<uint32_t, MeshData> meshes_;
    std::unordered_map<uint32_t, TextureData> textures_;
    std::unordered_map<uint32_t, std::vector<char>> shaders_;
};

} // namespace Asset
} // namespace Engine
