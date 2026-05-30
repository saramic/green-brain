#include "Arduino_RouterBridge.h"

void handle_message(int value) {
  // Use Monitor.print to send text back to the App Lab console
  Monitor.print("MCU received value: ");
  Monitor.println(value);
}

void setup() {
  // Start the Bridge communication layer
  Bridge.begin();

  // Start the Monitor
  Monitor.begin();

  // Expose the function to the Bridge so Python can call it.
  // We use "print_value" as the name Python will use to find it.
  Bridge.provide_safe("print_value", handle_message);
}

void loop() {
  // The Bridge handles incoming calls automatically in the background.
  // Keep the loop clear of blocking code.
}