cmake_minimum_required(VERSION 3.14)
project(LearnVulkan VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan REQUIRED)
include(FetchContent)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_Declare(glm  GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.3)
FetchContent_Declare(vk-bootstrap GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git GIT_TAG v1.4.352)
FetchContent_Declare(VulkanMemoryAllocator GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git GIT_TAG v3.3.0)
FetchContent_Declare(spirv-cross GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git GIT_TAG vulkan-sdk-1.3.296.0)
FetchContent_Declare(entt GIT_REPOSITORY https://github.com/skypjack/entt.git GIT_TAG v3.13.2)
FetchContent_Declare(cgltf GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git GIT_TAG v1.14)
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.8)
FetchContent_MakeAvailable(glfw glm vk-bootstrap VulkanMemoryAllocator spirv-cross entt cgltf stb imgui)

find_path(SHADERC_INCLUDE_DIR shaderc/shaderc.hpp HINTS "${Vulkan_INCLUDE_DIRS}" "${Vulkan_INCLUDE_DIRS}/.." PATH_SUFFIXES Include include)
get_filename_component(_vulkan_lib_dir "${Vulkan_LIBRARY}" DIRECTORY)
find_library(SHADERC_LIBRARY NAMES shaderc_shared shaderc shaderc_combined HINTS "${_vulkan_lib_dir}" PATH_SUFFIXES Lib lib)
if(SHADERC_INCLUDE_DIR AND SHADERC_LIBRARY)
    message(STATUS "Found shaderc: ${SHADERC_LIBRARY}")
    add_library(shaderc UNKNOWN IMPORTED)
    set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION "${SHADERC_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SHADERC_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "shaderc not found")
endif()

add_executable(LearnVulkan
    src/Engine/Runtime/main.cpp
    src/Engine/Platform/window.cpp
    src/Engine/Platform/input.cpp
    src/Engine/Platform/timer.cpp
    src/Engine/Renderer/Vulkan/vulkan_context.cpp
    src/Engine/Renderer/Vulkan/vulkan_device.cpp
    src/Engine/Renderer/Vulkan/vulkan_surface.cpp
    src/Engine/Renderer/Vulkan/vulkan_swapchain.cpp
    src/Engine/Renderer/Vulkan/deferred_renderer.cpp
    src/Engine/Renderer/RenderGraph/render_graph.cpp
    src/Engine/Renderer/Resource/gpu_allocator.cpp
    src/Engine/Renderer/Resource/gpu_buffer.cpp
    src/Engine/Renderer/Resource/gpu_texture.cpp
    src/Engine/Renderer/Resource/gpu_sampler.cpp
    src/Engine/Renderer/Shader/shader_manager.cpp
    src/Engine/Renderer/Pipeline/pipeline_cache.cpp
    src/Engine/ECS/world.cpp
    src/Engine/Scene/scene.cpp
    src/Engine/Asset/asset_loader.cpp
    src/Engine/Editor/editor.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_link_libraries(LearnVulkan PRIVATE
    glfw glm Vulkan::Vulkan vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator
    shaderc spirv-cross-core spirv-cross-c EnTT::EnTT
)

target_include_directories(LearnVulkan PRIVATE
    src
    ${cgltf_SOURCE_DIR} ${stb_SOURCE_DIR} ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(LearnVulkan PUBLIC SHADERS_DIR="${CMAKE_SOURCE_DIR}/shaders")
