import os
import queue
import sys
import threading
import time

from flask import Flask, jsonify, request, send_from_directory
from arduino.app_utils import App, Bridge

COMMAND_FILE = '/tmp/led_cmd'
WEB_PORT     = 8080

counter  = 0
led_state = False
cmd_queue = queue.Queue()

# ── Flask web server ──────────────────────────────────────────────────────────
web = Flask(__name__)

STATIC_DIR = os.path.join(os.path.dirname(__file__), 'static')

@web.route('/')
def index():
    return send_from_directory(STATIC_DIR, 'index.html')

@web.route('/status')
def status():
    return jsonify({'tick': counter, 'led': 'ON' if led_state else 'OFF'})

@web.route('/led', methods=['POST'])
def set_led():
    cmd = (request.get_json() or {}).get('state', '').upper()
    if cmd in ('ON', 'OFF'):
        cmd_queue.put(cmd)
    return jsonify({'ok': True, 'led': cmd})

threading.Thread(
    target=lambda: web.run(host='0.0.0.0', port=WEB_PORT, debug=False, use_reloader=False),
    daemon=True
).start()

# ── File watcher (docker exec fallback) ──────────────────────────────────────
def watch_file():
    while True:
        if os.path.exists(COMMAND_FILE):
            try:
                cmd = open(COMMAND_FILE).read().strip().upper()
                os.remove(COMMAND_FILE)
                if cmd in ('ON', 'OFF'):
                    cmd_queue.put(cmd)
            except Exception:
                pass
        time.sleep(0.1)

threading.Thread(target=watch_file, daemon=True).start()

# ── Bridge loop ───────────────────────────────────────────────────────────────
def loop():
    global counter, led_state

    counter += 1
    Bridge.call("print_value", counter)

    # Poll queue every 100ms for up to 2s so LED responds quickly
    for _ in range(20):
        while not cmd_queue.empty():
            cmd = cmd_queue.get()
            led_state = (cmd == 'ON')
            Bridge.call("set_led", cmd)
            print(f"LED: {cmd}")
        time.sleep(0.1)

print(f"Web UI → http://pollyanna.local:{WEB_PORT}/")
App.run(user_loop=loop)
