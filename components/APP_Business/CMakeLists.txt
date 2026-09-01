set(src_dirs
    APP_System/src
)

set(include_dirs
    APP_System/include
)

set(requires
    APP_Core
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)