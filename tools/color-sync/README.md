# Color Sync 0.4.1

Photoshop 相似图层配色与渐变同步。安装和使用参见[项目主页](../../README.md)。

`ColorSync.psjs` 包含配色算法、图层操作和紧凑双栏界面；`ColorSync.jsx` 注册滤镜菜单；`Launcher.cs` 通过 COM 将菜单调用转到实际脚本，连接已有 Photoshop 实例。

在 Windows PowerShell 中构建辅助程序：

```powershell
& "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /platform:x64 /reference:System.Windows.Forms.dll /reference:System.Web.Extensions.dll /out:ColorSyncLauncher.exe Launcher.cs
```

将生成的 EXE、两个脚本和 Install.ps1 放在同一个文件夹，即可按主页步骤安装。

配色算法对平坦色样本建立颜色分组，拟合二维渐变场，应用平滑的颜色差值；不是逐像素覆盖。0.4.1 仅调整界面，配色算法保持原效果。

本机 PS 26.8 已验证菜单调用、辅助进程退出、临时文档双目标输出与算法结果一致、参考层及透明度不变。0.4.1 的实际 PS 布局验证确认每行 25 像素、列表 392 像素，底部按钮完整可见。
