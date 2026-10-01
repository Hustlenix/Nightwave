idf_component_register(SRCS "oled_display.cpp" "player_frontend.cpp" INCLUDE_DIRS "include"
    REQUIRES app_state esp_driver_i2c esp_driver_gpio esp_timer heap
        playback storage input settings)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
