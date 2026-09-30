idf_component_register(
    SRCS "audio_sink.cpp"
    INCLUDE_DIRS "include"
    REQUIRES audio_pipeline app_state esp_driver_gpio esp_driver_i2s
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
