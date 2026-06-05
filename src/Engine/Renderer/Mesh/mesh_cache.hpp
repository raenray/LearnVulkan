#pragma once
#include "Engine/Renderer/Mesh/mesh.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Engine
{
namespace Renderer
{
class GpuAllocator;

// Owns a pool of named GPU meshes.  Call createMesh() to upload vertex/index
// data and store it under a string id; use get() to retrieve it later.
class MeshCache
{
public:
    MeshCache() = default;
    ~MeshCache() = default;
    MeshCache(const MeshCache&) = delete;
    MeshCache& operator=(const MeshCache&) = delete;

    // Upload a mesh from host-side vertex/index arrays.
    // The caller is responsible for supplying a valid VkCommandPool and VkQueue.
    void createMesh(const std::string& id,
                    const std::vector<Vertex>& vertices,
                    const std::vector<uint32_t>& indices,
                    void* vmaAllocator,
                    VkDevice device,
                    VkCommandPool cmdPool,
                    VkQueue queue);

    Mesh* get(const std::string& id);

    bool has(const std::string& id) const;

    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes_;
};
} // namespace Renderer
} // namespace Engine
