#include "Engine/Renderer/Shader/shader_manager.hpp"
#include <shaderc/shaderc.hpp>
#include <spirv_cross_c.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>

namespace Engine
{
namespace Renderer
{
static std::string readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::ate);
    if (!f)
        throw std::runtime_error("ShaderManager: cannot open " + path);
    size_t sz = (size_t) f.tellg();
    std::string buf(sz, '\0');
    f.seekg(0);
    f.read(buf.data(), sz);
    return buf;
}

static shaderc_shader_kind toKind(VkShaderStageFlagBits s)
{
    switch (s)
    {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_fragment_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_compute_shader;
        default:
            throw std::runtime_error("ShaderManager: unsupported stage");
    }
}

struct ShaderManager::Impl
{
    shaderc::Compiler c;
    shaderc::CompileOptions o;

    Impl()
    {
        o.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        o.SetTargetSpirv(shaderc_spirv_version_1_0);
        o.SetOptimizationLevel(shaderc_optimization_level_performance);
    }
};

ShaderManager::ShaderManager(VkDevice d)
    : device_(d)
    , impl_(std::make_unique<Impl>())
{
}

ShaderManager::~ShaderManager()
{
    for (auto& e : entries_)
        if (e.module)
            vkDestroyShaderModule(device_, e.module, nullptr);
}

ShaderHandle ShaderManager::compileFile(const std::string& p, VkShaderStageFlagBits s, const char* ep)
{
    for (size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].filePath == p && entries_[i].stage == s)
            return (ShaderHandle) i;
    ShaderHandle h = (ShaderHandle) entries_.size();
    Entry& e = entries_.emplace_back();
    e.name = std::filesystem::path(p).filename().string();
    e.filePath = p;
    e.stage = s;
    e.entryPoint = ep;
    e.fileTime = std::filesystem::last_write_time(p);
    compileEntry(e, readFile(p));
    reflectEntry(e);
    return h;
}

ShaderHandle ShaderManager::compileSource(const std::string& n, const std::string& src, VkShaderStageFlagBits s, const char* ep)
{
    ShaderHandle h = (ShaderHandle) entries_.size();
    Entry& e = entries_.emplace_back();
    e.name = n;
    e.stage = s;
    e.entryPoint = ep;
    compileEntry(e, src);
    reflectEntry(e);
    return h;
}

void ShaderManager::compileEntry(Entry& e, const std::string& src)
{
    auto r = impl_->c.CompileGlslToSpv(src, toKind(e.stage), e.name.c_str(), e.entryPoint.c_str(), impl_->o);
    if (r.GetCompilationStatus() != shaderc_compilation_status_success)
        throw std::runtime_error("ShaderManager: " + e.name + ":\n" + r.GetErrorMessage());
    e.spirv.assign(r.cbegin(), r.cend());
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = e.spirv.size() * sizeof(uint32_t);
    ci.pCode = e.spirv.data();
    vkCreateShaderModule(device_, &ci, nullptr, &e.module);
}

void ShaderManager::reflectEntry(Entry& e)
{
    e.reflectionData.stage = e.stage;
    e.reflectionData.entryPoint = e.entryPoint;
    spvc_context ctx = nullptr;
    spvc_context_create(&ctx);
    spvc_parsed_ir ir = nullptr;
    spvc_context_parse_spirv(ctx, e.spirv.data(), e.spirv.size(), &ir);
    spvc_compiler comp = nullptr;
    spvc_context_create_compiler(ctx, SPVC_BACKEND_NONE, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &comp);
    spvc_resources res = nullptr;
    spvc_compiler_create_shader_resources(comp, &res);
    auto pl = [&](spvc_resource_type t, VkDescriptorType vt) {
        const spvc_reflected_resource* list = nullptr;
        size_t cnt = 0;
        spvc_resources_get_resource_list_for_type(res, t, &list, &cnt);
        for (size_t i = 0; i < cnt; ++i)
        {
            ShaderDescriptorBinding b;
            b.type = vt;
            b.name = list[i].name ? list[i].name : "";
            b.binding = spvc_compiler_get_decoration(comp, list[i].id, SpvDecorationBinding);
            b.set = spvc_compiler_get_decoration(comp, list[i].id, SpvDecorationDescriptorSet);
            if (b.set > 1000)
                b.set = 0;
            e.reflectionData.bindings.push_back(b);
        }
    };
    pl(SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    pl(SPVC_RESOURCE_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pl(SPVC_RESOURCE_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    pl(SPVC_RESOURCE_TYPE_SEPARATE_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    pl(SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS, VK_DESCRIPTOR_TYPE_SAMPLER);
    pl(SPVC_RESOURCE_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    spvc_context_destroy(ctx);
}

VkShaderModule ShaderManager::getModule(ShaderHandle h) const
{
    if (h >= entries_.size())
        return VK_NULL_HANDLE;
    return entries_[h].module;
}

const ShaderReflection& ShaderManager::reflection(ShaderHandle h) const
{
    static ShaderReflection empty;
    if (h >= entries_.size())
        return empty;
    return entries_[h].reflectionData;
}

int ShaderManager::hotReload()
{
    int c = 0;
    for (auto& e : entries_)
    {
        if (e.filePath.empty())
            continue;
        auto t = std::filesystem::last_write_time(e.filePath);
        if (t == e.fileTime)
            continue;
        e.fileTime = t;
        try
        {
            std::string src = readFile(e.filePath);
            if (e.module)
                vkDestroyShaderModule(device_, e.module, nullptr);
            compileEntry(e, src);
            reflectEntry(e);
            ++c;
        }
        catch (const std::exception& ex)
        {
            fprintf(stderr, "[ShaderManager] hot-reload failed: %s\n", ex.what());
        }
    }
    return c;
}
} // namespace Renderer
} // namespace Engine
