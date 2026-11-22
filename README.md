# Real-Time Fluid

A Qt 6 desktop application that simulates a 2D flow through a channel in real time. It has a consistent dark interface with the system font on macOS, Windows and Linux. Draw obstacles into the channel, release particles, add ink and push the fluid with the mouse, and watch the velocity, pressure, divergence or ink density change live.

The solver implements "Stable Fluids" by Jos Stam (diffusion, projection and semi-Lagrangian convection; the linear systems are solved with Gauss-Seidel iterations). It is a plain C++ library without Qt and runs in its own thread, so the interface stays responsive.

## Background

This project was written during the software development internship (Softwareentwicklungspraktikum) of the Bachelor in Computational Engineering Science at RWTH Aachen University in summer semester 2017. Students work in small groups on a software project for one of the university's institutes, learning the Qt framework, teamwork, maintainability and documentation. This application was the main institute project. The original documentation (in German) is in `doc/tex/documentation.pdf`.

Authors:

- Simon Schäfer
- Markus Klaus Mizael Moser
- Sören Mehnert
- Sharon Michael Rouf-Chowdhury

## Using the application

Pick a stored simulation in the sidebar, or change the parameters of the running one. All changes are saved automatically in the active simulation; **Duplicate** keeps a copy to experiment with. A new grid size is applied with **Restart** (Ctrl+R, Cmd+R on macOS). The field shown is selected below the simulation (keys 1 to 4).

| Mouse | Action |
| ----- | ------ |
| Left drag | Release particles |
| Right drag | Release particles that leave a trace; in the density view, add ink |
| Shift + left drag | Draw obstacles |
| Shift + right drag | Erase obstacles |
| Ctrl + drag (Cmd on macOS) | Push the fluid in the drag direction |

The app ships with example simulations:

| Example | What to look at |
| ------- | --------------- |
| Vortex Street | A cylinder sheds alternating vortices (Kármán vortex street), best in the pressure view |
| Airfoil | A wing at 12° angle of attack stalls, a large vortex separates from its top |
| Tube Bank | Flow through a staggered array of tubes, like in a heat exchanger |
| Jet | A jet through a narrow slot rolls up into vortices on both sides |
| Labyrinth | The flow snakes between baffles, with recirculation in every chamber |
| RWTH | Lettering as obstacles |
| Karman, Laminar Flow, Nozzle, Nozzle Gas, Nozzle Honey | The original examples from 2017 |

The simulations are stored per user (`~/Library/Application Support/RWTH SDI/RealTimeFluid` on macOS, `%APPDATA%\RWTH SDI\RealTimeFluid` on Windows, `~/.local/share/RWTH SDI/RealTimeFluid` on Linux). Delete `simulations.json` there to get the original presets back.

## Download

Prebuilt apps are attached to every [release](https://github.com/Foxei/sdi-flow-control/releases):

| OS | File |
| -- | ---- |
| macOS 13+ (Apple Silicon and Intel) | `RealTimeFluid-macos-universal.zip` |
| Windows 10+ (x86_64) | `RealTimeFluid-windows-x86_64.zip` |
| Linux (x86_64 / ARM64) | `RealTimeFluid-linux-x86_64.AppImage`, `RealTimeFluid-linux-aarch64.AppImage` |

The macOS app is not notarized. On first launch, right-click it and choose **Open**, or run `xattr -dr com.apple.quarantine RealTimeFluid.app`.

## Try it: build a native app with Docker

All you need is [Docker](https://docs.docker.com/get-docker/). A single command builds a self-contained application for your operating system and writes it to `dist/`:

| Your OS | Command | Result |
| ------- | ------- | ------ |
| macOS   | `docker compose up macos`   | `dist/macos/RealTimeFluid.app` (universal: Apple Silicon + Intel, macOS 13+) |
| Windows | `docker compose up windows` | `dist/windows/RealTimeFluid/RealTimeFluid.exe` (x86_64, Windows 10+) |
| Linux   | `docker compose up linux`   | `dist/linux/RealTimeFluid-<arch>.AppImage` |

`docker compose up` without a service name builds Linux and Windows.

The first build downloads the toolchains and Qt and takes a few minutes. Later builds are cached.

### macOS notes

The macOS build cross-compiles with clang/lld against the official Qt frameworks. It uses the macOS SDK from your Xcode Command Line Tools, which Docker reads from your Mac at build time. That's why the macOS build only works on a Mac. Install the tools once with:

```bash
xcode-select --install
```

To use a different SDK, set `MACOS_SDK`, for example `MACOS_SDK=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk docker compose up macos`.

The app is ad-hoc signed. A locally built copy opens normally. If you copy it to another Mac (for example via AirDrop or a download), Gatekeeper will block it. In that case, remove the quarantine flag:

```bash
xattr -dr com.apple.quarantine RealTimeFluid.app
```

Without Docker, the macOS app can also be built natively. You need only Python 3 and the Command Line Tools; the script installs Qt, Eigen, CMake and Ninja into `build-macos/`:

```bash
packaging/build-macos-native.sh   # dist/macos/RealTimeFluid.app
```

### Linux notes

The AppImage is built for the architecture of the machine running Docker and needs glibc 2.39 or newer (Ubuntu 24.04, Debian 13, Fedora 40 or later). Make it executable and run it:

```bash
chmod +x dist/linux/RealTimeFluid-*.AppImage
./dist/linux/RealTimeFluid-*.AppImage
```

## Development

### VS Code devcontainer

The repository includes a devcontainer with GCC, CMake, Ninja, GDB, Doxygen, Eigen and Qt 6.10. All Docker builds start from the same image. In VS Code, run **Dev Containers: Reopen in Container**. CMake Tools is already configured to use `code/` as the source directory. The devcontainer is meant for editing and building. To run the GUI, build a native app with `docker compose up <os>`.

### Building natively

Requirements:

- Qt 6.8 or newer (Core, Gui, Widgets, Svg)
- Eigen 3.4
- CMake 3.16 or newer
- A C++17 compiler

```bash
cmake -S code -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/app/RealTimeFluid          # Linux/Windows
open build/src/app/RealTimeFluid.app   # macOS
```

If CMake can't find Qt or Eigen, pass their locations, for example `-DCMAKE_PREFIX_PATH=$(brew --prefix qt)` or `-DCMAKE_PREFIX_PATH=~/Qt/6.10.1/macos`. An Eigen source tree without CMake config can be used with `-DEIGEN3_INCLUDE_DIR=<path>`.

### Icons

All icons are SVGs in `code/res/icons/`, so they stay sharp on Retina and other high-DPI screens. The platform app icons in `code/res/app/` (`.icns` for macOS, `.ico` for Windows, `.png` for Linux) are generated from `app.svg`. After changing it, regenerate them with:

```bash
docker run --rm -v "$PWD:/w" -w /w debian:trixie-slim packaging/generate-icons.sh
```

### Releases

`.github/workflows/release.yml` builds all platforms when a GitHub release is published and attaches the files to it. Linux and Windows use the same Docker builds as above. macOS is built natively on a macOS runner with `packaging/build-macos-native.sh`, because GitHub's macOS runners have no Docker. To test the builds without creating a release, run the workflow manually from the Actions tab; the files then appear as workflow artifacts.

### Documentation

Generate the API documentation with Doxygen:

```bash
cd code && doxygen Doxyfile   # output in code/doxygen/
```

## Project layout

```
code/
  src/solver/    fluid solver library (C++17 + Eigen, no Qt)
  src/app/       Qt GUI (model / view / controller, solver in a worker thread)
  res/icons/     SVG icons (UI and app icon)
  res/app/       platform app icons, Info.plist, Windows resource file
  res/presets/   simulations shipped with the app
doc/             original documentation, presentations and UML diagrams (2017)
packaging/       cross-compilation toolchains, build and deployment scripts
.github/         release workflow
Dockerfile       build stages for the devcontainer and all platforms
```
