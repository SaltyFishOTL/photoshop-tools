# Ink Cleanup — Photoshop 全颜色碎点清理

Windows x64 原生 Photoshop 滤镜。自动识别不同颜色的孤立碎点，用周边颜色修补；保留已确认效果的 0.7 清理算法，0.9 将计算移植为 C++ 并缓存预览结果。

## 下载与安装

从 [Releases](https://github.com/SaltyFishOTL/photoshop-tools/releases/latest) 下载 ZIP 并解压。

1. 保存工作并关闭 Photoshop。
2. 在 Photoshop 安装目录的 Plug-ins 文件夹中新建 InkCleanup 文件夹，放入 InkCleanup.8bf。更新时先把旧文件备份到 Plug-ins 目录之外，避免重复加载。
3. 启动 Photoshop，打开 RGB、8 位/通道的图片。
4. 选择 **滤镜 → Ink Cleanup → All Color Cleanup…**。

当前版本面向 Photoshop 2025 / 26.8，Windows x64；不支持 macOS。其他 Photoshop 版本尚未验证。

## 使用

- **识别强度**：默认 60；提高后可识别更弱的颜色差异，也可能去掉更多画面细节。
- **最大面积**：默认 60 像素；限制候选碎点的面积。
- 在预览中拖动画面，使用 100% 或更高倍率检查细节，切换清理前后比较。
- 对当前图层处理；有选区时只写入选区内。保留透明度。
- 确定后使用 Photoshop 的撤销操作回退。滤镜实现了参数记录与重放接口；具体智能滤镜宿主行为仍需在目标 PS 环境验证。

自动清理无法理解纹身、装饰纹样与杂点的语义区别，建议保留原图层。

## 0.9 性能与验证

本机相同 1024 × 1024 测试图，独立计算程序耗时从 0.8 的约 33.7 秒降至 0.9 的约 0.51 秒，约 66 倍；结果逐字节一致。此数字包含程序启动和文件读写，不代表 Photoshop 内的完整操作耗时。

另外通过 24 组确定性测试，覆盖颜色、渐变、弱线条、透明度、图像边缘及参数范围，并验证分块缓存、取消和宿主接口。测试用用户原图不随仓库分发。

## 从源码构建

需要 Windows、Visual Studio 2022 C++ 桌面开发工具及 Windows SDK、CMake 3.20+、Node.js。

单独取得 [Adobe Photoshop C++ SDK](https://github.com/AdobeDocs/photoshop-cpp-sdk)，阅读并遵守其许可。SDK 与从其样例生成的资源不包含在本仓库中。

在 VS 开发者命令提示符执行，将 SDK 路径替换为实际位置：

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DPHOTOSHOP_SDK_ROOT="D:/SDK/photoshop-cpp-sdk"
cmake --build build --config Release --parallel 4
node verify-cpp-cases.js
build\Release\cache_test.exe
```

输出滤镜：build/Release/InkCleanup.8bf。构建时 generate.js 从本地 SDK 样例生成 PiPLs.json；不要将生成文件或 SDK 提交到仓库。

engine-native.cpp 是实际清理引擎，auto-engine-0.7.js 仅作为测试参考，不在滤镜运行时执行。filter.cpp / filter.rc 实现原生界面与 Photoshop 接口，preview-cache.h 实现分块缓存。

## 第三方说明

参见 [THIRD_PARTY.md](THIRD_PARTY.md)。仓库公开不代表 Adobe SDK 随之获得不同的再分发许可。
