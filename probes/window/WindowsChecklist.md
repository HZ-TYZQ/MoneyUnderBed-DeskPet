# Windows 11 探针检查表

对应 `docs/Plans/DevelopmentPlan.md` 第 6.3 节。

范围：Windows 11 x86-64，单显示器。
混合 DPI 多显示器本轮不测，也不推测结果。

结果只填 `通过`、`失败`、`未测试`，后面附一句实际观察。
不要从其他用例的结果推断本用例的结果。

## 准备

1. 解压 Actions 下载的 ZIP，不要挑单个文件复制出来。
2. 在解压目录打开 `cmd` 或 PowerShell。
3. 记录 ZIP 的 SHA-256：

   ```powershell
   Get-FileHash .\deskpet-probe.exe -Algorithm SHA256
   ```

4. 每个用例都会在当前目录生成 `deskpet-probe-<用例>.log`。失败时把整份日志留下。

先跑一遍冒烟，确认依赖齐全：

```powershell
.\deskpet-probe.exe --case build
echo $LASTEXITCODE   # 应为 0
.\deskpet-probe.exe --case screen
```

`--case build` 非 0 或弹出缺少 DLL 的对话框，说明打包不完整，先不要继续。

## 1. 渲染与动画

```powershell
.\deskpet-probe.exe --case render --duration 0
.\deskpet-probe.exe --case animate --duration 60
```

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 背景完全透明，没有白底或黑底方块 |  |  |
| 角色边缘没有半透明描边或杂色 |  |  |
| 2× 下像素锐利，没有插值模糊 |  |  |
| 动画 9 帧顺序正确、循环连续 |  |  |
| 日志中 `animation_summary` 的 `min_ms`／`max_ms` 接近 100 |  |  |

## 2. 自主移动

```powershell
.\deskpet-probe.exe --case move --duration 40
```

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 窗口确实横向往返移动约 300 px |  |  |
| 日志中 `motion_tick` 的 `requested` 与 `reported` 一致 |  |  |
| 移动过程没有残影或撕裂 |  |  |

## 3. 拖动

两条路径分别测。一条通过不代表另一条通过。

```powershell
.\deskpet-probe.exe --case drag-system --duration 0
.\deskpet-probe.exe --case drag-manual --duration 0
```

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| `drag-system`：日志 `system_drag_requested accepted=true` |  |  |
| `drag-system`：窗口跟随鼠标 |  |  |
| `drag-manual`：窗口跟随鼠标 |  |  |
| `drag-manual`：松手位置与鼠标位置一致，无跳变 |  |  |

## 4. 透明像素命中

```powershell
.\deskpet-probe.exe --case hittest --duration 0
```

角色窗口正下方会出现一个点击靶窗口。角色矩形范围内既有角色像素，也有透明像素。

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 点击角色身体，日志出现 `pet_click` 且 `sprite_alpha` 大于 0 |  |  |
| 点击角色矩形内的透明角落，点击靶计数增加，角色不响应 |  |  |
| 点击靶上的计数显示与日志 `target_click` 一致 |  |  |
| 掩码没有把角色本身裁掉一部分 |  |  |

## 5. 整窗穿透

两条路径分别测。状态每 4 秒切换一次，日志会打印当前是否穿透。

```powershell
.\deskpet-probe.exe --case passthrough-qt --duration 40
.\deskpet-probe.exe --case passthrough-native --duration 40
```

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| `passthrough-qt`：穿透开启后点击落到点击靶 |  |  |
| `passthrough-qt`：穿透关闭后角色重新接收点击 |  |  |
| `passthrough-qt`：日志 `rebuilt=false`（`true` 表示窗口被重建） |  |  |
| `passthrough-qt`：切换瞬间不闪烁 |  |  |
| `passthrough-qt`：切换后仍不在任务栏、仍然置顶 |  |  |
| `passthrough-native`：日志 `applied=true` |  |  |
| `passthrough-native`：穿透与恢复均生效 |  |  |
| `passthrough-native`：切换瞬间不闪烁，日志 `rebuilt` 保持 `false` |  |  |

若 `passthrough-qt` 出现 `rebuilt=true`、闪烁、丢置顶或短暂出现在任务栏，
说明窗口重建问题已复现，`passthrough-native` 的结果决定是否在产品中采用原生路径。

## 6. 置顶

```powershell
.\deskpet-probe.exe --case topmost --duration 40
```

打开一个终端和一个浏览器窗口，把它们轮流激活。

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 置顶开启时始终在终端之上 |  |  |
| 置顶开启时始终在浏览器之上 |  |  |
| 置顶关闭时会被其他窗口盖住 |  |  |
| 每次切换日志 `rebuilt=false` |  |  |
| 切换过程不闪烁、不改变位置 |  |  |

## 7. 焦点

```powershell
.\deskpet-probe.exe --case focus --duration 60
```

先打开记事本并开始输入，再运行本用例。

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 探针启动后记事本仍是前台窗口，光标仍在闪 |  |  |
| 启动后继续输入，字进入记事本 |  |  |
| 点击角色后记事本仍保持前台，输入不中断 |  |  |
| 拖动角色后记事本仍保持前台 |  |  |
| 日志 `focus_after_show` 和 `focus_after_click` 的 `active_window=false` |  |  |
| 日志 `foreground` 一直是记事本 |  |  |

## 8. 任务栏与 Alt+Tab

```powershell
.\deskpet-probe.exe --case windowlist --duration 60
```

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 任务栏没有探针的按钮 |  |  |
| `Alt+Tab` 列表没有探针 |  |  |
| `Win+Tab` 任务视图中的表现（记录实际情况即可） |  |  |
| 日志 `ex_bits` 含 `toolwindow`，不含 `appwindow` |  |  |

## 9. 缩放

在系统设置里逐档切换缩放，每档重新启动一次探针。

```powershell
.\deskpet-probe.exe --case dpi --duration 30
```

| 缩放 | 视觉大小 | 像素是否锐利 | 日志 `dpr` 与 `logical_dpi` |
| --- | --- | --- | --- |
| 100% |  |  |  |
| 125% |  |  |  |
| 150% |  |  |  |
| 200% |  |  |  |

另外用 `--scale 1` 和 `--scale 2` 各跑一次 `render`，记录两者的实际像素尺寸。

## 10. 生命周期

```powershell
.\deskpet-probe.exe --case lifecycle --duration 0
```

| 检查项 | 结果 | 观察 |
| --- | --- | --- |
| 从任务管理器结束进程后没有残留窗口 |  |  |
| 关闭窗口后进程退出，日志有 `window_close` 与 `exit` |  |  |
| 连续启动两次，两个实例互不干扰（探针没有单实例逻辑，这是预期） |  |  |
| 锁屏并解锁后窗口仍在原位、仍然置顶 |  |  |
| 睡眠并唤醒后窗口仍在原位、仍然置顶 |  |  |

## 失败证据

每条失败都要留下：

- 完整的 `deskpet-probe-<用例>.log`。
- 日志里的 `probe.event=environment` 行（含实际 Qt 版本与平台名）。
- 视觉问题的截图或短录屏。
- 该用例在另一档缩放下是否同样失败。
- Windows 版本号（`winver`）与显卡驱动信息。
