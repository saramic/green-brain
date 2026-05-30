#include "Arduino_RouterBridge.h"

void handle_message(int value) {
    Monitor.print("tick: ");
    Monitor.println(value);
}

void handle_set_led(String state) {
    Monitor.print("raw len=");
    Monitor.print(state.length());
    Monitor.print(" bytes=[");
    for (int i = 0; i < (int)state.length(); i++) {
        Monitor.print((int)state[i]);
        Monitor.print(' ');
    }
    Monitor.println("]");

    state.trim();
    bool on = state == "ON";
    digitalWrite(LED_BUILTIN, on ? LOW : HIGH);  // STM32 LED_BUILTIN is active-LOW
    Monitor.print("LED: ");
    Monitor.println(on ? "ON" : "OFF");
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    Bridge.begin();
    Monitor.begin();

    Bridge.provide_safe("print_value", handle_message);
    Bridge.provide_safe("set_led",     handle_set_led);
}

void loop() {
    // Bridge handles incoming calls in the background.
}
