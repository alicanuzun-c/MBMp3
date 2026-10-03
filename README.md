## MBMp3

Custom made MP3 Player.

## Hardware

### ESP32 and ILI9341 TFT

The display is configured for 240 x 320 portrait resolution. Connect the TFT and resistive touch controller as follows:

| TFT / touch pin | ESP32 pin |
| --- | --- |
| TFT SDO / MISO | GPIO 12 |
| TFT SDI / MOSI | GPIO 13 |
| TFT SCK | GPIO 14 |
| TFT CS | GPIO 15 |
| TFT D/C | GPIO 2 |
| TFT RESET | EN / RESET |
| TFT LED / backlight | GPIO 21 |
| Touch T_IRQ | GPIO 36 |
| Touch T_OUT / MISO | GPIO 39 |
| Touch T_DIN / MOSI | GPIO 32 |
| Touch T_CS | GPIO 33 |
| Touch T_CLK | GPIO 25 |
| GND | GND |
| VCC | 3.3 V |

The TFT uses the ILI9341 controller. Its reset pin is configured as `-1`, so the display reset is tied to the ESP32 EN/RESET line. Power the module from 3.3 V as requested for this setup.

### External microSD module

The Robotistan microSD module is wired on separate software-SPI pins so it does not share the TFT or touch signal wires:

| SD module pin | ESP32 pin |
| --- | --- |
| SCK / CLK | GPIO 18 |
| MISO / DO | GPIO 19 |
| MOSI / DI | GPIO 23 |
| CS | GPIO 26 |
| VCC | 5 V (level-shifting module) |
| GND | GND |

Keep all grounds common. The TFT remains on HSPI (GPIO 14/12/13), and the XPT2046 touch controller remains on GPIO 25/39/32 with CS on GPIO 33. Firmware uses SdFat 2.3.1 software SPI (`SPI_DRIVER_SELECT=2`); the `SD_SCK_MHZ()` setting does not control the software-SPI clock.

## Software

- Board: ESP32 Dev Module (`esp32dev`), Arduino framework.
- Display driver: TFT_eSPI 2.5.43, ILI9341, HSPI at 40 MHz.
- Touch controller: XPT2046, using its own SPI pins and chip-select.
- Storage: SdFat 2.3.1 software SPI on GPIO 18/19/23, with SD CS on GPIO 26.
- UI: LVGL 9.5.0 with the three-screen EEZ Studio (EEZ Flow) export in `Software/src/ui`.
- Runtime: LVGL and EEZ Flow are serviced by a dedicated FreeRTOS task.
- LVGL display color format: RGB565. For transparent EEZ image assets, export with alpha (RGB565A8 or ARGB8888); plain RGB565 does not preserve transparency.

Touch input is calibrated in `Software/src/main.cpp`. The current measured calibration is X left/right `236/13` and Y top/bottom `313/7`, mapped to display coordinates `0..239` and `0..319`. Recalibrate these values if the touch panel or rotation changes.

## Build and Upload

Open the `Software` folder as the PlatformIO project, then run:

```sh
pio run
pio run --target upload
pio device monitor
```