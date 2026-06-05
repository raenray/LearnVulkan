# LearnVulkan

基于 Vulkan 的实时 3D 渲染引擎，用于学习现代图形 API。支持延迟渲染管线、PBR 光照、Dear ImGui 编辑器界面和 GLTF 模型加载。

## 系统要求

- **操作系统**：Windows 10+ / Linux（X11/Wayland）
- **GPU**：支持 Vulkan 1.0+ 的显卡
- **编译器**：支持 C++20（MSVC 2022+ / GCC 11+ / Clang 14+）
- **CMake**：3.14+
- **Vulkan SDK**：[vulkan.lunarg.com](https://vulkan.lunarg.com/)

## 依赖安装

### Windows

安装 [Vulkan SDK](https://vulkan.lunarg.com/) 后无需额外依赖——GLFW、glm、vk-bootstrap、VMA、spirv-cross、entt、cgltf、stb、imgui 全部通过 CMake `FetchContent` 自动下载。

### Ubuntu / Debian

```shell
# Vulkan SDK
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools

# GLFW 系统库
sudo apt install libxkbcommon-dev libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev mesa-libgl-devel wayland-devel

# GLSL → SPIR-V 编译器
sudo apt install glslc
```

### Fedora

```shell
sudo dnf install vulkan-tools vulkan-loader-devel vulkan-validation-layers
sudo dnf install libxkbcommon-devel libX11-devel libXrandr-devel libXinerama-devel \
                 libXcursor-devel libXi-devel mesa-libGL-devel wayland-devel \
                 wayland-scanner glslc
```

## 构建

```shell
# 1. 编译着色器（GLSL → SPIR-V）
cd shaders
./compile.sh          # 或 compile.bat（Windows）

# 2. CMake 构建
cd ..
mkdir build && cd build
cmake ..
cmake --build .       # 或 make / ninja
```

## 运行

```shell
./build/LearnVulkan    # 单文件可执行，无外部资源依赖
```

启动时窗口自动使用 80% 屏幕尺寸。分辨率会持久化到 `settings.cfg`，下次启动时自动恢复。

## 操作说明

| 操作 | 按键 |
| ------ | ------ |
| 进入自由摄影机 | Viewport 面板点击「Start Free Camera」 |
| 移动 | W / A / S / D |
| 升降 | Q / E |
| 旋转视角 | 鼠标移动（摄影机模式下自动捕获） |
| 重置位置 | R |
| 退出自由摄影机 | ESC |
| 关闭程序 | 菜单栏 `File → Exit` 或关闭窗口 |

## 编辑器

- **Viewport**：显示当前分辨率、FPS、帧耗时、draw call 数；提供 80%/60% 预设按钮和自定义宽高 + Apply 按钮调节窗口大小
- **Inspector**：查看摄影机位置 / 帧信息，调节材质参数（Albedo、Metallic、Roughness）
- **Hierarchy**：显示场景实体列表（Camera、Light、Sphere）
- **Console**：运行时日志输出
- **View 菜单**：开关面板、调节字体缩放（100%–300%）

## 游戏逻辑入口

游戏代码写在 `src/Engine/Game/game.cpp`：

```cpp
void Game::init(Scene::Scene& scene, Renderer::MeshCache& meshCache) {
    // 创建摄影机、灯光、实体，设置 ECS 组件
    auto* node = scene.createNode("Player");
    scene.world().setComponent<ECS::MeshComponent>(node->entity(),
        {/* .meshId = "myMesh", … */});
}

void Game::update(float dt, GLFWwindow* window) {
    // 每帧逻辑：移动、旋转、输入处理
}
```

添加新模型：在 `App::init()`（`main.cpp`）中调用 `meshCache_.createMesh("id", vertices, indices, ...)` 上传 GPU 网格，然后在 `Game::init()` 中创建带 `MeshComponent{.meshId = "id"}` 的实体。

## 项目结构

```shell
LearnVulkan/
├── assets/
│   └── fonts/                  # 字体文件
├── shaders/                    # GLSL 着色器源码
│   ├── gbuffer.vert/frag       # G-buffer 几何通道
│   ├── lighting.vert/frag      # 延迟光照通道
│   └── compile.sh/bat          # 编译脚本 (glslc → .spv)
├── src/Engine/
│   ├── Platform/               # 窗口、输入、计时器
│   ├── Renderer/
│   │   ├── Vulkan/             # 上下文、设备、交换链、延迟渲染器
│   │   ├── Resource/           # GPU 内存分配、缓冲区、纹理、采样器
│   │   ├── Mesh/               # GPU 网格 + 网格缓存 + 场景渲染器
│   │   ├── Shader/             # 着色器编译、热重载、反射
│   │   ├── Pipeline/           # 管线缓存、计算管线
│   │   └── RenderGraph/        # 帧图资源管理 (WIP)
│   ├── ECS/                    # 实体组件系统
│   ├── Scene/                  # 场景图与场景管理
│   ├── Game/                   # 游戏逻辑入口 (Game::init / Game::update)
│   ├── Asset/                  # GLTF 模型 / 纹理 / 着色器加载
│   ├── Editor/                 # Dear ImGui 编辑器界面
│   └── Runtime/                # 入口点 main.cpp（App 类、引擎运行时）
├── .clang-format               # 代码风格配置（Allman 风格）
└── readme.md
```

## 渲染特性

- **延迟着色**：G-buffer（世界空间位置、法线、反照率、材质参数）+ 深度
- **PBR 光照**：Cook-Torrance BRDF，方向光源支持 metallic / roughness
- **ECS 驱动渲染**：SceneRenderer 遍历 ECS，根据 MeshComponent + TransformComponent 自动发出 draw call
- **交换链**：FIFO 呈现模式，BGRA8 sRGB 色彩空间
- **push constant** 传递 MVP 矩阵和光照参数
- **Maple Mono NF CN** 等宽中文字体，覆盖简繁中文 + 日韩约 3 万字符
- **ImGui 编辑器**：层级面板、检视面板、视口（含 FPS 和分辨率控制）、控制台、DPI 感知

## 验证层

默认启用 Vulkan 验证层。如需关闭，修改 `src/Engine/Renderer/Vulkan/vulkan_context.hpp`：

```cpp
struct VulkanContextConfig {
    bool enableValidation = false;  // 关闭验证层
    // …
};
```

## 许可

MIT
