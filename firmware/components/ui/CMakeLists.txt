idf_component_register(SRCS "oled_display.cpp" "player_frontend.cpp" "st7789_display.cpp" "st7789_spi.cpp" INCLUDE_DIRS "include"
    REQUIRES app_state esp_driver_i2c esp_driver_gpio esp_driver_spi esp_timer heap
        playback storage input settings library)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
