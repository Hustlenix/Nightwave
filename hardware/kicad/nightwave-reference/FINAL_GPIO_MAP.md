# Reference GPIO map

This map matches the reference schematic, not the historical OLED/direct-button firmware configuration. Live adapters remain unqualified.

| Module pad | GPIO/function | Net |
| --- | --- | --- |
| 1 | GND | GND |
| 2 | 3V3 | +3V3 |
| 3 | EN | ESP_EN |
| 4 | GPIO4 | BT_MFB |
| 5 | GPIO5 | I2S_BCLK |
| 6 | GPIO6 | I2S_LRCLK |
| 7 | GPIO7 | I2S_DOUT |
| 8 | GPIO15 | BT_RST_N |
| 9 | GPIO16 | HP_DETECT |
| 10 | GPIO17 | SPK_EN |
| 11 | GPIO18 | DAC_XSMT |
| 12 | GPIO8 | I2C_SDA |
| 13 | GPIO19_USB_D- | USB_DM_MCU |
| 14 | GPIO20_USB_D+ | USB_DP_MCU |
| 15 | GPIO3 | NC |
| 16 | GPIO46 | NC |
| 17 | GPIO9 | I2C_SCL |
| 18 | GPIO10 | BT_WAKE |
| 19 | GPIO11 | SD_CMD |
| 20 | GPIO12 | SD_CLK |
| 21 | GPIO13 | SD_D0 |
| 22 | GPIO14 | TCA_INT_N |
| 23 | GPIO21 | BT_MCLK_OPTION |
| 24 | GPIO47 | TFT_MOSI |
| 25 | GPIO48 | TFT_SCLK |
| 26 | GPIO45 | NC |
| 27 | GPIO0 | BOOT_N |
| 28 | GPIO35_PSRAM | NC |
| 29 | GPIO36_PSRAM | NC |
| 30 | GPIO37_PSRAM | NC |
| 31 | GPIO38 | TFT_BL_PWM |
| 32 | GPIO39 | HP_EN |
| 33 | GPIO40 | TFT_RST |
| 34 | GPIO41 | TFT_DC |
| 35 | GPIO42 | TFT_CS |
| 36 | RXD0 | UART0_RX |
| 37 | TXD0 | UART0_TX |
| 38 | GPIO2 | BT_UART_RX |
| 39 | GPIO1 | BT_UART_TX |
| 40 | GND | GND |
| 41 | EP_GND | GND |

## TCA9535 input map, address 0x20

| IC pad | Port/function | Net |
| --- | --- | --- |
| 1 | INT | TCA_INT_N |
| 2 | A1 | GND |
| 3 | A2 | GND |
| 4 | P0_0 | BTN_PREV_N |
| 5 | P0_1 | BTN_PLAY_N |
| 6 | P0_2 | BTN_NEXT_N |
| 7 | P0_3 | BTN_VOL_DOWN_N |
| 8 | P0_4 | BTN_VOL_UP_N |
| 9 | P0_5 | SD_DETECT_N |
| 10 | P0_6 | CHG_STAT |
| 11 | P0_7 | FUEL_ALERT_N |
| 12 | GND | GND |
| 13 | P1_0 | CHG_INT_N |
| 14 | P1_1 | PGOOD_CHG |
| 15 | P1_2 | PWR_GOOD_3V3 |
| 16 | P1_3 | PWR_GOOD_BT |
| 17 | P1_4 | EXP_IN14 |
| 18 | P1_5 | EXP_IN15 |
| 19 | P1_6 | EXP_IN16 |
| 20 | P1_7 | EXP_IN17 |
| 21 | A0 | GND |
| 22 | SCL | I2C_SCL |
| 23 | SDA | I2C_SDA |
| 24 | VCC | +3V3 |
