cmake_minimum_required(VERSION 3.21)

project(RailDeckPro VERSION 1.0.0 LANGUAGES C CXX)

# RailDeck Pro - train driver's cab display with interchangeable UI frontends.
# The simulation core is UI-agnostic; pick the frontend(s) at configure time:
#   -DRAILDECK_UI=qt | lvgl | both | core   (default: both)
# "core" builds only the simulation library + tests (used by the sanitizer
# and coverage trees, which need neither Qt nor SDL).
# The environment variable RAILDECK_UI seeds the default for build drivers
# that cannot pass -D options (axivion/start_analysis.sh).
if(DEFINED ENV{RAILDECK_UI} AND NOT DEFINED CACHE{RAILDECK_UI})
    set(RAILDECK_UI "$ENV{RAILDECK_UI}" CACHE STRING
        "UI frontend(s) to build: qt, lvgl, both or core")
endif()
set(RAILDECK_UI "both" CACHE STRING "UI frontend(s) to build: qt, lvgl, both or core")
set_property(CACHE RAILDECK_UI PROPERTY STRINGS qt lvgl both core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)

enable_testing()
add_subdirectory(tests)

if(RAILDECK_UI STREQUAL "qt" OR RAILDECK_UI STREQUAL "both")
    add_subdirectory(ui-qt)
endif()

if(RAILDECK_UI STREQUAL "lvgl" OR RAILDECK_UI STREQUAL "both")
    add_subdirectory(ui-lvgl)
endif()
