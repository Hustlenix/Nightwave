idf_component_register(
    SRCS "storage.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app_state fatfs sdmmc esp_driver_sdmmc esp_timer
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
