idf_component_register(
    SRCS "wav_parser.cpp"
    INCLUDE_DIRS "include"
    REQUIRES audio_pipeline
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
