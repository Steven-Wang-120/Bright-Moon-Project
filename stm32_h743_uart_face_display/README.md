# STM32H743 UART Face Display

当前版本：`v0.2.0`

这个目录对应当前的 `H743` 主线固件：

- 保留新的串口链路：`Orange Pi -> USART1(PA9/PA10) -> STM32H743`
- 保留新的 OLED 引脚：`PC8/PC9`
- 保留新的协议：`一行一个 JSON，以 \n 结束`
- 显示逻辑改为“表情图片编译进 STM32H743 程序 Flash”

这版不再使用手画表情兜底。

## 目标链路

```text
AstrBot 插件 -> MQTT -> Orange Pi MQTT-UART Bridge -> USART1 -> STM32H743ZIP6 -> SSD1306
```

## 当前实现

- MCU: `STM32H743ZIP6`
- 工具链: `arm-none-eabi-gcc`
- 通信: `USART1`
- 屏幕: `SSD1306 128x64`
- 屏幕总线: 软件 `I2C`
- 协议: `一行一个 JSON，以 \n 结束`
- 表情资源: 本地图片转换成 `const uint8_t[]`，随固件一起编译进 MCU Flash

## 引脚

- `USART1_TX = PA9`
- `USART1_RX = PA10`
- `OLED_SCL = PC8`
- `OLED_SDA = PC9`

对应配置在 [inc/board.h](inc/board.h)。

## 支持的 JSON

基础格式：

```json
{"face":"happy","intensity":65}
```

扩展格式：

```json
{"cmd":"show","emo":"thinking","intensity":70,"dur":3000}
```

其他命令：

```json
{"cmd":"clear"}
```

```json
{"cmd":"ping"}
```

## 串口行为

固件按行接收，`\r` 忽略，`\n` 结束一帧。

成功时返回：

```text
ACK show happy 65
ACK show thinking 70 3000
ACK clear
ACK ping
```

解析失败时返回：

```text
ERR missing face/emo
ERR unsupported face
ERR line too long
```

如果消息里带 `dur`，到时后会自动恢复 `neutral 50`：

```text
ACK revert neutral 50
```

上电启动时先回：

```text
READY stm32_h743_uart_face_display v0.2.0 json-line stm32h743 uart1 pa9-pa10 oled pc8-pc9 flash-assets
```

## 表情资源模型

这版不从 SD 卡读图，也不在运行时访问文件系统。

流程是：

1. 把表情图片放进 [assets](assets/README.md) 目录
2. 运行 [tools/generate_face_assets.py](tools/generate_face_assets.py)
3. 脚本生成：
   - [inc/generated_face_assets.h](inc/generated_face_assets.h)
   - [src/generated_face_assets.c](src/generated_face_assets.c)
4. `make` 时把这些位图和固件一起编译进 STM32H743 的程序 Flash

运行时，固件会按 `face + intensity` 在编译进 Flash 的资源表里找最接近的一张图显示。

如果当前没有对应资源，屏幕会清空，不再回退到手画表情。

## 资源目录

资源目录结构见 [assets/README.md](assets/README.md)。

支持的表情目录名固定为：

- `happy`
- `sad`
- `angry`
- `surprised`
- `thinking`
- `neutral`

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
- 当前 OLED 使用软件 I2C，不依赖硬件 I2C 外设

## 生成资源

先安装 `Pillow`：

```bash
cd stm32_h743_uart_face_display
python -m pip install Pillow
```

然后生成资源代码：

```bash
python tools/generate_face_assets.py
```

如果还没有准备任何图片资源，工程也能编译；只是屏幕不会显示表情图。

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

默认使用：

- `interface/stlink.cfg`
- `target/stm32h7x.cfg`

如果你用的是 CubeProgrammer，也可以直接把 `build/stm32_h743_uart_face_display.bin` 烧到 `0x08000000`。

## 工程结构

- [inc/board.h](inc/board.h)：板级引脚和波特率
- [inc/mcu.h](inc/mcu.h)：最小寄存器定义
- [inc/version.h](inc/version.h)：固件名和版本号
- [inc/face_assets.h](inc/face_assets.h)：资源查找接口
- [inc/generated_face_assets.h](inc/generated_face_assets.h)：生成后的资源声明
- [src/main.c](src/main.c)：主循环、JSON 处理、ACK
- [src/json_protocol.c](src/json_protocol.c)：轻量 JSON 解析
- [src/uart.c](src/uart.c)：`USART1`
- [src/ssd1306.c](src/ssd1306.c)：软件 I2C、SSD1306、整屏位图加载
- [src/face_assets.c](src/face_assets.c)：按强度选择最接近资源
- [src/generated_face_assets.c](src/generated_face_assets.c)：生成后的位图资源
- [src/face_display.c](src/face_display.c)：显示入口
- [tools/generate_face_assets.py](tools/generate_face_assets.py)：图片转位图脚本
- [assets/README.md](assets/README.md)：资源目录约定

## 改引脚的方法

如果后面要改引脚，只需要改 [inc/board.h](inc/board.h)：

- 串口脚：`UART_TX_PIN`、`UART_RX_PIN`
- 串口 AF：`UART_AF`
- OLED 脚：`OLED_SCL_PIN`、`OLED_SDA_PIN`

## 说明

这版固件的核心定位是：

- 新主线串口和 JSON 协议不变
- 新主线 `PC8/PC9` OLED 引脚不变
- 旧图片资源编译进 Flash 的显示模型迁移到 `H743`
- 不使用 SD 卡
- 不使用手画表情兜底
