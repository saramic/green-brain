#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>
#include <DHT.h>

#define DHT_PIN    4
#define DHT_TYPE   DHT11
#define MCP_CS_PIN 10
#define NODE_ID    1
#define CAN_CLOCK  MCP_8MHZ

// Comment out to switch to normal (live bus) mode once a second node is present.
// #define LOOPBACK_TEST

DHT dht(DHT_PIN, DHT_TYPE);
MCP2515 mcp2515(MCP_CS_PIN);

uint8_t sequence = 0;

// // Read CANSTAT register directly to verify chip operating mode.
// // MCP2515 opmode bits [7:5]: 0x00=Normal 0x20=Sleep 0x40=Loopback
// //                            0x60=Listen-only 0x80=Config
// uint8_t readCanstat() {
//     SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
//     digitalWrite(MCP_CS_PIN, LOW);
//     SPI.transfer(0x03); // READ instruction
//     SPI.transfer(0x0E); // CANSTAT register address
//     uint8_t val = SPI.transfer(0x00);
//     digitalWrite(MCP_CS_PIN, HIGH);
//     SPI.endTransaction();
//     return val;
// }

can_frame buildFrame(float temp, float hum, bool sensorOk) {
    can_frame frame;
    frame.can_id  = 0x100 + NODE_ID;
    frame.can_dlc = 8;

    uint16_t tempRaw = sensorOk ? (uint16_t)(temp * 10) : 0;
    uint16_t humRaw  = sensorOk ? (uint16_t)(hum  * 10) : 0;

    frame.data[0] = NODE_ID;
    frame.data[1] = (tempRaw >> 8) & 0xFF;
    frame.data[2] =  tempRaw       & 0xFF;
    frame.data[3] = (humRaw  >> 8) & 0xFF;
    frame.data[4] =  humRaw        & 0xFF;
    frame.data[5] = (sensorOk ? 0x01 : 0x00) | 0x02;
    frame.data[6] = sequence++;

    uint8_t chk = 0;
    for (int i = 0; i < 7; i++) chk ^= frame.data[i];
    frame.data[7] = chk;

    return frame;
}

void printFrame(const char* prefix, const can_frame& f) {
    Serial.print(prefix);
    Serial.print(F("ID:0x")); Serial.print(f.can_id, HEX);
    Serial.print(F(" node:")); Serial.print(f.data[0]);
    float t = ((uint16_t)(f.data[1] << 8) | f.data[2]) / 10.0;
    float h = ((uint16_t)(f.data[3] << 8) | f.data[4]) / 10.0;
    Serial.print(F(" T:")); Serial.print(t, 1); Serial.print(F("C"));
    Serial.print(F(" H:")); Serial.print(h, 1); Serial.print(F("%"));
    Serial.print(F(" seq:")); Serial.print(f.data[6]);
    Serial.print(F(" chk:0x")); Serial.println(f.data[7], HEX);
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    dht.begin();

    Serial.println(F("GreenBrain | CAN Garden Node | Phase 2"));
    Serial.print(F("Node ID : ")); Serial.println(NODE_ID);
#ifdef LOOPBACK_TEST
    Serial.println(F("Mode    : LOOPBACK"));
#else
    Serial.println(F("Mode    : NORMAL"));
#endif

    mcp2515.reset();
    delay(100);

    // 500kbps is marginal at 8MHz (only 8 time-quanta). 200kbps is reliable.
    if (mcp2515.setBitrate(CAN_500KBPS, CAN_CLOCK) != MCP2515::ERROR_OK) {
        Serial.println(F("ERR: setBitrate failed — check SPI wiring"));
        while (true);
    }
    Serial.println(F("setBitrate OK"));
    delay(10);

#ifdef LOOPBACK_TEST
    mcp2515.setLoopbackMode();
#else
    mcp2515.setNormalMode();
#endif
    delay(10);

    // Verify mode actually took by reading CANSTAT register directly.
    // opmode bits [7:5]: 0x00=Normal 0x40=Loopback 0x80=Config
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    digitalWrite(MCP_CS_PIN, LOW);
    SPI.transfer(0x03); SPI.transfer(0x0E); // READ CANSTAT
    uint8_t canstat = SPI.transfer(0x00);
    digitalWrite(MCP_CS_PIN, HIGH);
    SPI.endTransaction();

    uint8_t opmode = canstat & 0xE0;
    Serial.print(F("CANSTAT=0x")); Serial.print(canstat, HEX);
#ifdef LOOPBACK_TEST
    if (opmode == 0x40) {
        Serial.println(F("  mode=Loopback OK"));
    } else {
        Serial.println(F("  mode!=Loopback — setLoopbackMode() did not take"));
        Serial.println(F("  ERR: increase delay before setLoopbackMode, or check MISO"));
        while (true);
    }
#else
    Serial.println(opmode == 0x00 ? F("  mode=Normal OK") : F("  mode!=Normal — check wiring"));
#endif

    Serial.println(F("MCP2515 ready"));
}

void loop() {
    delay(2000);

    float hum  = dht.readHumidity();
    float temp = dht.readTemperature();
    bool  ok   = !isnan(hum) && !isnan(temp);

    if (!ok) Serial.println(F("ERR: DHT11 read failed — check wiring & pull-up"));

    can_frame tx = buildFrame(temp, hum, ok);
    MCP2515::ERROR sendErr = mcp2515.sendMessage(&tx);
    if (sendErr != MCP2515::ERROR_OK) {
        // code 2=ALLTXBUSY: TX buffers all stuck — chip probably in CONFIG mode
        // code 4=FAILTX: frame sent but no ACK (expected in normal mode, no bus)
        Serial.print(F("ERR: send failed code=")); Serial.println(sendErr);
        return;
    }
    printFrame("TX ", tx);

#ifdef LOOPBACK_TEST
    delay(10);
    can_frame rx;
    if (mcp2515.readMessage(&rx) == MCP2515::ERROR_OK) {
        printFrame("RX ", rx);
        Serial.println(F("  [loopback OK]"));
    } else {
        Serial.println(F("ERR: loopback RX failed"));
    }
#else
    // Drain all frames waiting in RX buffers from other nodes
    can_frame rx;
    while (mcp2515.readMessage(&rx) == MCP2515::ERROR_OK) {
        printFrame("RX ", rx);
    }
#endif
}
