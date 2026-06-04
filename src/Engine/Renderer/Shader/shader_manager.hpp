#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include "Engine/Renderer/Shader/shader_reflection.hpp"
namespace Engine { namespace Renderer {
using ShaderHandle=uint32_t; constexpr ShaderHandle kInvalidShader=UINT32_MAX;
class ShaderManager {
public:
    explicit ShaderManager(VkDevice device); ~ShaderManager();
    ShaderManager(const ShaderManager&)=delete; ShaderManager& operator=(const ShaderManager&)=delete;
    ShaderHandle compileFile(const std::string& path, VkShaderStageFlagBits stage, const char* entryPoint="main");
    ShaderHandle compileSource(const std::string& name, const std::string& source, VkShaderStageFlagBits stage, const char* entryPoint="main");
    VkShaderModule getModule(ShaderHandle h) const;
    const ShaderReflection& reflection(ShaderHandle h) const;
    int hotReload();
private:
    VkDevice device_=VK_NULL_HANDLE;
    struct Entry{std::string name,filePath,entryPoint;VkShaderStageFlagBits stage;std::vector<uint32_t> spirv;VkShaderModule module=VK_NULL_HANDLE;ShaderReflection reflectionData;std::filesystem::file_time_type fileTime;};
    std::vector<Entry> entries_;
    struct Impl; std::unique_ptr<Impl> impl_;
    void compileEntry(Entry& e, const std::string& source); void reflectEntry(Entry& e); void destroyModule(Entry& e);
};
} }
