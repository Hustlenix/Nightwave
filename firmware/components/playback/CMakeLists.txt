idf_component_register(
    SRCS "wav_player.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app_state audio_pipeline audio_i2s decoder freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
