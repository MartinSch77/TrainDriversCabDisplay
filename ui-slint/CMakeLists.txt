include(FetchContent)

# Slint C++ is consumed as the official prebuilt binary package - it bundles
# the slint-compiler, the headers and libslint_cpp.so, so no Rust toolchain
# is needed. Fetched at configure time, exactly like the LVGL sources.
set(SLINT_VERSION 1.17.1)
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(SLINT_ARCH "arm64")
    set(SLINT_SHA256 7580dca6b2f24d135439dbba33860c041e2db9f17c6cde60000c7af73b420ecf)
else()
    set(SLINT_ARCH "x86_64")
    set(SLINT_SHA256 a7ca87bbccfa892ce9cad36794c666f8dfa787fdc86acbb6fa826d909d268b37)
endif()

FetchContent_Declare(slint_cpp
    URL https://github.com/slint-ui/slint/releases/download/v${SLINT_VERSION}/Slint-cpp-${SLINT_VERSION}-Linux-${SLINT_ARCH}.tar.gz
    URL_HASH SHA256=${SLINT_SHA256}
)
FetchContent_MakeAvailable(slint_cpp)

find_package(Slint REQUIRED
    PATHS "${slint_cpp_SOURCE_DIR}/lib/cmake/Slint"
    NO_DEFAULT_PATH
)

add_executable(raildeck-slint
    src/main.cpp
)

# Generates Main.h (+ compilation units) from ui/Main.slint and its imports,
# including the generated ui/Theme.slint design tokens.
slint_target_sources(raildeck-slint ui/Main.slint)

target_link_libraries(raildeck-slint PRIVATE
    Slint::Slint
    traincore::traincore
)
