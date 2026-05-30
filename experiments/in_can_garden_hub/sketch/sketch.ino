#include "Arduino_RouterBridge.h"

// ── CAN backend ───────────────────────────────────────────────────────────────
// Default: WCMCU-230 (SN65HVD230) — 3.3V, no level shifter, uses STM32 FDCAN.
// To use MCP2515 via SPI instead, uncomment the line below AND add
// "autowp-mcp2515 (1.3.1)" to the libraries section in sketch.yaml.
//
// #define USE_MCP2515

#ifdef USE_MCP2515
  #include <SPI.h>
  #include <mcp2515.h>
  #define FRAME_ID(f)   (f).can_id
  #define FRAME_DLC(f)  (f).can_dlc
  #define FRAME_TYPE    can_frame
#else
  #include <zephyr/drivers/can.h>
  #include <zephyr/kernel.h>
  #define FRAME_ID(f)   (f).id
  #define FRAME_DLC(f)  (f).dlc
  #define FRAME_TYPE    can_frame

  /* Get the CAN device from Devicetree.
   * If this fails to compile, try DT_NODELABEL(fdcan1) instead. */
  #define CAN_DEVICE DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))

  // CAN_MSGQ_DEFINE is the CAN-specific wrapper around K_MSGQ_DEFINE
  CAN_MSGQ_DEFINE(can_msgq, 16);
#endif

// ── Config ────────────────────────────────────────────────────────────────────
#ifdef USE_MCP2515
  #define MCP_CS_PIN  10
  #define CAN_CLOCK   MCP_8MHZ
#else
  #define CAN_BITRATE 500000   // 500 kbps — must match sensor nodes (CAN_500KBPS)
#endif

// ── Node storage ──────────────────────────────────────────────────────────────
#define MAX_NODES 8

struct NodeReading {
    int      id;
    float    temp;
    float    hum;
    uint8_t  seq;
    bool     ok;
    uint32_t lastSeen;
};

#ifdef USE_MCP2515
  static MCP2515 mcp2515(MCP_CS_PIN);
#else
//   static const struct device *can_dev;
  static const struct device *can_dev;
#endif

static NodeReading nodes[MAX_NODES];
static int  nodeCount = 0;
static bool canReady  = false;

// ── CAN frame decoder ─────────────────────────────────────────────────────────
static bool decodeFrame(const FRAME_TYPE& f, int& id, float& temp, float& hum, uint8_t& seq, bool& ok) {
    if (FRAME_DLC(f) != 8) return false;
    const uint8_t* d = f.data;
    uint8_t chk = 0;
    for (int i = 0; i < 7; i++) chk ^= d[i];
    if (chk != d[7]) return false;
    id = (FRAME_ID(f) & 0x7FF) - 0x100;
    if (id < 1 || id > 255) return false;
    ok   = (d[5] & 0x01) != 0;
    seq  = d[6];
    temp = ok ? ((d[1] << 8) | d[2]) / 10.0f : 0;
    hum  = ok ? ((d[3] << 8) | d[4]) / 10.0f : 0;
    return true;
}

static void updateNode(int id, float temp, float hum, uint8_t seq, bool ok) {
    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i].id == id) { nodes[i] = { id, temp, hum, seq, ok, (uint32_t)millis() }; return; }
    }
    if (nodeCount < MAX_NODES)
        nodes[nodeCount++] = { id, temp, hum, seq, ok, (uint32_t)millis() };
}

// ── Bridge handlers ───────────────────────────────────────────────────────────
static String handle_get_nodes() {
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

static String handle_get_status() {
#ifdef USE_MCP2515
    String backend = "\"MCP2515\"";
#else
    String backend = "\"WCMCU-230\"";
#endif
    return "{\"canReady\":"  + String(canReady ? "true" : "false")
         + ",\"backend\":"   + backend
         + ",\"nodes\":"     + String(nodeCount)
         + ",\"uptime\":"    + String(millis() / 1000)
         + "}";
}

// ── CAN init helpers ──────────────────────────────────────────────────────────
static bool initCan() {
#ifdef USE_MCP2515
    mcp2515.reset();
    delay(100);
    if (mcp2515.setBitrate(CAN_500KBPS, CAN_CLOCK) != MCP2515::ERROR_OK) {
        Monitor.println("MCP2515 setBitrate failed — check SPI wiring");
        return false;
    }
    mcp2515.setNormalMode();
    delay(10);
    Monitor.println("MCP2515 ready @ 500kbps");
    return true;
#else
    // FDCAN1: CTX → D4 (PA12), CRX → D5 (PA11)
    // Board overlay has zephyr,deferred-init — must call device_init() explicitly.
    can_dev = CAN_DEVICE;
    int initErr = device_init(can_dev);
    if (initErr != 0 && initErr != -EALREADY) {
        Monitor.print("CAN device_init failed: ");
        Monitor.println(initErr);
        return false;
    }
    if (!device_is_ready(can_dev)) {
        Monitor.println("CAN device not ready — check CTX->D4 / CRX->D5");
        return false;
    }
    if (can_set_bitrate(can_dev, CAN_BITRATE) != 0) {
        Monitor.println("CAN set bitrate failed");
        return false;
    }
    if (can_start(can_dev) != 0) {
        Monitor.println("CAN start failed");
        return false;
    }
    // Accept all standard CAN frames (mask=0 matches any ID)
    const struct can_filter accept_all = { .id = 0, .mask = 0, .flags = 0 };
    if (can_add_rx_filter_msgq(can_dev, &can_msgq, &accept_all) < 0) {
        Monitor.println("CAN filter add failed");
        return false;
    }
    Monitor.println("WCMCU-230 (SN65HVD230) ready @ 500kbps");
    return true;
#endif
}

static bool readFrame(FRAME_TYPE& frame) {
#ifdef USE_MCP2515
    return mcp2515.readMessage(&frame) == MCP2515::ERROR_OK;
#else
    return k_msgq_get(&can_msgq, &frame, K_NO_WAIT) == 0;
#endif
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);  delay(100);
        digitalWrite(LED_BUILTIN, HIGH); delay(100);
    }

    Bridge.begin();
    Monitor.begin();

#ifdef USE_MCP2515
    Monitor.println("CAN Garden Hub — backend: MCP2515");
#else
    Monitor.println("CAN Garden Hub — backend: WCMCU-230");
#endif
    // Monitor output requires the arduino-app-cli monitor to be connected.
    // If you see nothing, check: arduino-app-cli app list  and  arduino-app-cli monitor

    Bridge.provide_safe("get_nodes",  handle_get_nodes);
    Bridge.provide_safe("get_status", handle_get_status);

    canReady = initCan();
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    static uint32_t lastBlink = 0;
    if (millis() - lastBlink >= 5000) {
        digitalWrite(LED_BUILTIN, LOW);  delay(50);
        digitalWrite(LED_BUILTIN, HIGH);
        Monitor.print("heartbeat uptime=");
        Monitor.print(millis() / 1000);
        Monitor.print("s canReady=");
        Monitor.print(canReady ? "true" : "false");
        Monitor.print(" nodes=");
        Monitor.println(nodeCount);
        lastBlink = millis();
    }

    if (!canReady) return;

    FRAME_TYPE frame;
    if (readFrame(frame)) {
        int id; float temp, hum; uint8_t seq; bool ok;
        if (decodeFrame(frame, id, temp, hum, seq, ok)) {
            updateNode(id, temp, hum, seq, ok);
            Monitor.print("node="); Monitor.print(id);
            Monitor.print(" T=");   Monitor.print(temp, 1);
            Monitor.print(" H=");   Monitor.println(hum, 1);
        }
    }
}
