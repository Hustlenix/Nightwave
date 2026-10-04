# Nightwave reference GPIO map

This map is generated into the AI-authored reference schematic. It is a review
input, not a claim of tested hardware.

| Function | ESP32-S3 GPIO | Notes |
| --- | ---: | --- |
| BM83 UART TX / RX | 1 / 2 | Crossed at BM83 RX/TX |
| BM83 MFB | 4 | Power/key control; sequencing review required |
| I2S BCLK / LRCLK / DOUT | 5 / 6 / 7 | Fans out through 33-ohm source branches |
| I2C SDA / SCL | 8 / 9 | TCA9535, gauge, charger/CC controller as reviewed |
| BM83 wake | 10 | P0_0 / UART_TX_IND function must match AT image |
| SD CMD / CLK / D0 | 11 / 12 / 13 | 1-bit SDMMC |
| TCA9535 INT | 14 | Open-drain; pull-up required |
| BM83 reset | 15 | Verify active polarity and rail sequencing |
| Headphone detect | 16 | Uses jack switch network; qualify electrically |
| Speaker enable | 17 | Class-D shutdown |
| DAC mute | 18 | PCM5102A XSMT |
| USB D- / D+ | 19 / 20 | Native USB, controlled pair routing required |
| Optional BM83 MCLK | 21 | Reserved until AT/I2S clock qualification |
| TFT backlight PWM | 38 | Verify backlight interface/current |
| Headphone amp enable | 39 | Pop/click sequencing required |
| TFT reset / DC / CS | 40 / 41 / 42 | Direct control |
| UART0 TX / RX | 43 / 44 | Bring-up header only |
| TFT MOSI / SCLK | 47 / 48 | ST7789V2 four-wire SPI |

## TCA9535 input map (address straps A2:A0 = 000, nominal 0x20)

| Port | Input |
| --- | --- |
| P0.0-P0.4 | Previous, play/pause, next, volume down, volume up |
| P0.5-P0.7 | SD detect, charger status, fuel alert |
| P1.0-P1.3 | Charger IRQ, USB-C controller IRQ, 3.3-V power-good, Bluetooth power-good |
| P1.4-P1.7 | Reserved inputs with explicit pulls required |
