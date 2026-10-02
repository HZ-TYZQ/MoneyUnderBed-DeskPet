#!/usr/bin/env bash

# 归档 Qt 与 AppImage 工具链的对应源码（GPL 分发义务的一部分）。
# AppImage 捆绑的 Ubuntu 系统库源码由 PackageAppImage.sh 另行归档。
#
# 用法：ArchiveCorrespondingSource.sh <输出目录>
# 每个归档旁边写一个同名 .sha256；下载的归档按 pins.env 里的哈希核对。

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <output-directory>" >&2
    exit 2
fi

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=packaging/pins.env
source "$here/pins.env"

output="$(readlink -m "$1")"
mkdir -p "$output"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# 下载并按锁定哈希核对。
fetch()
{
    local name="$1" url="$2" sha256="$3"
    curl -fL --retry 3 -o "$output/$name" "$url"
    printf '%s  %s\n' "$sha256" "$name" > "$output/$name.sha256"
    (cd "$output" && sha256sum -c "$name.sha256")
}

# 按锁定提交导出 Git 仓库源码。
export_git()
{
    local name="$1" repository="$2" commit="$3"
    git clone --filter=blob:none "$repository" "$work/$name"
    git -C "$work/$name" checkout "$commit"
    test "$(git -C "$work/$name" rev-parse HEAD)" = "$commit"
    local archive="$name-$commit.tar.gz"
    git -C "$work/$name" archive --format=tar.gz --prefix="$name-$commit/" \
        -o "$output/$archive" HEAD
    (cd "$output" && sha256sum "$archive" > "$archive.sha256")
}

fetch "qtbase-everywhere-src-${QT_VERSION:?QT_VERSION must be set}.tar.xz" \
    "$QTBASE_SOURCE_URL" "$QTBASE_SOURCE_SHA256"
fetch icu4c-73_2-src.tgz "$ICU_SOURCE_URL" "$ICU_SOURCE_SHA256"
fetch fuse-3.15.0.tar.xz "$FUSE_SOURCE_URL" "$FUSE_SOURCE_SHA256"
fetch squashfuse-0.5.2.tar.gz "$SQUASHFUSE_SOURCE_URL" "$SQUASHFUSE_SOURCE_SHA256"

export_git type2-runtime https://github.com/AppImage/type2-runtime.git \
    "$APPIMAGE_RUNTIME_COMMIT"
export_git linuxdeploy https://github.com/linuxdeploy/linuxdeploy.git \
    "$LINUXDEPLOY_COMMIT"
export_git linuxdeploy-plugin-qt https://github.com/linuxdeploy/linuxdeploy-plugin-qt.git \
    "$LINUXDEPLOY_QT_COMMIT"

echo "Corresponding source archived in $output:"
ls -l "$output"
