# Color Sync 0.6.0

Photoshop 相似图层配色与渐变同步。安装和使用参见[项目主页](../../README.md)。

`ColorSync.psjs` 包含配色算法、图层操作和紧凑双栏界面；`ColorSync.jsx` 注册滤镜菜单；`Launcher.cs` 通过 COM 将菜单调用转到实际脚本，连接已有 Photoshop 实例。

在 Windows PowerShell 中构建辅助程序：

```powershell
& "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /platform:x64 /reference:System.Windows.Forms.dll /reference:System.Web.Extensions.dll /out:ColorSyncLauncher.exe Launcher.cs
```

将生成的 EXE、两个脚本和 Install.ps1 放在同一个文件夹，即可按主页步骤安装。

配色算法对平坦色样本建立颜色分组，拟合二维渐变场，应用平滑的颜色差值；不是逐像素覆盖。0.4.1 仅调整界面，配色算法保持原效果。

本机 PS 26.8 已验证菜单调用、辅助进程退出、临时文档双目标输出与算法结果一致、参考层及透明度不变。0.4.1 的实际 PS 布局验证确认每行 25 像素、列表 392 像素，底部按钮完整可见。


## 0.5.0 蒙版支持

普通像素蒙版在临时副本上应用，原图层和输出副本的可编辑蒙版保留；只将计算后的 RGB 回写，原始 Alpha 不变。完全隐藏的像素保持原样，停用蒙版按未应用处理。矢量蒙版与图层效果仍不支持。

运行 node build-mask-test.js 生成 release/mask-validation.psjs，通过 Photoshop 执行该脚本可在独立临时文档中验证：带蒙版参考层、柔边/未羽化/停用蒙版目标、无蒙版目标、蒙版像素与属性保持、原始 Alpha 保持、取消回滚和完全遮挡回滚。报告输出 release/mask-validation.json；测试不会写入已有 PSD。


## 0.6.0 单次点击内细化

一次点击最多执行三轮原有颜色场修正，像素不再变化时提前结束。参考分析复用，后续轮次在内存中重新拟合目标；只创建一个结果层，保留同一个撤销步骤。每轮仍保留 24/通道的修正上限及黑线、白色、低饱和保护，累计变化可能更大；不能保证任意图像一次完全一致。单次计算量可能增加，但省去重复读取、写回和手工操作。蒙版只应用一次用于读取，输出继续保留原 Alpha 和可编辑蒙版。
