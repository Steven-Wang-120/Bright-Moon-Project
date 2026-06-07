# STM32H743 UART Face Display

当前版本：`v0.1.1`

这个目录是一套面向 `STM32H743ZIT6` 的最小自包含固件工程，目标是直接接收 Orange Pi 通过串口发来的单行 JSON，然后在 SSD1306 屏幕上显示表情。

## GitHub 上传状态

这个目录已经整理成适合直接上传 GitHub 的形式：

- 编译产物 `build/`、`*.elf`、`*.bin`、`*.map` 已写入 `.gitignore`
- 版本号写在 [VERSION](VERSION) 和 [inc/version.h](inc/version.h)
- README 中的代码路径已改成 GitHub 可用的相对链接

## 目标链路

```text
AstrBot 插件 -> MQTT -> Orange Pi MQTT-UART Bridge -> USART1 -> STM32H743ZIT6 -> SSD1306
```

本固件不再依赖旧的 `STM32F103 + MQTT 文本协议` 原型。

## 当前实现

- MCU: `STM32H743ZIT6`
- 工具链: `arm-none-eabi-gcc`
- 通信: `USART1`
- 屏幕: `SSD1306 128x64`
- 屏幕总线: 软件 `I2C`
- 协议: `一行一个 JSON，以 \n 结束`

## 选择的引脚

当前固件使用下面这一组引脚：

- `USART1_TX = PA9`
- `USART1_RX = PA10`
- `OLED_SCL = PC8`
- `OLED_SDA = PC9`
- `GND` 共地

如果你板子上这组脚不方便，可以直接修改 [inc/board.h](inc/board.h)。

## 支持的 JSON

### AstrBot 当前 MQTT JSON

```json
{"face":"happy","intensity":65}
```

### 扩展格式

```json
{"cmd":"show","emo":"thinking","dur":3000}
```

### 其他命令

清屏：

```json
{"cmd":"clear"}
```

心跳：

```json
{"cmd":"ping"}
```

## 串口行为

固件按行接收，`\r` 会被忽略，`\n` 代表一帧结束。

成功时会返回：

```text
ACK show happy 65
ACK show thinking 60 3000
ACK clear
ACK ping
```

解析失败时会返回：

```text
ERR missing face/emo
ERR unsupported face
ERR line too long
```

如果消息里带 `dur`，到时后会自动恢复默认中性脸，并返回：

```text
ACK revert neutral 50
```

上电启动时会先输出一行版本标识：

```text
READY stm32_h743_uart_face_display v0.1.1 json-line stm32h743 uart1 pa9-pa10 oled pc8-pc9
```

## 表情显示

为了保持工程自包含，这一版没有依赖外部素材文件，而是直接在 SSD1306 上绘制 6 种基础表情：

- `happy`
- `sad`
- `angry`
- `surprised`
- `thinking`
- `neutral`

这样更适合当前“先打通串口收 JSON 控屏”的最小可用目标。

## 硬件连接

### Orange Pi 和 STM32

- Orange Pi `TX` -> STM32 `PA10 (USART1_RX)`
- Orange Pi `RX` -> STM32 `PA9 (USART1_TX)`
- Orange Pi `GND` -> STM32 `GND`

### SSD1306

- `VCC` -> `3.3V`
- `GND` -> `GND`
- `SCL` -> `PC8`
- `SDA` -> `PC9`

注意：

- 这是 `3.3V TTL`
- SSD1306 模块最好自带 I2C 上拉
- 当前 OLED 使用软件 I2C，不依赖特定硬件 I2C 外设配置

## 编译

要求先装好 `GNU Arm Embedded Toolchain` 和 `make`。

```bash
cd stm32_h743_uart_face_display
make
```

生成文件：

- `build/stm32_h743_uart_face_display.elf`
- `build/stm32_h743_uart_face_display.bin`

## 烧录

如果你有 `OpenOCD + ST-LINK`：

```bash
make flash
```

当前 [openocd.cfg](openocd.cfg) 默认使用：

- `interface/stlink.cfg`
- `target/stm32h7x.cfg`

如果你用的是 CubeProgrammer，也可以直接烧 `build/stm32_h743_uart_face_display.bin` 到 `0x08000000`。

## 工程结构

- [inc/board.h](inc/board.h)：板级引脚和波特率
- [inc/mcu.h](inc/mcu.h)：最小寄存器定义
- [inc/version.h](inc/version.h)：固件名和版本号
- [src/main.c](src/main.c)：主循环、协议处理、ACK
- [src/json_protocol.c](src/json_protocol.c)：轻量 JSON 解析
- [src/uart.c](src/uart.c)：`USART1`
- [src/ssd1306.c](src/ssd1306.c)：软件 I2C 和 `SSD1306`
- [src/face_display.c](src/face_display.c)：基础表情绘制
- [startup_stm32h743xx.s](startup_stm32h743xx.s)：启动文件
- [stm32h743zi.ld](stm32h743zi.ld)：链接脚本
- [VERSION](VERSION)：当前版本号
- [.gitignore](.gitignore)：Git 忽略规则

## 改引脚的方法

如果后面你想换引脚，只需要改 [inc/board.h](inc/board.h)：

- 串口脚：`UART_TX_PIN`、`UART_RX_PIN`
- 串口 AF：`UART_AF`
- OLED 脚：`OLED_SCL_PIN`、`OLED_SDA_PIN`

## 说明

这版固件是“当前最小可用链路优先”的实现：

- 不依赖 HAL/CubeMX
- 不依赖外部 JSON 库
- 不依赖外部位图库
- 先保证串口收一行 JSON -> 解析 -> 控屏 -> ACK 可用

如果后面还要继续演进，下一步建议可以加：

- DMA/中断串口接收
- 更严格的 JSON 校验
- CRC 或消息序号
- 屏幕默认表情策略
- 和上位机或其他外设统一协议
