idf_component_register(
    SRCS "button_monitor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app_state esp_driver_gpio esp_timer freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
