# Orange Pi MQTT UART Bridge

当前版本：`v0.1.0`

这个目录提供一套可以直接部署到 Linux 香橙派上的 `MQTT -> UART` 薄桥。

它做的事情很简单：

- 订阅你指定的一个或多个 MQTT topic
- 收到 AstrBot MQTT 插件发来的 JSON payload 后，不改消息结构
- 只在串口侧补一个换行符 `\n`
- 原样发送给 STM32
- 同时提供一个本地 Web 控制台，方便人工改配置、启停服务、看日志、做测试

## GitHub 上传状态

这个目录已经整理成适合直接上传 GitHub 的形式：

- 运行期敏感配置使用 `config.json`，并已写入 `.gitignore`
- 仓库里只保留 `config.example.json`
- Python 虚拟环境、缓存文件、日志文件均已忽略
- 版本号写在 [VERSION](VERSION) 和 [project_meta.py](project_meta.py)

## 适用消息格式

桥程序不会把消息从 JSON 改成别的格式，推荐直接传：

```json
{"face":"happy","intensity":65}
```

也兼容你后续可能扩展的格式：

```json
{"cmd":"show","emo":"thinking","dur":3000}
```

桥程序会把它原样发到串口：

```text
{"face":"happy","intensity":65}\n
```

## 目录说明

- [app.py](app.py)：Flask Web 服务入口
- [bridge_service.py](bridge_service.py)：MQTT、串口、配置和日志核心逻辑
- [project_meta.py](project_meta.py)：项目名和版本号
- [templates/index.html](templates/index.html)：Web 控制台页面
- [static/style.css](static/style.css)：页面样式
- [requirements.txt](requirements.txt)：Python 依赖
- [config.example.json](config.example.json)：配置示例
- [orange_pi_mqtt_uart_bridge.service.example](orange_pi_mqtt_uart_bridge.service.example)：`systemd` 服务示例
- [VERSION](VERSION)：当前版本号
- [.gitignore](.gitignore)：Git 忽略规则

## 安装

在香橙派上准备 Python 3：

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip
```

进入目录并安装依赖：

```bash
cd orange_pi_mqtt_uart_bridge
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

首次运行前复制一份配置：

```bash
cp config.example.json config.json
```

## 启动

```bash
source .venv/bin/activate
python3 app.py
```

默认 Web 页面地址：

```text
http://<orange-pi-ip>:8080
```

你也可以通过环境变量覆盖 Web 监听地址：

```bash
BRIDGE_WEB_HOST=0.0.0.0 BRIDGE_WEB_PORT=8080 python3 app.py
```

## Web 控制台功能

- 修改 MQTT Host、Port、用户名、密码、Client ID
- 自由填写订阅 topic，支持一行一个，也支持逗号分隔
- 修改串口设备名和波特率
- 一键启动、停止、重启桥服务
- 实时查看 MQTT 收包、UART 发包、STM32 ACK
- 可以手动注入测试 payload
- 可以不走 MQTT，直接手动向串口发送一行 JSON

## 串口建议

常见香橙派串口设备名可能是：

- `/dev/ttyS1`
- `/dev/ttyS2`
- `/dev/ttyAMA0`

启动前建议先确认：

```bash
ls /dev/ttyS* /dev/ttyAMA* 2>/dev/null
```

如果当前用户没有串口权限，可以加入 `dialout`：

```bash
sudo usermod -aG dialout $USER
```

然后重新登录。

## 和 STM32 的接线

最少两线：

- 香橙派 `TX` -> STM32 `RX`
- 香橙派 `GND` -> STM32 `GND`

推荐三线：

- 香橙派 `TX` -> STM32 `RX`
- 香橙派 `RX` -> STM32 `TX`
- 香橙派 `GND` -> STM32 `GND`

电平必须是 `3.3V TTL`。

## systemd 常驻运行

把目录放到例如 `/opt/orange_pi_mqtt_uart_bridge` 后：

```bash
sudo cp orange_pi_mqtt_uart_bridge.service.example /etc/systemd/system/orange-pi-mqtt-uart-bridge.service
sudo systemctl daemon-reload
sudo systemctl enable --now orange-pi-mqtt-uart-bridge.service
```

查看日志：

```bash
journalctl -u orange-pi-mqtt-uart-bridge.service -f
```

## 设计边界

这不是一个复杂协议网关。它故意只做很薄的一层：

- 不改 AstrBot 插件发出的 JSON
- 不做业务字段重写
- 只负责 MQTT 订阅、串口发送、日志和人工操作界面

如果你后面改 topic、改串口、加 ACK，这一层都还能继续用。
