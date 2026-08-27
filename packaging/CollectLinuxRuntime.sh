#!/usr/bin/env bash

# Collect provenance for every ELF in the AppDir.  The source-package list is
# consumed later in the same job to archive the exact Debian source packages
# alongside the binary.

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <AppDir> <source-package-list>" >&2
    exit 2
fi

appdir="$(readlink -f "$1")"
source_list="$(readlink -m "$2")"
license_dir="$appdir/licenses/linux-runtime"
manifest="$appdir/licenses/linux-runtime.tsv"

if [[ ! -d "$appdir/usr/lib" ]]; then
    echo "AppDir library directory does not exist: $appdir/usr/lib" >&2
    exit 2
fi
if [[ -z "${QT_ROOT_DIR:-}" || ! -d "$QT_ROOT_DIR" ]]; then
    echo "QT_ROOT_DIR must identify the Qt installation used for packaging" >&2
    exit 2
fi

mkdir -p "$license_dir"
printf 'file\tcomponent\tbinary-package\tbinary-version\tsource-package\tsource-version\tnotice\torigin\tdeployed-sha256\torigin-sha256\tbuild-id\n' \
    > "$manifest"
: > "$source_list"

installed_library_of()
{
    local soname="$1"
    ldconfig -p | awk -v name="$soname" '$1 == name { print $NF; exit }'
}

package_owner_of()
{
    local installed="$1"
    local canonical
    canonical="$(readlink -f "$installed")"
    local owner
    owner="$(dpkg-query -S "$canonical" 2>/dev/null | head -n 1 || true)"
    if [[ -z "$owner" ]]; then
        owner="$(dpkg-query -S "$installed" 2>/dev/null | head -n 1 || true)"
    fi
    if [[ -z "$owner" ]]; then
        return 1
    fi
    printf '%s' "${owner%%: /*}"
}

elf_build_id()
{
    readelf -n "$1" 2>/dev/null \
        | awk '/Build ID:/ { print $3; exit }'
}

verify_binary_identity()
{
    local deployed="$1"
    local origin="$2"
    local description="$3"

    if [[ ! -f "$origin" ]]; then
        echo "Provenance origin is missing for $description: $origin" >&2
        exit 1
    fi

    identity_deployed_sha="$(sha256sum "$deployed" | cut -d ' ' -f 1)"
    identity_origin_sha="$(sha256sum "$origin" | cut -d ' ' -f 1)"
    identity_build_id="$(elf_build_id "$deployed")"

    # linuxdeploy rewrites RPATH with patchelf, so the complete-file hashes are
    # normally different.  The linker-provided GNU Build ID is preserved by
    # that rewrite and identifies the original ELF.  Equal complete hashes are
    # also accepted for files that needed no rewrite.
    if [[ "$identity_deployed_sha" != "$identity_origin_sha" ]]; then
        local origin_build_id
        origin_build_id="$(elf_build_id "$origin")"
        if [[ -z "$identity_build_id" || "$identity_build_id" != "$origin_build_id" ]]; then
            echo "Bundled ELF does not match its claimed origin: $description" >&2
            echo "  deployed_sha256=$identity_deployed_sha build_id=$identity_build_id" >&2
            echo "  origin_sha256=$identity_origin_sha build_id=$origin_build_id" >&2
            exit 1
        fi
    fi
}

build_commit="$(sed -n 's/^commit=//p' "$appdir/BUILD_INFO.txt")"
build_version="$(sed -n 's/^version=//p' "$appdir/BUILD_INFO.txt")"
if [[ -z "$build_commit" || -z "$build_version" ]]; then
    echo "BUILD_INFO.txt does not contain commit and version" >&2
    exit 1
fi

while IFS= read -r deployed; do
    if ! readelf -h "$deployed" >/dev/null 2>&1; then
        continue
    fi

    relative="${deployed#"$appdir/"}"
    soname="$(basename "$deployed")"

    if [[ "$relative" == usr/bin/money-under-bed-deskpet ]]; then
        deployed_sha="$(sha256sum "$deployed" | cut -d ' ' -f 1)"
        build_id="$(elf_build_id "$deployed")"
        printf '%s\tMoneyUnderBed DeskPet\t-\t%s\trepository\t%s\tLICENSE\trepository:%s\t%s\t-\t%s\n' \
            "$relative" "$build_version" "$build_version" "$build_commit" \
            "$deployed_sha" "$build_id" >> "$manifest"
        continue
    fi

    if [[ "$soname" == libQt6*.so* ]]; then
        origin="$(readlink -f "$QT_ROOT_DIR/lib/$soname")"
        verify_binary_identity "$deployed" "$origin" "$relative"
        printf '%s\tQt 6.11.2\t-\t-\tqtbase\t6.11.2\tlicenses/qt-modules-6.11.2.spdx\tqt:lib/%s\t%s\t%s\t%s\n' \
            "$relative" "$soname" "$identity_deployed_sha" \
            "$identity_origin_sha" "$identity_build_id" >> "$manifest"
        continue
    fi

    if [[ "$relative" == usr/plugins/* ]]; then
        case "$relative" in
        usr/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so|\
        usr/plugins/platforminputcontexts/libibusplatforminputcontextplugin.so|\
        usr/plugins/platforms/libqoffscreen.so|\
        usr/plugins/platforms/libqxcb.so|\
        usr/plugins/platformthemes/libqxdgdesktopportal.so|\
        usr/plugins/xcbglintegrations/libqxcb-egl-integration.so|\
        usr/plugins/xcbglintegrations/libqxcb-glx-integration.so)
            ;;
        *)
            echo "Unreviewed Qt plugin in AppDir: $relative" >&2
            exit 1
            ;;
        esac
        plugin_path="${relative#usr/plugins/}"
        origin="$(readlink -f "$QT_ROOT_DIR/plugins/$plugin_path")"
        verify_binary_identity "$deployed" "$origin" "$relative"
        printf '%s\tQt Base plugin 6.11.2\t-\t-\tqtbase\t6.11.2\tlicenses/qt-modules-6.11.2.spdx\tqt:plugins/%s\t%s\t%s\t%s\n' \
            "$relative" "$plugin_path" "$identity_deployed_sha" \
            "$identity_origin_sha" "$identity_build_id" >> "$manifest"
        continue
    fi

    # Qt's official Linux desktop archive carries ICU 73 beside Qt.  Ubuntu
    # 22.04 itself has an older SONAME, so these files deliberately do not map
    # to a dpkg package on the runner.
    if [[ "$soname" =~ ^libicu(data|i18n|uc)\.so\.73$ ]]; then
        origin="$(readlink -f "$QT_ROOT_DIR/lib/$soname")"
        verify_binary_identity "$deployed" "$origin" "$relative"
        printf '%s\tICU 73.2\t-\t-\ticu\t73.2\tlicenses/icu.txt\tqt:lib/%s\t%s\t%s\t%s\n' \
            "$relative" "$soname" "$identity_deployed_sha" \
            "$identity_origin_sha" "$identity_build_id" >> "$manifest"
        continue
    fi

    if [[ "$relative" != usr/lib/* ]]; then
        echo "Unreviewed ELF location in AppDir: $relative" >&2
        exit 1
    fi

    installed="$(installed_library_of "$soname" || true)"
    if [[ -z "$installed" ]]; then
        echo "Could not find bundled library on the Ubuntu runner: $relative" >&2
        exit 1
    fi
    origin="$(readlink -f "$installed")"
    verify_binary_identity "$deployed" "$origin" "$relative"

    owner="$(package_owner_of "$installed" || true)"
    if [[ -z "$owner" ]]; then
        echo "Could not map bundled library to an Ubuntu package: $relative" >&2
        exit 1
    fi

    binary_name="$(dpkg-query -W -f='${binary:Package}' "$owner")"
    binary_version="$(dpkg-query -W -f='${Version}' "$owner")"
    source_name="$(dpkg-query -W -f='${source:Package}' "$owner")"
    source_version="$(dpkg-query -W -f='${source:Version}' "$owner")"
    [[ -n "$source_name" ]] || source_name="${binary_name%%:*}"
    [[ -n "$source_version" ]] || source_version="$binary_version"

    notice_source="/usr/share/doc/${binary_name%%:*}/copyright"
    if [[ ! -f "$notice_source" ]]; then
        echo "Copyright notice is missing for $binary_name ($relative)" >&2
        exit 1
    fi
    notice_name="${binary_name%%:*}.copyright"
    cp -L "$notice_source" "$license_dir/$notice_name"

    printf '%s\tUbuntu 22.04\t%s\t%s\t%s\t%s\tlicenses/linux-runtime/%s\tubuntu:%s\t%s\t%s\t%s\n' \
        "$relative" "$binary_name" "$binary_version" "$source_name" \
        "$source_version" "$notice_name" "$installed" "$identity_deployed_sha" \
        "$identity_origin_sha" "$identity_build_id" >> "$manifest"
    printf '%s\t%s\n' "$source_name" "$source_version" >> "$source_list"
done < <(find "$appdir" -type f -print | sort)

sort -u -o "$source_list" "$source_list"

if [[ ! -s "$manifest" || ! -s "$source_list" ]]; then
    echo "Runtime provenance collection produced an empty result" >&2
    exit 1
fi

echo "Collected $(($(wc -l < "$manifest") - 1)) ELF provenance records"
echo "Collected $(wc -l < "$source_list") Ubuntu source package records"
