# QUMIOS Open (QDF)

**QUMIOS Development Framework (QDF)** is an embedded IoT development framework that provides a unified HAL (Hardware Abstraction Layer), RTOS abstraction, and reusable components for building connected devices across multiple MCU platforms.

## Features

- **Multi-platform support**: ESP32 series, Lierda EC71x (cellular), and extensible board support
- **OS abstraction**: FreeRTOS, or bare-metal (no-OS)
- **Unified HAL**: UART, GPIO, SPI, PWM, RTC, Flash, CAN, ADC, USB, WiFi, modem, BLE
- **Rich components**: BLE remote, AT command server, network (TCP/UDP/TLS), PPPoS, NTP, HTTP, SFUD, KV/TS storage, cloud, CLI, multi-timer, multi-button, and more
- **Utilities**: KV store, TLV, TOTP, memory helpers, barcode (Code128), sscanf/sprintf, crypto (AES, SHA, HMAC, CRC), JSON, base64, and others
- **Kconfig-based configuration**: Menuconfig for selecting board, OS, components, and options; generates `qm_config.h` per application

## Project Structure

```
qumios-open/
├── board/              # Board selection (ESP32x, lierda_ec71x)
├── components/         # Reusable components (BLE, network, cloud, SFUD, etc.)
├── examples/           # Example applications (wifi, soc, ble, cloud, etc.)
├── include/            # Public API: qmos (OS/kernel), hal (soc, wifi, modem)
├── platform/mcu/       # MCU-specific implementations (esp32, lierda_ec71x)
├── tools/kconfig/      # Kconfig tooling (menuconfig, genconfig)
├── utility/            # Utility libraries (kv, tlv, totp, crypto, etc.)
├── qm_kconfig          # Root Kconfig
├── qdf.py              # Wrapper for menuconfig/genconfig
└── export.sh           # Sets QDF_PATH and PATH
```

## Quick Start

### 1. Set up the framework path

```bash
source export.sh
# or: export QDF_PATH=$(pwd) && export PATH=$(pwd):$PATH
```

### 2. Configure your application

Applications (e.g. under `examples/`) use a local `qm_config/` directory. From your app directory:

```bash
python ${QDF_PATH}/qdf.py menuconfig
```

This runs Kconfig menuconfig, then generates `qm_config/qm_config.h` from your choices (board, OS, components, HAL options, etc.).

### 3. Generate config only (if .config already exists)

```bash
python ${QDF_PATH}/qdf.py genconfig
```

### 4. Build and run (platform-dependent)

- **ESP32**: Use ESP-IDF in your app project; after sourcing IDF env, run for example:
  ```bash
  idf.py build flash monitor
  ```
- **Lierda EC71x**: Use the SDK/build system for that platform with QDF as the framework.

## Configuration Overview

- **Board**: Choose `esp32x` or `lierda_ec71x`.
- **QUMIOS**: Select OS type (QM OS support or no-OS), and if OS: FreeRTOS / AliOS Things / miio_vela. Configure task priorities and heap if needed.
- **HAL**: Enable and configure SOC drivers (UART, GPIO, SPI, PWM, RTC, Flash, CAN, ADC, USB), WiFi, and modem.
- **Components**: Enable only the components you need (qm_ble, qm_at, network, pppos, ntpdate, http, sfud, qm_cloud, ble_remoter, etc.).
- **Utility**: Enable kv, tlv, mtrace, totp, memory, barcode, sscanf, sprintf as needed.
- **Examples**: Pick the example app to build (e.g. AT server, soc demos).

Log output can be set to UART, JTAG, or USB CDC, with baud rate and pins configurable when UART is selected.

## Examples (high level)

| Category   | Examples |
|-----------|----------|
| **WiFi**  | sta, scan, ap, ap_sta |
| **SoC**   | uart, gpio, spi (master/slave), pwm, rtc, flash, can, adc, usb |
| **BLE**   | qm_ble, qm_ble_gap, ble_remoter |
| **Network** | TCP/UDP, TLS |
| **App**   | qm_work, qm_time, multi_timer, multi_button, relay, led_blink, web_server, weekly_timer |
| **Storage** | qm_db (KV/TS), sfud |
| **Cloud / OTA** | qm_cloud, ota_service |
| **Other** | qm_at (AT server), dns_server, comm_base, prodtst_client, utility (sprintf, sscanf, list) |

## Requirements

- **Python**: For Kconfig (menuconfig.py, genconfig.py) and qdf.py


## License

See repository or component-level license files (e.g. `utility/heatshrink/LICENSE`, `tools/kconfig/LICENSE.txt`) for terms. The framework may aggregate multiple licenses; check each subdirectory as needed.

## Contributing

Contributions are welcome. Please follow the project’s code style and add or update tests/docs as appropriate for your change.
