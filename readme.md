# LearnVulkan

基于 Vulkan 的实时 3D 渲染引擎，用于学习现代图形 API。支持延迟渲染管线、PBR 光照、Dear ImGui 编辑器界面和 GLTF 模型加载。

## 系统要求

- **操作系统**：Linux（X11/Wayland）
- **GPU**：支持 Vulkan 1.0+
- **编译器**：支持 C++20（GCC 11+ / Clang 14+）
- **CMake**：3.14+

## 依赖安装

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

> 其余依赖（glfw、glm、vk-bootstrap、VMA、spirv-cross、entt、cgltf、stb、imgui）均通过 CMake `FetchContent` 自动下载，无需手动安装。

## 构建

```shell
# 1. 编译着色器（GLSL → SPIR-V）
cd shaders
./compile.sh          # 生成 *.spv 文件

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

## 操作说明

| 操作 | 按键 |
|------|------|
| 移动摄影机 | W / A / S / D |
| 升降摄影机 | Q / E |
| 旋转视角 | 按住鼠标右键拖拽 |
| 开关编辑器界面 | 菜单栏 `View → Show Editor` |
| 开关摄影机控制 | 编辑器 `Camera Control` 复选框 |

## 项目结构

```
LearnVulkan/
├── shaders/                    # GLSL 着色器源码
│   ├── deferred_geometry.*     # G-buffer 几何通道
│   ├── deferred_lighting.*     # 延迟光照通道
│   └── compile.sh              # 编译脚本 (glslc → .spv)
├── src/Engine/
│   ├── Platform/               # 窗口、输入、计时器
│   ├── Renderer/
│   │   ├── Vulkan/             # 上下文、设备、交换链、延迟渲染器
│   │   ├── Resource/           # GPU 内存分配、缓冲区、纹理、采样器
│   │   ├── Shader/             # 着色器编译、热重载、反射
│   │   ├── Pipeline/           # 管线缓存、计算管线
│   │   └── RenderGraph/        # 帧图资源管理 (WIP)
│   ├── ECS/                    # 实体组件系统
│   ├── Scene/                  # 场景图与场景管理
│   ├── Asset/                  # GLTF 模型 / 纹理 / 着色器加载
│   ├── Editor/                 # Dear ImGui 编辑器界面
│   └── Runtime/                # 入口点 main.cpp
├── .clang-format               # 代码风格配置
└── readme.md
```

## 渲染特性

- **延迟着色**：G-buffer（位置、法线、反照率、材质参数）+ 深度
- **PBR 光照**：方向光源，支持 metallic / roughness 参数
- **交换链**：FIFO 呈现模式，BGRA8 sRGB 色彩空间
- **push constant** 传递 MVP 矩阵和光照参数
- **ImGui 编辑器**：层级面板、检视面板、视口统计、控制台日志

## 验证层

默认启用 Vulkan 验证层。如需关闭，修改 `src/Engine/Runtime/main.cpp`：

```cpp
VulkanContextConfig cfg{};
cfg.enableValidation = false;  // 关闭验证层
```

## 许可

MIT
