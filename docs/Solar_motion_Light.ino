//Quelle: https://forum.mysensors.org/topic/3107/chinese-solar-lipo-powered-pir-led-lamp/19

// Enable debug prints
#define MY_DEBUG
#define MY_NODE_ID 11 
// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>
#include <Bounce2.h>

//unsigned long SLEEP_TIME = 120000; // Sleep time between reports (in milliseconds)
#define DIGITAL_INPUT_SENSOR 2   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
//#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 12   // Id of the sensor child

boolean lastMotion = false;

// Initialize motion message - start
MyMessage msg(CHILD_ID, V_TRIPPED);

//trigger solar power day on/off -start
#define CHILD_ID_SW 5
#define BUTTON_PIN  5  // Arduino Digital I/O pin for button/reed switch
Bounce debouncer = Bounce();
int oldValue = -1;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage SolarMsg(CHILD_ID_SW, V_TRIPPED);
// trigger solar - end

#define RELAY_1  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 2 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

void setup()
{
    //trigger solar power day on/off - start
    // Setup the button
    pinMode(BUTTON_PIN, INPUT);
    // Activate internal pull-up
    digitalWrite(BUTTON_PIN, HIGH);

    // After setting up the button, setup debouncer
    debouncer.attach(BUTTON_PIN);
    debouncer.interval(5);
    //trigger solar - end


    pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input

    for (int sensor = 1, pin = RELAY_1; sensor <= NUMBER_OF_RELAYS; sensor++, pin++) {
        // Then set relay pins in output mode
        pinMode(pin, OUTPUT);
        // Set relay to last known state (using eeprom storage) 
        digitalWrite(pin, loadState(sensor) ? RELAY_ON : RELAY_OFF);
    }
}

void presentation() {
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Motion Sensor and light", "1.0");

    // Register all sensors to gw (they will be created as child devices)
    present(CHILD_ID, S_MOTION);
    for (int sensor = 1, pin = RELAY_1; sensor <= NUMBER_OF_RELAYS; sensor++, pin++) {
        // Register all sensors to gw (they will be created as child devices)
        present(sensor, S_LIGHT);

        // Register binary input sensor to gw (they will be created as child devices)
        // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
        // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
        present(CHILD_ID_SW, S_DOOR);
    }
}

void loop()
{
    // Read digital motion value
    boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH;

    if (lastMotion != tripped) {
        Serial.println(tripped);
        lastMotion = tripped;
        send(msg.set(tripped ? "1" : "0"));  // Send tripped value to gw 
    }
    // Sleep until interrupt comes in on motion sensor. Send update every two minute. 
    //sleep(INTERRUPT,CHANGE, SLEEP_TIME);

    //trigger solar power day on/off - start
    debouncer.update();
    // Get the update value
    int value = debouncer.read();

    if (value != oldValue) {
        // Send in the new value
        send(SolarMsg.set(value == HIGH ? 1 : 0));
        oldValue = value;
    //trigger solar power day on/off - stop
    }
}

void receive(const MyMessage &message) {
    // We only expect one type of message from controller. But we better check anyway.
    if (message.type == V_LIGHT) {
        // Change relay state
        digitalWrite(message.sensor - 1 + RELAY_1, message.getBool() ? RELAY_ON : RELAY_OFF);
        // Store state in eeprom
        saveState(message.sensor, message.getBool());
        // Write some debug info
        Serial.print("Incoming change for sensor:");
        Serial.print(message.sensor);
        Serial.print(", New status: ");
        Serial.println(message.getBool());
    }
}
