idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        app_state
        storage
        library
        decoder
        audio_pipeline
        audio_i2s
        playback
        input
        ui
        power
        settings
        diagnostics
        bluetooth
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
