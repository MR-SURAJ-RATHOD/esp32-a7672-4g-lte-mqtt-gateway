#!/usr/bin/env python3
"""
Subscribe to your MQTT topic and print incoming telemetry (development helper).

Usage:
  pip install paho-mqtt
  set MQTT_BROKER=your.broker.host
  set MQTT_USER=your_user
  set MQTT_PASS=your_pass
  set MQTT_TOPIC=your/topic
  python verify_mqtt.py [DEVICE_ID]

Or pass env vars inline (Linux/macOS):
  MQTT_BROKER=192.168.1.1 MQTT_TOPIC=devices/telemetry python verify_mqtt.py GW-001
"""

import json
import os
import sys
import time

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Install: pip install paho-mqtt")
    sys.exit(1)

BROKER = os.environ.get("MQTT_BROKER", "YOUR_MQTT_BROKER_HOST")
PORT = int(os.environ.get("MQTT_PORT", "1883"))
USER = os.environ.get("MQTT_USER", "YOUR_MQTT_USERNAME")
PASS = os.environ.get("MQTT_PASS", "YOUR_MQTT_PASSWORD")
TOPIC = os.environ.get("MQTT_TOPIC", "YOUR_MQTT_PUBLISH_TOPIC")
WAIT_SEC = int(os.environ.get("MQTT_WAIT_SEC", "70"))


def on_connect(client, userdata, flags, rc):
    print(f"Connected rc={rc}, subscribing to {TOPIC}")
    client.subscribe(TOPIC)


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
        device_id = payload.get("ID", "?")
        dt = payload.get("DT", "?")
    except Exception as exc:
        device_id = "parse_error"
        dt = str(exc)

    print(
        f"[{time.strftime('%H:%M:%S')}] topic={msg.topic} retain={msg.retain} "
        f"ID={device_id} DT={dt} bytes={len(msg.payload)}"
    )
    print(msg.payload.decode("utf-8", "replace")[:400])
    print("-" * 60)


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "YOUR_DEVICE_ID"
    print(f"Broker={BROKER}:{PORT} topic={TOPIC}")
    print(f"Waiting {WAIT_SEC}s (filtering log for ID={target})...")

    client = mqtt.Client(client_id="verify-mqtt-subscriber")
    if USER and USER != "YOUR_MQTT_USERNAME":
        client.username_pw_set(USER, PASS)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    time.sleep(WAIT_SEC)
    client.loop_stop()
    print("Done.")


if __name__ == "__main__":
    main()
