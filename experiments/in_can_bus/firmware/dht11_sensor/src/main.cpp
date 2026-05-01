#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN 4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
    Serial.begin(115200);
    dht.begin();
    Serial.println(F("GreenBrain | DHT11 Sensor | Phase 1"));
    Serial.println(F("-------------------------------------"));
}

void loop() {
    delay(2000);

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("ERR: DHT11 read failed — check wiring"));
        return;
    }

    Serial.print(F("T:"));
    Serial.print(temperature, 1);
    Serial.print(F("C  H:"));
    Serial.print(humidity, 1);
    Serial.println(F("%"));
}
