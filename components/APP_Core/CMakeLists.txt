set(src_dirs
    src
    src/effect
)

set(include_dirs
    include
    include/effect
)

set(requires
    freertos
    esp_common
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)