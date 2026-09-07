set(src_dirs
    APP_System/src
    APP_Test/src
    APP_Capture/src
    APP_Storage/src
    APP_UI/src
    APP_Application/src
    APP_Runtime/src
    )

set(include_dirs
    APP_System/include
    APP_Test/include
    APP_Capture/include
    APP_Storage/include
    APP_UI/include
    APP_Application/include
    APP_Runtime/include
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
