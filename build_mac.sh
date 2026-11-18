#!/bin/bash
# Disco Valley SqueFM - one-line local build + install for macOS
set -e
echo "== Disco Valley SqueFM builder =="
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
if [ -d src/.git ]; then
  (cd src && git fetch --depth 1 origin main && git reset --hard origin/main)
else
  rm -rf src && git clone --depth 1 https://github.com/yhali243-alt/Squalch32.git src
fi
cd src
echo ">> Building SqueFM (a few minutes)..."
"$CM" -B build -DCMAKE_BUILD_TYPE=Release
"$CM" --build build --config Release --target SqueFM_VST3 -j"$(sysctl -n hw.ncpu)"
DEST="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$DEST"
# keep the previous working build until the new one is in place
if [ -d "$DEST/SqueFM.vst3" ]; then rm -rf "$W/SqueFM.previous.vst3"; cp -R "$DEST/SqueFM.vst3" "$W/SqueFM.previous.vst3"; fi
rm -rf "$DEST/SqueFM.vst3"
cp -R build/SqueFM_artefacts/Release/VST3/SqueFM.vst3 "$DEST/"
xattr -dr com.apple.quarantine "$DEST/SqueFM.vst3" 2>/dev/null || true
codesign --force --deep -s - "$DEST/SqueFM.vst3" 2>/dev/null || true
echo ""
echo "== DONE! Disco Valley SqueFM installed. Restart Cubase (or Studio > VST Plug-in Manager > Rescan) =="
