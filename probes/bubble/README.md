# 气泡原型 A 审核工具

对应 `docs/Plans/DevelopmentPlan.md` 第 11.1 节。

目的只有一个：让项目所有者用**真实**素材看清楚原型 A 的实际观感，
然后冻结具体渲染参数并写回 `docs/Decisions.md`。参数冻结后正式气泡
在 `src/ui/` 中实现，本目录随即可以整个删除。

## 与 `probes/window/` 的区别

窗口探针刻意不链接产品代码，因为它要独立验证产品将要做的事。
本原型相反：它必须使用产品真实的台词数据、表情素材和字体，
所以链接 `MUB::Core` 与 `MUB::Resources`。复制一份台词只会引入漂移。

## 基线是已选定的 HTML 原型

布局与全部默认值**逐条取自** `Temp/dialogue-bubble-designs/prototype.css` 中
`body[data-layout="embedded"]` 的实际取值，不是重新设计的。
那份 HTML 是项目所有者已经选定的那一版，因此它是事实基线。

对应关系：

| CSS | 参数 | 默认值 |
| --- | --- | --- |
| `.dialogue-panel { width }` | `panelWidth` | `260`，**固定**，不随文字长短变化 |
| `.dialogue-panel { min-height }` | `panelMinHeight` | `78` |
| `.dialogue-panel { padding }` | `paddingTop/Right/Bottom` | `13 / 17 / 17` |
| `[embedded] .dialogue-panel { padding-left }` | `paddingLeft` | `72` |
| `.dialogue-panel { border-radius }` | `cornerRadius` | `1` |
| `[embedded] .dialogue-panel { background }` | `panelAlpha` | `rgb(12,11,14)` @ `219`（86%） |
| `[embedded] .dialogue-panel { border }` | `borderAlpha` | `41`（16%） |
| `.dialogue-portrait { width/height }` | `portraitWidth/Height` | `60 x 72` |
| `[embedded] .dialogue-portrait { left/bottom }` | `portraitLeft/Bottom` | `6 / 3` |
| `[embedded] .dialogue-panel::before` | `separatorLeft/Inset/Alpha` | `66 / 8 / 36`（14%） |
| `.dialogue-text { font-size }` | `fontPixelSize` | `12` |
| `.dialogue-text { line-height }` | `lineHeightPermille` | `1620`（1.62 倍） |
| `.dialogue-text { min-height }` | `textMinHeight` | `46` |
| `.page-cue` | `pageCue*` | 右下角一个 `□`，7px，70%，打字时降到 28% |
| Qt 原型审核冻结的 `right/bottom` | `offsetRight/Bottom` | `38 / 90` |
| `setInterval(..., 28)` | `typingMsPerChar` | `28` |

结构本身不提供改动入口，可调的只有这些数值。

三处与 HTML 原型不同，都是有意为之：

- **边缘避让**。HTML 原型跑在固定舞台上，没有这件事；`docs/Decisions.md`
  第 11.3 节要求不允许气泡或表情被裁出屏幕，因此新增 `screenMargin` 与
  `mirrorNearEdge`。具体避让方式在决策第 13 节仍未确定，交由本次审核决定。
- **角色动画**。HTML 用 CSS `steps(9)` 播 1.35 s，本原型改用产品的
  `AnimationClip` 登记值，以便看到与正式实现一致的节奏。
- **气泡不会自己消失**。原型把超时与自动消失都关掉了，免得审核过程中气泡跑掉。

## 运行

```bash
cmake --preset dev
cmake --build --preset dev
./build/dev/bin/mub-bubble-prototype
```

会打开两个窗口：

- **参数窗口**：所有可调数值、台词选择、倍率选择，以及一键导出。
- **桌面浮层**：无边框的角色与气泡，角色可拖动，气泡跟随。

## 需要在这个工具里确认的事

计划第 11.1 节列出的审核项：

- 实际像素字号、抗锯齿策略、行高。
- 面板固定宽度与文字区最小高度。参数窗口会显示当前页折行几行、面板能容纳几行，超出会提示该页需要人工再分页。
- 翻页提示样式。
- 表情切换方式：选一段多页对话，点击推进，看表情是否跟着换。
- 靠近屏幕左右与顶部边缘时的避让：把角色拖到各个边角再看气泡去了哪里。
- 拖动跟随：拖动角色时气泡是否稳定跟随。
- `1.5x` 的非均匀像素：倍率下拉里切到 `1.5x`，与 `1x`、`2x` 对比是否可接受。

Windows 上还要留意系统缩放的叠加效果：实际生效的是
`项目倍率 x 系统 DPR`，见 `docs/WindowsFeasibilityResults.md` 第 3 节。

## 导出

参数窗口底部的按钮把当前全部取值复制到剪贴板，格式已经可以直接贴回
`docs/Decisions.md`。**参数写回决策文档之前不实现正式气泡。**

## 无头渲染

用于在没有桌面的环境下检查布局，或者截图讨论：

```bash
QT_QPA_PLATFORM=offscreen ./build/dev/bin/mub-bubble-prototype \
    --render <输出目录> --dialogue icecream-drop
```

按默认参数把该段对话的每一页各渲染成一张 PNG，背景用中灰衬出半透明面板。
这条路径不能替代真实桌面审核：边缘避让、拖动跟随和倍率观感都看不出来。
