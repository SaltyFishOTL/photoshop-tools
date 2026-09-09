# Photoshop Tools · PS 工具箱

用于插画清理与角色多图层配色统一的 Photoshop 工具集合，后续 PS 工具集中维护在这里。

**已验证环境：Windows x64、Photoshop 2025 / 26.8、RGB 8 位/通道。其他版本未验证，不支持 macOS。**

| 工具 | 版本 | 用途 | 菜单入口 |
|---|---|---|---|
| Ink Cleanup | 0.9.0 | 自动识别所有颜色的孤立碎点，用周边颜色修补 | 滤镜 → Ink Cleanup → All Color Cleanup… |
| Color Sync | 0.4.1 | 选择参考层，将相似角色图层的配色及缓慢渐变向参考层同步 | 滤镜 → Color Sync… |

## 下载与安装

从 [最新发布](https://github.com/SaltyFishOTL/photoshop-tools/releases/latest) 下载 `Photoshop-Tools-v1.0.0.zip` 并解压。可以分别安装所需工具。安装前保存工作并关闭 Photoshop；安装完成后启动 PS。

### Ink Cleanup

1. 将 `InkCleanup/InkCleanup.8bf` 复制到 Photoshop 安装目录的 `Plug-ins/InkCleanup/` 中。
2. 默认完整路径为 `C:\Program Files\Adobe\Adobe Photoshop 2025\Plug-ins\InkCleanup\InkCleanup.8bf`。
3. 更新时覆盖同一路径；不要在其他插件目录留下旧副本。
4. 启动 PS，从上述滤镜菜单打开。识别强度、最大面积默认均为 **60**。

支持局部放大预览、当前图层处理及 PS 撤销。有选区时只写入选区内。自动识别无法区分所有装饰细节与杂点，建议在副本上检查效果。

### Color Sync

1. 解压后，在 `ColorSync` 文件夹中打开**管理员 PowerShell**。
2. 运行 `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1`。
3. 安装器默认定位 Photoshop 2025。若安装位置不同，先将 `Install.ps1` 的 `$scripts` 改为实际 `Presets\Scripts` 路径。
4. 启动 PS，打开 PSD，从 **滤镜 → Color Sync…** 进入。
5. 左侧选一个参考层，右侧选择多个同步层，点击“同步配色”。两侧都支持搜索，每侧约显示 15 行。

输出为新图层副本，保留原图层，整个操作可通过 PS 历史记录撤销。匹配配色与渐变，保留目标图层自身的线条、形状及透明度；不会复制参考层画面。适合构图和色彩区域相近的图层，不保证任意图片颜色完全一致。

支持普通像素层的不同不透明度、填充和混合模式；暂不支持背景层、智能对象、剪贴关系、蒙版及图层效果。必要时先准备普通像素副本。

Color Sync 是注册到滤镜菜单的多图层命令，**不是 .8bf 或智能滤镜**。只有 `ColorSync.jsx` 放在 PS 脚本目录，辅助程序与实际脚本放在 `%APPDATA%\ColorSync`。辅助程序连接已有 PS，完成后退出，不联网。请勿将辅助文件放进 PS 插件扫描目录。

## 更新、卸载与排错

- Color Sync 安装器会备份旧安装文件；仅更新配色脚本时重开工具窗口即可，首次安装菜单需要重启 PS。
- 卸载 Ink Cleanup：关闭 PS 后删除安装目录中的 `InkCleanup.8bf`。
- 卸载 Color Sync：关闭 PS 后删除 `Presets\Scripts\ColorSync.jsx` 和 `%APPDATA%\ColorSync`。
- 菜单缺失：检查安装路径、是否重启以及有无重复旧副本。
- Color Sync 启动失败：检查三个安装文件是否齐全；日志位于 `%LOCALAPPDATA%\ColorSync\last-run.json`。提交问题时请移除日志中的个人路径信息。

## 源码

- [Ink Cleanup 源码与构建说明](tools/ink-cleanup/README.md)
- [Color Sync 源码与构建说明](tools/color-sync/README.md)

发布包提供可安装成品。仓库不包含用户 PSD、角色图片、测试原图、Adobe SDK 或生成的 SDK 资源。Adobe SDK 的许可说明见 [THIRD_PARTY.md](tools/ink-cleanup/THIRD_PARTY.md)。
