#!/usr/bin/env bash

# Download the exact Ubuntu source package that produced a bundled binary.
#
# A GitHub runner image and the live Ubuntu mirror are updated independently.
# During that window the runner can still contain version N while the mirror
# has already removed N after publishing N+1.  Prefer authenticated APT data;
# if that exact version is gone, retrieve the same publication from Ubuntu's
# official Launchpad archive and verify every payload against its .dsc hashes.

set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 <source-package> <source-version> <destination>" >&2
    exit 2
fi

package="$1"
version="$2"
destination="$(readlink -m "$3")"

if [[ ! "$package" =~ ^[a-z0-9][a-z0-9+.-]*$ ]]; then
    echo "invalid Ubuntu source package name: $package" >&2
    exit 2
fi
if [[ -z "$version" || "$version" == *$'\n'* || "$version" == *$'\t'* ]]; then
    echo "invalid Ubuntu source package version" >&2
    exit 2
fi

mkdir -p "$destination"
temporary="$(mktemp -d)"
cleanup()
{
    rm -rf "$temporary"
}
trap cleanup EXIT

if command -v apt-get >/dev/null 2>&1 \
    && (cd "$temporary" && apt-get source --download-only "$package=$version"); then
    echo "Downloaded $package $version from the configured APT sources"
else
    echo "APT no longer provides $package $version; trying Ubuntu Launchpad" >&2

    api='https://api.launchpad.net/1.0/ubuntu/+archive/primary'
    publications="$temporary/publications.json"
    curl -fsSL --retry 3 --get "$api" \
        --data-urlencode 'ws.op=getPublishedSources' \
        --data-urlencode "source_name=$package" \
        --data-urlencode "distro_series=https://api.launchpad.net/1.0/ubuntu/jammy" \
        --data-urlencode 'exact_match=true' \
        --data-urlencode 'order_by_date=true' \
        -o "$publications"

    publication="$(python3 -c '
import json
import sys

version = sys.argv[1]
with open(sys.argv[2], encoding="utf-8") as stream:
    entries = json.load(stream)["entries"]
print(next((entry["self_link"] for entry in entries
            if entry["source_package_version"] == version), ""))
' "$version" "$publications")"
    if [[ -z "$publication" ]]; then
        echo "Ubuntu Launchpad has no publication for $package $version" >&2
        exit 1
    fi

    urls="$temporary/source-urls.json"
    curl -fsSL --retry 3 "${publication}?ws.op=sourceFileUrls" -o "$urls"
    mapfile -t source_urls < <(python3 -c '
import json
import sys

with open(sys.argv[1], encoding="utf-8") as stream:
    print("\n".join(json.load(stream)))
' "$urls")
    if [[ ${#source_urls[@]} -eq 0 ]]; then
        echo "Ubuntu Launchpad returned no files for $package $version" >&2
        exit 1
    fi

    for url in "${source_urls[@]}"; do
        filename="$(basename "${url%%\?*}")"
        if [[ -z "$filename" || "$filename" == . || "$filename" == .. ]]; then
            echo "invalid source file URL returned by Launchpad: $url" >&2
            exit 1
        fi
        curl -fsSL --retry 3 -o "$temporary/$filename" "$url"
    done

    mapfile -t dsc_files < <(find "$temporary" -maxdepth 1 -type f -name '*.dsc' -print)
    if [[ ${#dsc_files[@]} -ne 1 ]]; then
        echo "expected one .dsc for $package $version, found ${#dsc_files[@]}" >&2
        exit 1
    fi

    dsc_source="$(sed -n 's/^Source: //p' "${dsc_files[0]}")"
    dsc_version="$(sed -n 's/^Version: //p' "${dsc_files[0]}")"
    if [[ "$dsc_source" != "$package" || "$dsc_version" != "$version" ]]; then
        echo "Launchpad .dsc identity mismatch: expected $package $version," \
            "got $dsc_source $dsc_version" >&2
        exit 1
    fi

    checksums="$temporary/dsc-sha256sums"
    awk '
        $0 == "Checksums-Sha256:" { inside = 1; next }
        inside && $0 !~ /^ / { exit }
        inside { print $1 "  " $3 }
    ' "${dsc_files[0]}" > "$checksums"
    if [[ ! -s "$checksums" ]]; then
        echo "${dsc_files[0]} has no Checksums-Sha256 entries" >&2
        exit 1
    fi
    (cd "$temporary" && sha256sum -c "$(basename "$checksums")")
    echo "Downloaded and verified $package $version from Ubuntu Launchpad"
fi

find "$temporary" -maxdepth 1 -type f \
    ! -name publications.json \
    ! -name source-urls.json \
    ! -name dsc-sha256sums \
    -exec cp -t "$destination" -- {} +
