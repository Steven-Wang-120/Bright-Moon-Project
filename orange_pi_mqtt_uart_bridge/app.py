from __future__ import annotations

import os
from typing import Any

from flask import Flask, jsonify, render_template, request

from bridge_service import BridgeConfig, BridgeService, load_config
from project_meta import APP_NAME, APP_VERSION


app = Flask(__name__)
service = BridgeService(load_config())


def _parse_topics(raw_topics: str) -> list[str]:
    normalized = raw_topics.replace(",", "\n")
    return [item.strip() for item in normalized.splitlines() if item.strip()]


def _config_from_payload(payload: dict[str, Any]) -> BridgeConfig:
    current = service.config.to_dict()
    current.update(
        {
            "mqtt_host": payload.get("mqtt_host", current["mqtt_host"]),
            "mqtt_port": int(payload.get("mqtt_port", current["mqtt_port"])),
            "mqtt_username": payload.get("mqtt_username", current["mqtt_username"]),
            "mqtt_password": payload.get("mqtt_password", current["mqtt_password"]),
            "mqtt_client_id": payload.get(
                "mqtt_client_id",
                current["mqtt_client_id"],
            ),
            "mqtt_topics": _parse_topics(
                str(payload.get("mqtt_topics", "\n".join(current["mqtt_topics"])))
            ),
            "mqtt_qos": int(payload.get("mqtt_qos", current["mqtt_qos"])),
            "serial_port": payload.get("serial_port", current["serial_port"]),
            "baudrate": int(payload.get("baudrate", current["baudrate"])),
            "write_newline": bool(payload.get("write_newline")),
            "auto_start": bool(payload.get("auto_start")),
            "web_host": payload.get("web_host", current["web_host"]),
            "web_port": int(payload.get("web_port", current["web_port"])),
        }
    )
    return BridgeConfig.from_dict(current)


@app.get("/")
def index():
    return render_template(
        "index.html",
        status=service.snapshot(),
        config=service.config,
        app_name=APP_NAME,
        app_version=APP_VERSION,
    )


@app.get("/api/status")
def api_status():
    return jsonify(service.snapshot())


@app.get("/api/logs")
def api_logs():
    limit = int(request.args.get("limit", "200"))
    return jsonify({"logs": service.logs(limit=limit)})


@app.get("/api/ports")
def api_ports():
    return jsonify({"ports": service.list_serial_ports()})


@app.post("/api/config")
def api_config():
    payload = request.get_json(silent=True) or request.form.to_dict()
    config = _config_from_payload(payload)
    service.update_config(config)
    if payload.get("restart_after_save"):
        service.restart()
    return jsonify({"ok": True, "config": config.to_dict()})


@app.post("/api/start")
def api_start():
    try:
        service.start()
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500
    return jsonify({"ok": True, "status": service.snapshot()})


@app.post("/api/stop")
def api_stop():
    try:
        service.stop()
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500
    return jsonify({"ok": True, "status": service.snapshot()})


@app.post("/api/restart")
def api_restart():
    try:
        service.restart()
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500
    return jsonify({"ok": True, "status": service.snapshot()})


@app.post("/api/send_uart")
def api_send_uart():
    payload = request.get_json(silent=True) or {}
    text = str(payload.get("payload", "")).strip()
    topic = str(payload.get("topic", "manual/uplink")).strip() or "manual/uplink"
    if not text:
        return jsonify({"ok": False, "error": "payload is required"}), 400
    try:
        service.send_uart_payload(text, topic=topic)
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500
    return jsonify({"ok": True})


@app.post("/api/test_payload")
def api_test_payload():
    payload = request.get_json(silent=True) or {}
    text = str(payload.get("payload", "")).strip()
    topic = str(payload.get("topic", "manual/test")).strip() or "manual/test"
    if not text:
        return jsonify({"ok": False, "error": "payload is required"}), 400
    try:
        service.inject_mqtt_payload(text, topic=topic)
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500
    return jsonify({"ok": True})


if __name__ == "__main__":
    config = service.config
    if config.auto_start:
        service.start()

    host = os.environ.get("BRIDGE_WEB_HOST", config.web_host)
    port = int(os.environ.get("BRIDGE_WEB_PORT", str(config.web_port)))
    app.run(host=host, port=port, debug=False)
