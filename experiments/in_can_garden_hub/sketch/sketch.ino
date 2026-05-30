#include "Arduino_RouterBridge.h"
#include <SPI.h>
#include <mcp2515.h>

// ── Config ────────────────────────────────────────────────────────────────────
#define MCP_CS_PIN  10
#define CAN_CLOCK   MCP_8MHZ
#define MAX_NODES   8

// ── Node storage ──────────────────────────────────────────────────────────────
struct NodeReading {
    int     id;
    float   temp;
    float   hum;
    uint8_t seq;
    bool    ok;
    uint32_t lastSeen;  // millis()
};

MCP2515 mcp2515(MCP_CS_PIN);
NodeReading nodes[MAX_NODES];
int  nodeCount = 0;
bool canReady  = false;

// ── CAN frame decoder ─────────────────────────────────────────────────────────
bool decodeFrame(const can_frame& f, int& id, float& temp, float& hum, uint8_t& seq, bool& ok) {
    if (f.can_dlc != 8) return false;
    const uint8_t* d = f.data;
    uint8_t chk = 0;
    for (int i = 0; i < 7; i++) chk ^= d[i];
    if (chk != d[7]) return false;
    id = (f.can_id & 0x7FF) - 0x100;
    if (id < 1 || id > 255) return false;
    ok   = (d[5] & 0x01) != 0;
    seq  = d[6];
    temp = ok ? ((d[1] << 8) | d[2]) / 10.0f : 0;
    hum  = ok ? ((d[3] << 8) | d[4]) / 10.0f : 0;
    return true;
}

void updateNode(int id, float temp, float hum, uint8_t seq, bool ok) {
    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i].id == id) {
            nodes[i] = { id, temp, hum, seq, ok, millis() };
            return;
        }
    }
    if (nodeCount < MAX_NODES)
        nodes[nodeCount++] = { id, temp, hum, seq, ok, millis() };
}

// ── Bridge handlers (Python polls these) ─────────────────────────────────────
String handle_get_nodes() {
    String r = "[";
    for (int i = 0; i < nodeCount; i++) {
        if (i > 0) r += ",";
        r += "{\"node\":"  + String(nodes[i].id);
        r += ",\"temp\":"  + String(nodes[i].temp, 1);
        r += ",\"hum\":"   + String(nodes[i].hum,  1);
        r += ",\"seq\":"   + String(nodes[i].seq);
        r += ",\"ok\":"    + String(nodes[i].ok ? "true" : "false");
        r += ",\"age\":"   + String((millis() - nodes[i].lastSeen) / 1000);
        r += "}";
    }
    r += "]";
    return r;
}

String handle_get_status() {
    return "{\"canReady\":"  + String(canReady ? "true" : "false")
         + ",\"nodes\":"     + String(nodeCount)
         + ",\"uptime\":"    + String(millis() / 1000)
         + "}";
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    // 3 blinks — firmware alive (active-LOW)
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);  delay(100);
        digitalWrite(LED_BUILTIN, HIGH); delay(100);
    }

    Bridge.begin();
    Monitor.begin();
    Monitor.println("CAN Garden Hub starting");

    Bridge.provide_safe("get_nodes",  handle_get_nodes);
    Bridge.provide_safe("get_status", handle_get_status);

    mcp2515.reset();
    delay(100);

    if (mcp2515.setBitrate(CAN_200KBPS, CAN_CLOCK) != MCP2515::ERROR_OK) {
        Monitor.println("MCP2515 not found — waiting for hardware");
    } else {
        mcp2515.setNormalMode();
        delay(10);
        canReady = true;
        Monitor.println("MCP2515 ready @ 200kbps");
    }
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    // Heartbeat blink every 5s
    static uint32_t lastBlink = 0;
    if (millis() - lastBlink >= 5000) {
        digitalWrite(LED_BUILTIN, LOW);  delay(50);
        digitalWrite(LED_BUILTIN, HIGH);
        lastBlink = millis();
    }

    if (!canReady) return;

    can_frame frame;
    if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
        int id; float temp, hum; uint8_t seq; bool ok;
        if (decodeFrame(frame, id, temp, hum, seq, ok)) {
            updateNode(id, temp, hum, seq, ok);
            Monitor.print("node="); Monitor.print(id);
            Monitor.print(" T=");   Monitor.print(temp, 1);
            Monitor.print(" H=");   Monitor.println(hum, 1);
        }
    }
}
