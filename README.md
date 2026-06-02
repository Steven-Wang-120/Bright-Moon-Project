# Bright-Moon-Project

这个仓库当前包含两套和 `astrbot_plugin_mqtt` 配合使用的新工程：

- [orange_pi_mqtt_uart_bridge](orange_pi_mqtt_uart_bridge/README.md)
  Orange Pi / Linux 上运行的 `MQTT -> UART` 薄桥，保留 MQTT JSON 原消息，并提供 Web 控制台。
- [stm32_h743_uart_face_display](stm32_h743_uart_face_display/README.md)
  面向 `STM32H743ZIP6` 的最小自包含固件工程，按行接收 JSON，解析后驱动 SSD1306 屏幕显示表情。

## 目标链路

```text
AstrBot + astrbot_plugin_mqtt
  -> MQTT JSON
  -> Orange Pi MQTT-UART Bridge
  -> UART
  -> STM32H743ZIP6
  -> SSD1306
```

## 当前版本

- `orange_pi_mqtt_uart_bridge`: `v0.1.0`
- `stm32_h743_uart_face_display`: `v0.1.0`
