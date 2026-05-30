import time

from arduino.app_utils import App, Bridge

counter = 0

def loop():
    global counter

    # Wait for two seconds between messages
    time.sleep(2)

    # Increment the counter
    counter += 1

    # Call the C++ function "print_value" that we exposed earlier
    # We pass the current counter as the argument
    Bridge.call("print_value", counter)

# See: https://docs.arduino.cc/software/app-lab/tutorials/getting-started/#app-run
App.run(user_loop=loop)
