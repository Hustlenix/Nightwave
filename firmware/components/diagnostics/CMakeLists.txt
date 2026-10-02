idf_component_register(
    SRCS "diagnostics.cpp" "engineering_report.cpp" "runtime_recorder.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_app_format esp_psram esp_system esp_timer heap spi_flash playback power
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
