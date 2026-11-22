#!/usr/bin/env bash
# Build a universal RealTimeFluid.app natively on macOS (used by CI, works
# locally too). Needs only Python 3 and the Xcode Command Line Tools; Qt,
# Eigen, CMake and Ninja are installed into the work directory.
#
# Usage: packaging/build-macos-native.sh [output-dir]   (default: dist/macos)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-${ROOT}/dist/macos}"
WORK="${WORK_DIR:-${ROOT}/build-macos}"
QT_VERSION="$(sed -n 's/^ARG QT_VERSION=//p' "${ROOT}/Dockerfile")"

# Tools
python3 -m venv "${WORK}/venv"
"${WORK}/venv/bin/pip" install --quiet aqtinstall cmake ninja
export PATH="${WORK}/venv/bin:${PATH}"

# Qt (same version as the Docker builds)
QT="${WORK}/qt/${QT_VERSION}/macos"
if [[ ! -d "${QT}" ]]; then
  aqt install-qt mac desktop "${QT_VERSION}" clang_64 -O "${WORK}/qt"
fi

# Eigen (header only, same version as Ubuntu 24.04 in the dev container)
EIGEN="${WORK}/eigen-3.4.0"
if [[ ! -d "${EIGEN}" ]]; then
  curl -fsSL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz | tar -xz -C "${WORK}"
fi

# Build arm64 + x86_64 in one go
cmake -S "${ROOT}/code" -B "${WORK}/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${QT}" \
    -DEIGEN3_INCLUDE_DIR="${EIGEN}" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build "${WORK}/build"

# Bundle Qt into the app
mkdir -p "${OUT}"
rm -rf "${OUT}/RealTimeFluid.app"
cp -R "${WORK}/build/src/app/RealTimeFluid.app" "${OUT}/"
"${ROOT}/packaging/deploy-macos.sh" "${OUT}/RealTimeFluid.app" "${QT}"

# Ad-hoc sign the complete bundle (no Apple Developer ID needed)
codesign --force --deep --sign - "${OUT}/RealTimeFluid.app"
codesign --verify --deep --strict "${OUT}/RealTimeFluid.app"
echo "Built ${OUT}/RealTimeFluid.app"
