#!/usr/bin/env bash

# Collect provenance and notices for every non-Qt shared library copied from the
# Ubuntu runner into the AppImage.  The source-package list is consumed later in
# the same job to archive the exact Debian source packages alongside the binary.

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

mkdir -p "$license_dir"
printf 'file\tcomponent\tbinary-package\tbinary-version\tsource-package\tsource-version\tnotice\n' \
    > "$manifest"
: > "$source_list"

package_owner_of()
{
    local soname="$1"
    local installed
    installed="$(ldconfig -p | awk -v name="$soname" '$1 == name { print $NF; exit }')"
    if [[ -z "$installed" ]]; then
        return 1
    fi

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

while IFS= read -r deployed; do
    relative="${deployed#"$appdir/"}"
    soname="$(basename "$deployed")"

    if [[ "$soname" == libQt6*.so* ]]; then
        printf '%s\tQt 6.11.2\t-\t-\tqtbase\t6.11.2\tlicenses/qt-modules-6.11.2.spdx\n' \
            "$relative" >> "$manifest"
        continue
    fi

    # Qt's official Linux desktop archive carries ICU 73 beside Qt.  Ubuntu
    # 22.04 itself has an older SONAME, so these files deliberately do not map
    # to a dpkg package on the runner.
    if [[ "$soname" =~ ^libicu(data|i18n|uc)\.so\.73$ ]]; then
        printf '%s\tICU 73.2\t-\t-\ticu\t73.2\tlicenses/icu.txt\n' \
            "$relative" >> "$manifest"
        continue
    fi

    owner="$(package_owner_of "$soname" || true)"
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

    printf '%s\tUbuntu 22.04\t%s\t%s\t%s\t%s\tlicenses/linux-runtime/%s\n' \
        "$relative" "$binary_name" "$binary_version" "$source_name" \
        "$source_version" "$notice_name" >> "$manifest"
    printf '%s\t%s\n' "$source_name" "$source_version" >> "$source_list"
done < <(find "$appdir/usr/lib" -maxdepth 1 -type f -name '*.so*' | sort)

sort -u -o "$source_list" "$source_list"

if [[ ! -s "$manifest" || ! -s "$source_list" ]]; then
    echo "Runtime provenance collection produced an empty result" >&2
    exit 1
fi

echo "Collected $(($(wc -l < "$manifest") - 1)) runtime library records"
echo "Collected $(wc -l < "$source_list") Ubuntu source package records"
