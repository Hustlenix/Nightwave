idf_component_register(
    SRCS "diagnostics.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_app_format esp_psram esp_system esp_timer heap
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
