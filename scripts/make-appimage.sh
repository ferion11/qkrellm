#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds, tests and packages qkrellm as an x86_64 AppImage with linuxdeploy (+ Qt plugin).
# Used by .github/workflows/appimage.yml; runs the same way locally. Everything lands under
# ${QKRELLM_OUT:-/tmp/qkrellm}: build/appimage, appimage/{tools,AppDir,out}.
#
#   scripts/make-appimage.sh            # -> /tmp/qkrellm/appimage/out/qkrellm-<ver>-x86_64.AppImage
#
# Environment:
#   QMAKE                  qmake of the Qt to bundle (default: qmake6 or qmake on PATH)
#   QK_APPIMAGE_VERSION    version in the file name (default: <project version>-g<short sha>)
#   QK_UPDATE_INFORMATION  AppImage update info; when set, a .zsync file is produced as well
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
build="$out/build/appimage"
work="$out/appimage"
tools="$work/tools"
appdir="$work/AppDir"
dist="$work/out"

# Pinned tool releases (bump deliberately; checksums guard against a replaced asset).
linuxdeploy_url=https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage
linuxdeploy_sha=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d
qtplugin_url=https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-x86_64.AppImage
qtplugin_sha=15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724

fetch() { # url sha256 -> $tools/<basename>
    local file
    file="$tools/$(basename "$1")"
    if [[ ! -f "$file" ]] || ! echo "$2  $file" | sha256sum --check --status; then
        curl -fsSL --retry 3 -o "$file.part" "$1"
        echo "$2  $file.part" | sha256sum --check --quiet
        mv "$file.part" "$file"
    fi
    chmod +x "$file"
}

QMAKE=${QMAKE:-$(command -v qmake6 || command -v qmake || true)}
[[ -n "$QMAKE" ]] || { echo "error: qmake not found; set QMAKE" >&2; exit 1; }
export QMAKE

cd "$repo"
version=${QK_APPIMAGE_VERSION:-}
if [[ -z "$version" ]]; then
    version=$(sed -n 's/^ *VERSION \([0-9.]*\)$/\1/p' CMakeLists.txt | head -n1)
    version="$version-g$(git rev-parse --short HEAD)"
fi

# 1. Build + test. Warnings are policed by the CI build jobs, not here: the packaging
#    toolchain may be older than the one the code is developed with.
cmake --preset release -B "$build" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DQKRELLM_WARNINGS_AS_ERRORS=OFF \
    -DQKRELLM_BUILD_TOOLS=OFF >/dev/null
cmake --build "$build"
QT_QPA_PLATFORM=offscreen ctest --test-dir "$build" --output-on-failure

# 2. Stage the application (example plugins are a separate component and are not shipped).
rm -rf "$appdir" "$dist"
mkdir -p "$tools" "$dist"
DESTDIR="$appdir" cmake --install "$build" --component Unspecified --strip

# 3. Bundle Qt and package.
fetch "$linuxdeploy_url" "$linuxdeploy_sha"
fetch "$qtplugin_url" "$qtplugin_sha"
export APPIMAGE_EXTRACT_AND_RUN=1 # no FUSE needed (CI runners, containers)
export NO_STRIP=1                 # our binary is stripped at install; Qt ships stripped
export LINUXDEPLOY_OUTPUT_VERSION="$version"
if [[ -n "${QK_UPDATE_INFORMATION:-}" ]]; then
    export LDAI_UPDATE_INFORMATION="$QK_UPDATE_INFORMATION"
fi
id=io.github.ferion11.qkrellm
cd "$dist"
PATH="$tools:$PATH" "$tools/linuxdeploy-x86_64.AppImage" \
    --appdir "$appdir" \
    --desktop-file "$appdir/usr/share/applications/$id.desktop" \
    --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/$id.svg" \
    --plugin qt \
    --output appimage

ls -l "$dist"
