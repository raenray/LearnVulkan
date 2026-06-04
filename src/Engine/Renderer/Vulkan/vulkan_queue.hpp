#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
namespace Engine { namespace Renderer {
struct VulkanQueue { VkQueue handle = VK_NULL_HANDLE; uint32_t family = 0; };
} }
