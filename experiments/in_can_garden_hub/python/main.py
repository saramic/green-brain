import json
import os
import queue
import threading
import time

from flask import Flask, Response, jsonify, send_from_directory, stream_with_context
from arduino.app_utils import App, Bridge

WEB_PORT   = 8080
STATIC_DIR = os.path.join(os.path.dirname(__file__), 'static')

nodes       = {}   # keyed by node id
sse_clients = set()
sse_lock    = threading.Lock()

# ── SSE broadcast ─────────────────────────────────────────────────────────────
def sse_push(event_data):
    payload = f"data: {json.dumps(event_data)}\n\n"
    with sse_lock:
        dead = set()
        for q in sse_clients:
            try:
                q.put_nowait(payload)
            except queue.Full:
                dead.add(q)
        sse_clients.difference_update(dead)

# ── Flask web server ──────────────────────────────────────────────────────────
web = Flask(__name__)

@web.route('/')
def index():
    return send_from_directory(STATIC_DIR, 'index.html')

@web.route('/events')
def events():
    q = queue.Queue(maxsize=10)
    with sse_lock:
        sse_clients.add(q)
    # Send current snapshot immediately on connect
    initial = json.dumps({'nodes': list(nodes.values()), 'status': None})

    def generate():
        try:
            yield f"data: {initial}\n\n"
            while True:
                try:
                    yield q.get(timeout=30)
                except queue.Empty:
                    yield ": heartbeat\n\n"   # keep-alive comment
        finally:
            with sse_lock:
                sse_clients.discard(q)

    return Response(
        stream_with_context(generate()),
        content_type='text/event-stream',
        headers={'Cache-Control': 'no-cache', 'X-Accel-Buffering': 'no'},
    )

@web.route('/api/nodes')
def api_nodes():
    return jsonify(list(nodes.values()))

@web.route('/health')
def health():
    return jsonify({'status': 'ok', 'nodes': len(nodes)})

threading.Thread(
    target=lambda: web.run(host='0.0.0.0', port=WEB_PORT, debug=False,
                           use_reloader=False, threaded=True),
    daemon=True,
).start()

# ── Bridge poll loop ──────────────────────────────────────────────────────────
def loop():
    status = None
    try:
        raw  = Bridge.call("get_nodes")
        data = json.loads(raw)
        for node in data:
            node['ts'] = int(time.time() * 1000)
            nodes[node['node']] = node
        status = json.loads(Bridge.call("get_status"))
    except Exception as e:
        print(f"[bridge] {e}")

    sse_push({'nodes': list(nodes.values()), 'status': status})
    time.sleep(2)

print(f"Web UI → http://pollyanna.local:{WEB_PORT}/")
App.run(user_loop=loop)
