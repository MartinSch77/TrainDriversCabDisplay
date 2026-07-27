add_library(traincore STATIC
    src/train_simulation.cpp
    include/traincore/train_simulation.h
    include/traincore/train_types.h
)
add_library(traincore::traincore ALIAS traincore)

target_include_directories(traincore PUBLIC include)
target_compile_features(traincore PUBLIC cxx_std_17)
