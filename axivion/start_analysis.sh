#!/bin/bash
set -ex -o pipefail
# Only one axivion_ci per project: concurrent runs (e.g. build_all.sh while a
# manual run is active) share build_axivion/ and delete each other's IR right
# before the dashboard upload. clean_all.sh during a run does the same.
exec 9>"$HOME/.axivion-RailDeckPro.lock"
if ! flock -n 9; then
    echo "another Axivion run for RailDeckPro is already active — aborting" >&2
    exit 1
fi

if [ ! -f "$HOME/bauhaus-suite/bauhaus-kshrc" ]; then
    echo "Axivion Suite not found at ~/bauhaus-suite — stage skipped (license-bound tool)" >&2
    exit 3 # build_all.sh reports exit code 3 as "skipped", not as a failure
fi
. "$HOME/bauhaus-suite/bauhaus-kshrc"
if [ -z "${AXIVION_USERNAME:-}" ] && [ -z "${AXIVION_PASSWORD:-}" ] && [ -z "${AXIVION_PASSFILE:-}" ]; then
    # You may put dashboard credentials inside such a guarded block:
    export AXIVION_USERNAME=admin
    export AXIVION_PASSWORD=password
fi
export BAUHAUS_CONFIG="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
# Toolchain setup command (regenerate after a compiler update):
#   ~/bauhaus-suite/bin/gccsetup --cc 'gcc ' --cxx 'g++ ' --config "$BAUHAUS_CONFIG/compiler_config.json"

# The Axivion cmake configure doesn't pass -DCMAKE_PREFIX_PATH, so find_package
# would fall back to the system Qt6. Point it at the same online-installer Qt
# the normal build uses; prepend so any existing value still wins.
export CMAKE_PREFIX_PATH="$HOME/Qt/6.11.1/gcc_64${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"

# Analyze the Qt frontend + core only: the LVGL frontend compiles ~800 fetched
# third-party C files whose MISRA findings would drown the project's own.
# (CMakeLists.txt reads this env var as the RAILDECK_UI default.)
export RAILDECK_UI=qt

# --jobs (no N) = parallel analysis jobs, auto-sized. Placed after "$@" so an
# explicit caller-supplied -j N still wins.
axivion_ci "$@" --jobs
