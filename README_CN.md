# QUMIOS Open (QDF)

**QUMIOS 开发框架（QDF，QUMIOS Development Framework）** 是一款面向嵌入式 IoT 的开发框架，提供统一的 HAL（硬件抽象层）、RTOS 抽象以及可复用组件，用于在多种 MCU 平台上开发联网设备。

## 特性

- **多平台支持**：ESP32 系列、利尔达 EC71x（蜂窝模组）等，可扩展更多板级支持
- **OS 抽象**：支持 FreeRTOS，或裸机（无 OS）
- **统一 HAL**：UART、GPIO、SPI、PWM、RTC、Flash、CAN、ADC、USB、WiFi、 modem、BLE 等
- **丰富组件**：蓝牙遥控、AT 指令服务、网络（TCP/UDP/TLS）、PPPoS、NTP、HTTP、SFUD、KV/TS 存储、云接入、CLI、多定时器、多按键等
- **工具库**：KV 存储、TLV、TOTP、内存工具、条码（Code128）、sscanf/sprintf、加解密（AES、SHA、HMAC、CRC）、JSON、base64 等
- **Kconfig 配置**：通过 menuconfig 选择板子、OS、组件与选项，按应用生成 `qm_config.h`

## 目录结构

```
qumios-open/
├── board/              # 板级选择（ESP32x、lierda_ec71x）
├── components/         # 可复用组件（BLE、网络、云、SFUD 等）
├── examples/           # 示例工程（wifi、soc、ble、云等）
├── include/            # 对外 API：qmos（OS/内核）、hal（soc、wifi、modem）
├── platform/mcu/       # MCU 具体实现（esp32、lierda_ec71x）
├── tools/kconfig/      # Kconfig 工具（menuconfig、genconfig）
├── utility/            # 工具库（kv、tlv、totp、加解密等）
├── qm_kconfig          # 根 Kconfig
├── qdf.py              # menuconfig/genconfig 入口脚本
└── export.sh           # 设置 QDF_PATH 与 PATH
```

## 快速开始

### 1. 设置框架路径

```bash
source export.sh
# 或：export QDF_PATH=$(pwd) && export PATH=$(pwd):$PATH
```

### 2. 配置应用

应用通常位于 `examples/` 下，各有本地 `qm_config/`。在对应应用目录下执行：

```bash
python ${QDF_PATH}/qdf.py menuconfig
```

会进入 Kconfig 菜单，选择板子、OS、组件、HAL 等后，生成该应用的 `qm_config/qm_config.h`。

### 3. 仅生成配置（已有 .config 时）

```bash
python ${QDF_PATH}/qdf.py genconfig
```

### 4. 编译与运行（与平台相关）

- **ESP32**：在应用工程中配合 ESP-IDF 使用；先加载 IDF 环境，再执行例如：
  ```bash
  idf.py build flash monitor
  ```
- **利尔达 EC71x**：使用该平台对应的 SDK 与构建系统，将 QDF 作为框架集成。

## 配置概览

- **板级**：选择 `esp32x` 或 `lierda_ec71x`
- **QUMIOS**：选择是否使用 OS（QM OS 或 no-OS）；若使用 OS，可选 FreeRTOS / AliOS Things / miio_vela，并可配置任务优先级、堆大小等
- **HAL**：按需使能并配置 SOC 驱动（UART、GPIO、SPI、PWM、RTC、Flash、CAN、ADC、USB）、WiFi、modem
- **组件**：按需使能 qm_ble、qm_at、network、pppos、ntpdate、http、sfud、qm_cloud、ble_remoter 等
- **工具库**：按需使能 kv、tlv、mtrace、totp、memory、barcode、sscanf、sprintf 等
- **示例**：选择要编译的示例（如 AT 服务器、soc 外设示例等）

日志可配置为 UART、JTAG 或 USB CDC；使用 UART 时可配置波特率与引脚。

## 示例分类概览

| 类别     | 示例 |
|----------|------|
| **WiFi** | sta、scan、ap、ap_sta |
| **SoC**  | uart、gpio、spi（主/从）、pwm、rtc、flash、can、adc、usb |
| **BLE**  | qm_ble、qm_ble_gap、ble_remoter |
| **网络** | TCP/UDP、TLS |
| **应用** | qm_work、qm_time、multi_timer、multi_button、relay、led_blink、web_server、weekly_timer |
| **存储** | qm_db（KV/TS）、sfud |
| **云/OTA** | qm_cloud、ota_service |
| **其他** | qm_at（AT 服务器）、dns_server、comm_base、prodtst_client、utility（sprintf、sscanf、list） |

## 环境要求

- **Python**：用于 Kconfig（menuconfig.py、genconfig.py）及 qdf.py

## 许可证

具体条款见仓库或各组件下的许可证文件（如 `utility/heatshrink/LICENSE`、`tools/kconfig/LICENSE.txt`）。本框架可能包含多种许可证，请按子目录查阅。

## 参与贡献

欢迎提交贡献。请遵循项目代码风格，并对改动补充或更新测试与文档。
