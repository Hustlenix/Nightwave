idf_component_register(SRCS "board_i2c.cpp" INCLUDE_DIRS "include"
    REQUIRES app_state esp_driver_i2c freertos)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
