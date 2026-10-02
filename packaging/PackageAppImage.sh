#!/usr/bin/env bash

# 把 Release 构建打成 AppImage，并归档它捆绑的 Ubuntu 系统库的对应源码。
#
# 只在 GitHub Actions 的 ubuntu-22.04 runner 上运行：会改写 apt 源列表，并依赖
# install-qt-action 设置的 QT_ROOT_DIR。
#
# 用法：PackageAppImage.sh <构建目录> <AppImage 文件名> <显示版本>
# 产出（当前目录）：<AppImage>、<AppImage>.sha256、linux-system-source/

set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 <build-directory> <appimage-file-name> <display-version>" >&2
    exit 2
fi

build_dir="$1"
image="$2"
display_version="$3"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=packaging/pins.env
source "$here/pins.env"

appdir="$PWD/AppDir"
tools="$PWD/tools"
app_id=io.github.hz_tyzq.MoneyUnderBedDeskPet

export APPIMAGE_EXTRACT_AND_RUN=1
# linuxdeploy 自带的旧 strip 不认识现代 Qt ELF 的 .relr.dyn。官方支持 NO_STRIP；
# Release 体积以可靠可运行为先。
export NO_STRIP=1

echo "::group::Install the unified AppDir layout"
cmake --install "$build_dir" --prefix "$appdir"
printf '%s\n' \
    "commit=${GITHUB_SHA:?GITHUB_SHA must be set}" \
    "version=$display_version" \
    "qt=${QT_VERSION:?QT_VERSION must be set}" \
    "platform=linux-x86_64" \
    "runner=ubuntu-22.04" > "$appdir/BUILD_INFO.txt"
cmake \
    -DPACKAGE_ROOT="$appdir" \
    -DPACKAGE_EXECUTABLE=usr/bin/money-under-bed-deskpet \
    -DPACKAGE_REQUIRED_FILES=licenses/appimage-runtime.txt \
    -P "$here/VerifyPackage.cmake"
echo "::endgroup::"

echo "::group::Download pinned AppImage tooling"
mkdir -p "$tools"
curl -fL --retry 3 -o "$tools/linuxdeploy-x86_64.AppImage" "$LINUXDEPLOY_URL"
curl -fL --retry 3 -o "$tools/linuxdeploy-plugin-qt-x86_64.AppImage" "$LINUXDEPLOY_QT_URL"
curl -fL --retry 3 -o "$tools/runtime-x86_64" "$APPIMAGE_RUNTIME_URL"
sha256sum -c - <<CHECKSUMS
$LINUXDEPLOY_SHA256  $tools/linuxdeploy-x86_64.AppImage
$LINUXDEPLOY_QT_SHA256  $tools/linuxdeploy-plugin-qt-x86_64.AppImage
$APPIMAGE_RUNTIME_SHA256  $tools/runtime-x86_64
CHECKSUMS
chmod +x "$tools"/*.AppImage
echo "::endgroup::"

echo "::group::Populate and prune the AppDir"
qmake_path="$(command -v qmake || command -v qmake6)"
EXTRA_PLATFORM_PLUGINS=libqoffscreen.so QMAKE="$qmake_path" \
    "$tools/linuxdeploy-x86_64.AppImage" \
        --appdir "$appdir" \
        --executable "$appdir/usr/bin/money-under-bed-deskpet" \
        --desktop-file "$appdir/usr/share/applications/$app_id.desktop" \
        --icon-file "$appdir/usr/share/icons/hicolor/160x160/apps/$app_id.png" \
        --plugin qt

# 产品只加载 PNG 素材（PNG 支持内置于 QtGui），不用 TLS；界面译文编在可执行文件
# 的资源里。删掉这些软插件和 Qt 自带翻译，缩小体积，也缩小第三方审计面。
# 下面对实际 AppImage 的 offscreen 自检证明剩下的素材路径仍然可用。
rm -rf "$appdir/usr/plugins/imageformats" "$appdir/usr/plugins/tls" \
    "$appdir/usr/translations"
rm -f "$appdir/usr/lib/libQt6Svg.so.6"

curl -fL --retry 3 -o "$appdir/licenses/icu.txt" "$ICU_LICENSE_URL"
echo "$ICU_LICENSE_SHA256  $appdir/licenses/icu.txt" | sha256sum -c -
mkdir -p "$appdir/licenses/appimage-runtime"
while IFS=$'\t' read -r name url checksum; do
    target="$appdir/licenses/appimage-runtime/$name"
    curl -fL --retry 3 -o "$target" "$url"
    echo "$checksum  $target" | sha256sum -c -
done <<LICENSES
libfuse-LGPL-2.1.txt	$LIBFUSE_LICENSE_URL	$LIBFUSE_LICENSE_SHA256
squashfuse.txt	$SQUASHFUSE_LICENSE_URL	$SQUASHFUSE_LICENSE_SHA256
musl.txt	$MUSL_LICENSE_URL	$MUSL_LICENSE_SHA256
zstd.txt	$ZSTD_LICENSE_URL	$ZSTD_LICENSE_SHA256
zlib.txt	$ZLIB_LICENSE_URL	$ZLIB_LICENSE_SHA256
LICENSES

"$here/CollectLinuxRuntime.sh" "$appdir" linux-source-packages.tsv
echo "::endgroup::"

echo "::group::Archive the exact Ubuntu source packages"
sudo sed -n 's/^deb /deb-src /p' /etc/apt/sources.list \
    | sudo tee /etc/apt/sources.list.d/mub-source.list >/dev/null
sudo apt-get update
mkdir -p linux-system-source
while IFS=$'\t' read -r package version; do
    "$here/DownloadUbuntuSource.sh" "$package" "$version" linux-system-source
done < linux-source-packages.tsv
cp linux-source-packages.tsv linux-system-source/SOURCE_PACKAGES.tsv
(
    cd linux-system-source
    find . -maxdepth 1 -type f ! -name SHA256SUMS -print0 \
        | sort -z | xargs -0 sha256sum > SHA256SUMS
)
test -s linux-system-source/SHA256SUMS
echo "::endgroup::"

echo "::group::Build the AppImage"
LDAI_OUTPUT="$image" LDAI_RUNTIME_FILE="$tools/runtime-x86_64" \
    "$tools/linuxdeploy-x86_64.AppImage" --appdir "$appdir" --output appimage
test -s "$image"
echo "::endgroup::"

echo "::group::Verify the actual AppImage"
QT_QPA_PLATFORM=offscreen "./$image" --self-test

extract_dir="$(mktemp -d)"
trap 'rm -rf "$extract_dir"' EXIT
image_path="$PWD/$image"
(cd "$extract_dir" && env -u APPIMAGE_EXTRACT_AND_RUN "$image_path" --appimage-extract >/dev/null)
required=(
    usr/plugins/platforms/libqxcb.so
    usr/plugins/platforms/libqoffscreen.so
    "usr/share/metainfo/$app_id.appdata.xml"
    licenses/appimage-runtime.txt
    licenses/appimage-tooling.txt
    licenses/icu.txt
    licenses/linux-runtime.tsv
    licenses/appimage-runtime/libfuse-LGPL-2.1.txt
    licenses/appimage-runtime/squashfuse.txt
    licenses/appimage-runtime/musl.txt
    licenses/appimage-runtime/zstd.txt
    licenses/appimage-runtime/zlib.txt
)
forbidden=(
    usr/plugins/imageformats
    usr/plugins/tls
    usr/translations
    usr/lib/libQt6Svg.so.6
)
cmake \
    -DPACKAGE_ROOT="$extract_dir/squashfs-root" \
    -DPACKAGE_EXECUTABLE=usr/bin/money-under-bed-deskpet \
    "-DPACKAGE_REQUIRED_FILES=$(IFS=';'; echo "${required[*]}")" \
    "-DPACKAGE_FORBIDDEN_PATHS=$(IFS=';'; echo "${forbidden[*]}")" \
    -P "$here/VerifyPackage.cmake"
echo "::endgroup::"

sha256sum "$image" > "$image.sha256"
cat "$image.sha256"
