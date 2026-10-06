idf_component_register(SRCS "reference_power.cpp" "power_i2c.cpp" INCLUDE_DIRS "include"
    REQUIRES board_io esp_driver_i2c esp_rom)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
