include(FetchContent)

# Point LVGL at our configuration before it is fetched/configured.
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)

FetchContent_Declare(lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(lvgl)

find_package(SDL2 REQUIRED CONFIG)

# LVGL's SDL driver needs the SDL headers/library.
target_link_libraries(lvgl PUBLIC SDL2::SDL2)

add_executable(raildeck-lvgl
    src/main.cpp
    src/ui.cpp
)

target_include_directories(raildeck-lvgl PRIVATE
    ${CMAKE_SOURCE_DIR}/shared   # generated theme_tokens.h
    src
)

target_link_libraries(raildeck-lvgl PRIVATE
    lvgl
    traincore::traincore
    SDL2::SDL2
)
