idf_component_register(
    SRCS "button_monitor.cpp" "tca9535_i2c.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app_state esp_driver_gpio esp_driver_i2c esp_timer freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
