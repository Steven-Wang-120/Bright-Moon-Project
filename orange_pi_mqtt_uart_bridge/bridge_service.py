from __future__ import annotations

import json
import threading
import time
from collections import deque
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

import paho.mqtt.client as mqtt
import serial
from serial.tools import list_ports

from project_meta import APP_NAME, APP_VERSION


CONFIG_PATH = Path(__file__).with_name("config.json")


@dataclass
class BridgeConfig:
    mqtt_host: str = "127.0.0.1"
    mqtt_port: int = 1883
    mqtt_username: str = ""
    mqtt_password: str = ""
    mqtt_client_id: str = "orange-pi-mqtt-uart-bridge"
    mqtt_topics: list[str] = field(default_factory=lambda: ["test"])
    mqtt_qos: int = 0
    serial_port: str = "/dev/ttyS1"
    baudrate: int = 115200
    write_newline: bool = True
    auto_start: bool = False
    web_host: str = "0.0.0.0"
    web_port: int = 8080

    @classmethod
    def from_dict(cls, raw: dict[str, Any]) -> "BridgeConfig":
        topics = raw.get("mqtt_topics", ["test"])
        if isinstance(topics, str):
            topics = [item.strip() for item in topics.replace(",", "\n").splitlines()]
        topics = [str(item).strip() for item in topics if str(item).strip()]
        return cls(
            mqtt_host=str(raw.get("mqtt_host", cls.mqtt_host)),
            mqtt_port=int(raw.get("mqtt_port", cls.mqtt_port)),
            mqtt_username=str(raw.get("mqtt_username", cls.mqtt_username)),
            mqtt_password=str(raw.get("mqtt_password", cls.mqtt_password)),
            mqtt_client_id=str(raw.get("mqtt_client_id", cls.mqtt_client_id)),
            mqtt_topics=topics or ["test"],
            mqtt_qos=max(0, min(2, int(raw.get("mqtt_qos", cls.mqtt_qos)))),
            serial_port=str(raw.get("serial_port", cls.serial_port)),
            baudrate=int(raw.get("baudrate", cls.baudrate)),
            write_newline=bool(raw.get("write_newline", cls.write_newline)),
            auto_start=bool(raw.get("auto_start", cls.auto_start)),
            web_host=str(raw.get("web_host", cls.web_host)),
            web_port=int(raw.get("web_port", cls.web_port)),
        )

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def load_config() -> BridgeConfig:
    if not CONFIG_PATH.exists():
        config = BridgeConfig()
        save_config(config)
        return config
    return BridgeConfig.from_dict(json.loads(CONFIG_PATH.read_text(encoding="utf-8")))


def save_config(config: BridgeConfig) -> None:
    CONFIG_PATH.write_text(
        json.dumps(config.to_dict(), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


class BridgeService:
    def __init__(self, config: BridgeConfig) -> None:
        self._config = config
        self._lock = threading.RLock()
        self._logs: deque[dict[str, Any]] = deque(maxlen=500)
        self._serial_conn: serial.Serial | None = None
        self._serial_thread: threading.Thread | None = None
        self._stop_event = threading.Event()
        self._mqtt_client: mqtt.Client | None = None
        self._running = False
        self._mqtt_connected = False
        self._started_at = 0.0
        self._last_error = ""
        self._rx_count = 0
        self._tx_count = 0
        self._ack_count = 0
        self._last_topic = ""
        self._last_payload = ""
        self._last_ack = ""

    @property
    def config(self) -> BridgeConfig:
        with self._lock:
            return BridgeConfig.from_dict(self._config.to_dict())

    def update_config(self, config: BridgeConfig) -> None:
        with self._lock:
            self._config = config
            save_config(config)

    def logs(self, limit: int = 200) -> list[dict[str, Any]]:
        with self._lock:
            items = list(self._logs)
        return items[-limit:]

    def snapshot(self) -> dict[str, Any]:
        with self._lock:
            return {
                "name": APP_NAME,
                "version": APP_VERSION,
                "running": self._running,
                "mqtt_connected": self._mqtt_connected,
                "started_at": self._started_at,
                "uptime_seconds": max(0.0, time.time() - self._started_at)
                if self._started_at
                else 0.0,
                "last_error": self._last_error,
                "rx_count": self._rx_count,
                "tx_count": self._tx_count,
                "ack_count": self._ack_count,
                "last_topic": self._last_topic,
                "last_payload": self._last_payload,
                "last_ack": self._last_ack,
                "config": self._config.to_dict(),
            }

    def start(self) -> None:
        with self._lock:
            if self._running:
                return

            self._last_error = ""
            self._stop_event.clear()
            try:
                self._open_serial_locked()
                self._serial_thread = threading.Thread(
                    target=self._serial_reader_loop,
                    name="serial-reader",
                    daemon=True,
                )
                self._serial_thread.start()

                self._mqtt_client = self._build_mqtt_client_locked()
                self._mqtt_client.connect_async(
                    self._config.mqtt_host,
                    self._config.mqtt_port,
                    keepalive=60,
                )
                self._mqtt_client.loop_start()

                self._running = True
                self._started_at = time.time()
                self._log("bridge", "service started")
            except Exception as exc:
                self._last_error = str(exc)
                self._log("bridge", f"start failed: {exc}")
                serial_conn = self._serial_conn
                self._serial_conn = None
                if serial_conn is not None:
                    try:
                        serial_conn.close()
                    except Exception:
                        pass
                self._mqtt_client = None
                self._serial_thread = None
                self._running = False
                self._started_at = 0.0
                raise

    def stop(self) -> None:
        with self._lock:
            if not self._running:
                return
            self._stop_event.set()

            client = self._mqtt_client
            self._mqtt_client = None
            if client is not None:
                try:
                    client.disconnect()
                except Exception:
                    pass
                try:
                    client.loop_stop()
                except Exception:
                    pass

            serial_conn = self._serial_conn
            self._serial_conn = None
            if serial_conn is not None:
                try:
                    serial_conn.close()
                except Exception:
                    pass

            self._mqtt_connected = False
            self._running = False
            self._started_at = 0.0
            self._log("bridge", "service stopped")

        if self._serial_thread is not None:
            self._serial_thread.join(timeout=1.0)
            self._serial_thread = None

    def restart(self) -> None:
        self.stop()
        self.start()

    def send_uart_payload(self, payload_text: str, topic: str = "manual") -> None:
        with self._lock:
            if self._serial_conn is None or not self._serial_conn.is_open:
                raise RuntimeError("serial port is not open")
            wire_text = payload_text
            if self._config.write_newline and not wire_text.endswith("\n"):
                wire_text += "\n"
            self._serial_conn.write(wire_text.encode("utf-8"))
            self._serial_conn.flush()
            self._tx_count += 1
            self._last_topic = topic
            self._last_payload = payload_text
            self._log("uart_tx", f"{topic} -> {payload_text}")

    def inject_mqtt_payload(self, payload_text: str, topic: str = "manual/test") -> None:
        self._handle_payload(topic, payload_text.encode("utf-8"))

    def list_serial_ports(self) -> list[dict[str, str]]:
        ports: list[dict[str, str]] = []
        for port in list_ports.comports():
            ports.append(
                {
                    "device": port.device,
                    "description": port.description or "",
                }
            )
        return ports

    def _build_mqtt_client_locked(self) -> mqtt.Client:
        try:
            client = mqtt.Client(
                mqtt.CallbackAPIVersion.VERSION2,
                client_id=self._config.mqtt_client_id,
            )
        except AttributeError:
            client = mqtt.Client(client_id=self._config.mqtt_client_id)

        if self._config.mqtt_username:
            client.username_pw_set(
                self._config.mqtt_username,
                self._config.mqtt_password or None,
            )

        client.on_connect = self._on_mqtt_connect
        client.on_disconnect = self._on_mqtt_disconnect
        client.on_message = self._on_mqtt_message
        return client

    def _open_serial_locked(self) -> None:
        self._serial_conn = serial.Serial(
            port=self._config.serial_port,
            baudrate=self._config.baudrate,
            timeout=0.2,
            write_timeout=1.0,
        )
        self._log(
            "serial",
            f"opened {self._config.serial_port} @ {self._config.baudrate}",
        )

    def _serial_reader_loop(self) -> None:
        buffer = bytearray()
        while not self._stop_event.is_set():
            with self._lock:
                serial_conn = self._serial_conn
            if serial_conn is None:
                break

            try:
                chunk = serial_conn.read(64)
            except Exception as exc:
                with self._lock:
                    self._last_error = f"serial read failed: {exc}"
                    self._log("serial", self._last_error)
                time.sleep(0.3)
                continue

            if not chunk:
                continue

            buffer.extend(chunk)
            while b"\n" in buffer:
                raw_line, _, remaining = buffer.partition(b"\n")
                buffer = bytearray(remaining)
                line = raw_line.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                with self._lock:
                    self._ack_count += 1
                    self._last_ack = line
                    self._log("uart_rx", line)

    def _on_mqtt_connect(
        self,
        client: mqtt.Client,
        _userdata: Any,
        _flags: Any,
        reason_code: Any,
        _properties: Any = None,
    ) -> None:
        with self._lock:
            try:
                connected = int(reason_code) == 0
            except Exception:
                connected = (reason_code == 0)
            self._mqtt_connected = connected
            if not connected:
                self._last_error = f"mqtt connect failed: {reason_code}"
                self._log("mqtt", self._last_error)
                return
            for topic in self._config.mqtt_topics:
                client.subscribe(topic, qos=self._config.mqtt_qos)
                self._log(
                    "mqtt",
                    f"subscribed topic={topic} qos={self._config.mqtt_qos}",
                )
            self._log(
                "mqtt",
                f"connected {self._config.mqtt_host}:{self._config.mqtt_port}",
            )

    def _on_mqtt_disconnect(self, _client: mqtt.Client, *_args: Any) -> None:
        with self._lock:
            self._mqtt_connected = False
            self._log("mqtt", "disconnected")

    def _on_mqtt_message(
        self,
        _client: mqtt.Client,
        _userdata: Any,
        msg: mqtt.MQTTMessage,
    ) -> None:
        self._handle_payload(msg.topic, msg.payload)

    def _handle_payload(self, topic: str, payload: bytes) -> None:
        payload_text = payload.decode("utf-8", errors="replace").strip()
        with self._lock:
            self._rx_count += 1
            self._last_topic = topic
            self._last_payload = payload_text
            self._log("mqtt_rx", f"{topic} <- {payload_text}")

        if not payload_text:
            self._log("mqtt_rx", "ignored empty payload")
            return

        try:
            self.send_uart_payload(payload_text, topic=topic)
        except Exception as exc:
            with self._lock:
                self._last_error = f"uart send failed: {exc}"
                self._log("bridge", self._last_error)

    def _log(self, category: str, message: str) -> None:
        self._logs.append(
            {
                "ts": time.strftime("%Y-%m-%d %H:%M:%S"),
                "category": category,
                "message": message,
            }
        )
