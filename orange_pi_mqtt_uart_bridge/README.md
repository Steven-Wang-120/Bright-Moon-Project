# Orange Pi MQTT UART Bridge

当前版本：`v0.1.0`

这个目录提供一套部署在 Linux 香橙派上的 `MQTT -> UART` 薄桥。

它只做几件事：

- 订阅指定的一个或多个 MQTT topic
- 收到 AstrBot MQTT 插件发来的 JSON payload 后，不改消息结构
- 只在串口侧补一个换行符 `\n`
- 原样发送给 STM32
- 提供本地 Web 控制台，方便改配置、启停服务、看日志、做测试

## 适用消息格式

推荐直接传：

```json
{"face":"happy","intensity":65}
```

也兼容扩展格式：

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
- [Dockerfile](Dockerfile)：容器镜像构建文件
- [.dockerignore](.dockerignore)：容器构建忽略规则
- [orange_pi_mqtt_uart_bridge.service.example](orange_pi_mqtt_uart_bridge.service.example)：`systemd` 服务示例
- [VERSION](VERSION)：当前版本号
- [.gitignore](.gitignore)：Git 忽略规则

## 本地 Python 运行

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

启动：

```bash
source .venv/bin/activate
python3 app.py
```

默认 Web 页面地址：

```text
http://<orange-pi-ip>:8080
```

也可以通过环境变量覆盖 Web 监听地址：

```bash
BRIDGE_WEB_HOST=0.0.0.0 BRIDGE_WEB_PORT=8080 python3 app.py
```

## Docker 镜像构建

在本目录执行：

```bash
docker build -t orange-pi-mqtt-uart-bridge:0.1.0 .
```

镜像默认：

- 监听 `0.0.0.0:8080`
- 启动命令是 `python app.py`
- 健康检查访问 `http://127.0.0.1:8080/api/status`

## Docker 运行

先准备运行配置：

```bash
cp config.example.json config.json
```

然后按实际串口设备启动，例如：

```bash
docker run -d \
  --name orange-pi-mqtt-uart-bridge \
  --restart unless-stopped \
  -p 8080:8080 \
  --device /dev/ttyS1:/dev/ttyS1 \
  -v "$(pwd)/config.json:/app/config.json" \
  orange-pi-mqtt-uart-bridge:0.1.0
```

如果你的设备不是 `/dev/ttyS1`，把上面的设备映射和 `config.json` 里的 `serial_port` 一起改掉，例如 `/dev/ttyS2` 或 `/dev/ttyAMA0`。

容器化运行时要点：

- `config.json` 挂载到 `/app/config.json`
- 容器内串口路径必须和 `config.json.serial_port` 一致
- Web 控制台默认端口是 `8080`
- 当前镜像按 root 运行，避免串口权限在容器内再次卡住

## 后续 docker compose 设计约束

后面如果写 `compose.yaml`，至少要保留这几项：

- 端口映射：`8080:8080`
- 串口设备映射：如 `/dev/ttyS1:/dev/ttyS1`
- 配置文件挂载：`./config.json:/app/config.json`
- 重启策略：`unless-stopped`

也就是说，后续 compose 只是在 `docker run` 这组参数上做结构化展开，不需要改 Python 业务代码。

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
