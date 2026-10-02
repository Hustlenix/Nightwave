idf_component_register(
    SRCS "wav_parser.cpp" "mp3_decoder.cpp" "mp3_seek_index.cpp"
    INCLUDE_DIRS "include"
    REQUIRES audio_pipeline chmorgan__esp-libhelix-mp3
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
