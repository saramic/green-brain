import json
import os
import threading
import time

from flask import Flask, jsonify, send_from_directory
from arduino.app_utils import App, Bridge

WEB_PORT   = 8080
STATIC_DIR = os.path.join(os.path.dirname(__file__), 'static')

nodes = {}   # keyed by node id, updated each poll cycle

# ── Flask web server ──────────────────────────────────────────────────────────
web = Flask(__name__)

@web.route('/')
def index():
    return send_from_directory(STATIC_DIR, 'index.html')

@web.route('/api/nodes')
def api_nodes():
    return jsonify(list(nodes.values()))

@web.route('/api/status')
def api_status():
    try:
        return jsonify(json.loads(Bridge.call("get_status")))
    except Exception as e:
        return jsonify({'error': str(e)}), 503

@web.route('/health')
def health():
    return jsonify({'status': 'ok', 'nodes': len(nodes)})

threading.Thread(
    target=lambda: web.run(host='0.0.0.0', port=WEB_PORT, debug=False, use_reloader=False),
    daemon=True
).start()

# ── Bridge poll loop ──────────────────────────────────────────────────────────
def loop():
    try:
        raw  = Bridge.call("get_nodes")
        data = json.loads(raw)
        for node in data:
            node['ts'] = int(time.time() * 1000)
            nodes[node['node']] = node
    except Exception as e:
        print(f"[bridge] {e}")

    time.sleep(2)

print(f"Web UI → http://pollyanna.local:{WEB_PORT}/")
App.run(user_loop=loop)
