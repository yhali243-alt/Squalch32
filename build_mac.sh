#!/bin/bash
# Squalch32 - one-line local build + install for macOS
set -e
echo "== Squalch32 builder =="
if ! xcode-select -p >/dev/null 2>&1; then
  echo ">> Installing Apple Command Line Tools. Click 'Install' in the window that opens."
  echo ">> When it finishes, run the same command again."
  xcode-select --install || true
  exit 0
fi
W="$HOME/Squalch32-build"
mkdir -p "$W" && cd "$W"
CMV=3.31.6
if [ ! -x "$W/cmake/CMake.app/Contents/bin/cmake" ]; then
  echo ">> Downloading CMake..."
  curl -fL -o cmake.tgz "https://github.com/Kitware/CMake/releases/download/v$CMV/cmake-$CMV-macos-universal.tar.gz"
  tar xzf cmake.tgz && rm -rf cmake && mv "cmake-$CMV-macos-universal" cmake && rm cmake.tgz
fi
CM="$W/cmake/CMake.app/Contents/bin/cmake"
echo ">> Getting plugin source..."
rm -rf src && git clone --depth 1 https://github.com/yhali243-alt/Squalch32.git src
cd src
echo ">> Building (first time takes ~5-10 minutes)..."
"$CM" -B build -DCMAKE_BUILD_TYPE=Release
"$CM" --build build --config Release --target Squalch32_VST3 -j"$(sysctl -n hw.ncpu)"
DEST="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$DEST"
rm -rf "$DEST/Squalch32.vst3"
cp -R build/Squalch32_artefacts/Release/VST3/Squalch32.vst3 "$DEST/"
xattr -dr com.apple.quarantine "$DEST/Squalch32.vst3" 2>/dev/null || true
codesign --force --deep -s - "$DEST/Squalch32.vst3" 2>/dev/null || true
echo ""
echo "== DONE! Squalch32 installed. In Cubase: Studio > VST Plug-in Manager > Rescan =="
