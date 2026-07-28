#!/usr/bin/env bash
# Provision a naked Debian/Ubuntu Linux with every tool this project needs,
# idempotently — existing tools are left alone — and keep them current:
#
#   ./setup.sh [install]   install everything that is missing
#   ./setup.sh update      update all managed tools to their latest versions
#   ./setup.sh status      report found/missing tools and versions (read-only)
#
# What it manages
#   apt    build-essential, ninja, git, SDL2 dev (LVGL simulator),
#          clang-18 + LLVM tools, clang-tidy, cppcheck, clazy, valgrind,
#          lcov, doxygen/graphviz, python3 + pipx, Qt xcb/OpenGL runtime libs,
#          DejaVu fonts (monospace readouts in all three frontends)
#   pipx   cmake (>= 3.21 — distro cmake is often too old), strictdoc,
#          aqtinstall, codespell
#   aqt    Qt ${QT_VERSION} (gcc_64 + qtquick3d) into ~/Qt — the layout the
#          build scripts expect (override with QT_PREFIX at build time)
#
# The Slint frontend needs no provisioning here: CMake fetches the official
# prebuilt Slint C++ package (compiler + library) at configure time, the same
# way the LVGL sources are fetched.
#
# NOT installable here (license-bound, detected + reported only):
#   Axivion Suite (~/bauhaus-suite + dashboard) and Squish Coco
#   (/opt/SquishCoco). Both are optional — the pipeline skips them cleanly.
#
# install/update need sudo for the apt part; everything else stays in $HOME.
set -uo pipefail

MODE="${1:-install}"
QT_VERSION="${QT_VERSION:-6.10.2}"
QT_DIR="$HOME/Qt"
export PATH="$HOME/.local/bin:$PATH"

SUDO=""
[ "$(id -u)" -ne 0 ] && SUDO="sudo"

# Everything apt-managed. Qt runtime libs cover running the GUI/tests on a
# naked system (xcb platform plugin, OpenGL, fontconfig); SDL2 is the LVGL
# simulator backend.
APT_PKGS=(
    build-essential ninja-build git gh curl ca-certificates
    libsdl2-dev
    clang-18 llvm-18 clang-tidy clang-tools-18
    cppcheck clazy valgrind lcov
    doxygen graphviz
    python3 python3-venv python3-pip pipx
    libgl1-mesa-dev libglx-dev libopengl0 libegl1
    libxkbcommon0 libxkbcommon-x11-0 libfontconfig1 libfreetype6 libdbus-1-3
    libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0
    libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1
    fonts-dejavu-core
)
PIPX_PKGS=(cmake strictdoc aqtinstall codespell)

have() { command -v "$1" >/dev/null 2>&1; }

version_of() {
    case "$1" in
    cmake) cmake --version 2>/dev/null | head -1 | awk '{print $3}' ;;
    g++) g++ -dumpfullversion 2>/dev/null ;;
    clang-18) clang-18 --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 ;;
    qt) [ -d "$QT_DIR/$QT_VERSION/gcc_64" ] && echo "$QT_VERSION" ;;
    axivion) [ -x "$HOME/bauhaus-suite/bin/axivion_ci" ] && "$HOME/bauhaus-suite/bin/axivion_ci" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9.]+' | head -1 ;;
    coco) [ -x "/opt/SquishCoco/bin/csg++" ] && echo "installed" ;;
    *) "$1" --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9.]+' | head -1 ;;
    esac
}

status() {
    echo "== apt tools =="
    for t in g++ ninja git cppcheck clang-18 clang-tidy clazy-standalone valgrind lcov doxygen sdl2-config; do
        printf '  %-18s %s\n' "$t" "$(have "$t" && version_of "$t" || echo MISSING)"
    done
    echo "== pipx tools =="
    for t in cmake strictdoc aqt codespell; do
        printf '  %-18s %s\n' "$t" "$(have "$t" && version_of "$t" || echo MISSING)"
    done
    echo "== Qt kit =="
    printf '  %-18s %s\n' "Qt $QT_VERSION" "$([ -d "$QT_DIR/$QT_VERSION/gcc_64" ] && echo "$QT_DIR/$QT_VERSION/gcc_64" || echo MISSING)"
    echo "== Slint (prebuilt, fetched by CMake at configure time) =="
    SLINT_COMPILER_BIN="$(find "$(dirname "$0")/build/_deps" -name slint-compiler -type f 2>/dev/null | head -1)"
    printf '  %-18s %s\n' "Slint C++" "$([ -n "$SLINT_COMPILER_BIN" ] && "$SLINT_COMPILER_BIN" --version 2>/dev/null || echo "not fetched yet (cmake -DRAILDECK_UI=slint|all)")"
    echo "== license-bound (optional) =="
    printf '  %-18s %s\n' "Axivion Suite" "$(version_of axivion || echo "not found (~/bauhaus-suite)")"
    printf '  %-18s %s\n' "Squish Coco" "$(version_of coco || echo "not found (/opt/SquishCoco)")"
}

install_apt() {
    echo "== apt install =="
    $SUDO apt-get update -qq
    $SUDO apt-get install -y -qq "${APT_PKGS[@]}"
}

install_pipx() {
    echo "== pipx install =="
    pipx ensurepath >/dev/null 2>&1 || true
    for p in "${PIPX_PKGS[@]}"; do
        if pipx list --short 2>/dev/null | grep -q "^$p "; then
            [ "$MODE" = "update" ] && pipx upgrade "$p"
        else
            pipx install "$p"
        fi
    done
}

install_qt() {
    if [ -d "$QT_DIR/$QT_VERSION/gcc_64" ]; then
        echo "Qt $QT_VERSION already at $QT_DIR/$QT_VERSION/gcc_64"
        return 0
    fi
    echo "== aqt install Qt $QT_VERSION (gcc_64 + qtquick3d) =="
    aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 \
        --outputdir "$QT_DIR" --modules qtquick3d
}

case "$MODE" in
status)
    status
    ;;
install | update)
    install_apt
    install_pipx
    install_qt
    echo
    status
    ;;
*)
    echo "usage: $0 [install|update|status]" >&2
    exit 2
    ;;
esac
