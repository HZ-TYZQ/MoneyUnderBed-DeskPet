# `v@VERSION@` 的对应源码

本 Release **不含任何可执行程序**，只存放 [`v@VERSION@`](https://github.com/HZ-TYZQ/MoneyUnderBed-DeskPet/releases/tag/v@VERSION@)
所要求的对应源码（Corresponding Source）。程序本体请到 `v@VERSION@` 下载。

拆分只是为了让正式 Release 的资产列表可读。这些归档是 GPL 分发义务的一部分，
与 `v@VERSION@` 的二进制**同生共死**：只要 `v@VERSION@` 仍然公开，本 Release 就必须
保持公开且内容不变。

## 内容

- Qt Base 源码归档及其 `.sha256`。正式候选按 `GPL-3.0-only` 路径动态链接 Qt。
- ICU 源码归档。Qt 官方 Linux 归档随附的 ICU 版本比 Ubuntu 22.04 自带的更新。
- AppImage runtime 与打包工具源码：type2-runtime、linuxdeploy、
  linuxdeploy-plugin-qt、libfuse、squashfuse。
- AppImage 随包 Ubuntu 系统库的源码包（`.dsc`／`.orig`／`.debian`／`.diff`），
  外加 `SHA256SUMS` 与 `SOURCE_PACKAGES.tsv`。

## 这些源码与二进制的对应关系

不是「随便抓一版同名源码」。打包流水线用 `packaging/CollectLinuxRuntime.sh` 遍历
AppDir 里的每一个 ELF，逐个比对部署副本与来源文件的 SHA-256（linuxdeploy 用
patchelf 改写 RPATH 后完整哈希会变，此时改比对 GNU Build ID），再经 `dpkg-query`
映射到确切的源码包与版本，最后按该清单下载源码。任何一步对不上，构建直接失败。

对应关系记录在二进制包内的 `licenses/linux-runtime.tsv`（含每个文件的
`deployed-sha256`、`origin-sha256` 与 `build-id`），以及本 Release 的
`SOURCE_PACKAGES.tsv`。

## 身份

- 标签：`@SOURCES_TAG@`，指向与 `v@VERSION@` 相同的提交 `@COMMIT@`
- 全部归档产自 `v@VERSION@` 标签的同一次 Actions 流水线（run `@RUN_ID@`），
  不是事后另行收集
- Windows 便携包动态链接的是随包 app-local 部署的 MSVC 运行库，按其
  redistributable 条款分发，不属于 GPL 对应源码范围；许可说明在包内
  `licenses/msvc-runtime.md`
