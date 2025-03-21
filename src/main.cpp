#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Function declarations (prototypes)
void handleButton();
void toggleRelay();
void toggleFlashing();
void handleFlashing(int n, int flashDelay);
void handleRoot();
void handleConfig();

// Pin and Timing
const int controlPin = 4;
const unsigned long clickThreshold = 200; // Milliseconds for a click
const unsigned long holdThreshold = 500; // Milliseconds for a hold

// Relay State
const int relayPin = 2;
bool relayState = false;
bool currentRelayState = relayState;

// Flashing
bool flashing = false;
unsigned int flashNumberConfig = 3; // default flash 3 times
unsigned long flashDelay = 200; // Default flash delay
unsigned long previousMillis = 0;
unsigned int flashCount = 0;
unsigned int flashCounter = 0;
unsigned int flashNumber = 0;

// Button State
bool buttonPressed = false;
bool buttonHolded = false;
unsigned long buttonPressTime = 0;

// Configuration
Preferences preferences;
bool clickForOnOff = false; // Default: click for on/off, hold for flash

// Web Server
WebServer server(80);

void setup() {
    pinMode(controlPin, INPUT_PULLUP);
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW); // Initialize relay off
    Serial.begin(115200);

    // Load configuration from preferences
    preferences.begin("light_config");
    clickForOnOff = preferences.getBool("click_on_off", clickForOnOff);
    flashDelay = preferences.getUInt("flash_delay", flashDelay);
    flashNumber = preferences.getUInt("flash_number", flashNumberConfig);
    preferences.end();

    Serial.print("click_on_off:");
    Serial.print(clickForOnOff);

    // // WiFi setup
    // WiFi.softAP("LightConfig", "password");
    // Serial.println("WiFi Hotspot started");
    // Serial.print("IP Address: ");
    // Serial.println(WiFi.softAPIP());
    // // Web server routes
    // server.on("/", handleRoot);
    // server.on("/config", handleConfig);
    // server.begin();
    // Serial.println("Web server started");
}

void loop() {
    server.handleClient();
    handleButton();
    // handleFlashing();
}

void handleButton() {
    int buttonState = digitalRead(controlPin);
    if (buttonState == LOW && !buttonPressed) {
        buttonPressed = true;
        if (buttonPressTime == 0){
            buttonPressTime = millis();
        }
    }
    else if(buttonState == LOW && buttonPressed && !buttonHolded){
        unsigned long pressDuration = millis() - buttonPressTime;
        if (pressDuration >= holdThreshold &&  !buttonHolded) {
            buttonHolded = true;
            Serial.println("Holded");
            if (clickForOnOff) {
                Serial.println("Holded flashing");
                toggleFlashing();
            } else {
                Serial.println("Holded toggleRelay");
                toggleRelay();
            }
        }
    }
    else if (buttonState == HIGH && buttonHolded){
        buttonPressed = false;
        buttonHolded = false;
        buttonPressTime = 0;
    }
    else if (buttonState == HIGH && buttonPressed) {
        unsigned long pressDuration = millis() - buttonPressTime;
        Serial.print("Press Duration:");
        Serial.println(pressDuration);
        if (pressDuration < clickThreshold) {
            if (clickForOnOff) {
                Serial.println("Press toggle");
                toggleRelay();
            } else {
                Serial.println("Press flashing");
                handleFlashing(flashNumber, flashDelay);
            }
        } 

        buttonPressed = false;
        buttonPressTime = 0;
    }
}

void toggleRelay() {
    relayState = !relayState;
    digitalWrite(relayPin, relayState);
    Serial.print("Relay: ");
    Serial.println(relayState ? "ON" : "OFF");
}

void toggleFlashing() {
    flashing = !flashing;
    Serial.print("Flashing: ");
    Serial.println(flashing ? "ON" : "OFF");
}

// void handleFlashing(int num) {
//     if (!flashing) {
//         flashing = true;
//         flashCount = num * 2; // Nhân đôi số lần vì mỗi lần nhấp nháy gồm bật và tắt
//         flashCounter = 0;
//         currentRelayState = relayState; // Lưu trạng thái relay ban đầu
//         previousMillis = millis();
//     }

//     if (flashing) {
//         unsigned long currentMillis = millis();
//         if (currentMillis - previousMillis >= flashDelay) {
//             previousMillis = currentMillis;
//             relayState = !relayState;
//             digitalWrite(relayPin, relayState);
//             flashCounter++;
//             if (flashCounter >= flashCount) {
//                 flashing = false;
//                 digitalWrite(relayPin, currentRelayState); // Khôi phục trạng thái ban đầu
//                 relayState = currentRelayState;
//             }
//         }
//     }
// }

void handleFlashing(int num, int flashDelay) {
  if (!flashing) {
      flashing = true;
      currentRelayState = relayState; // Lưu trạng thái relay ban đầu
      for (int i = 0; i < num * 2; i++) {
          relayState = !relayState;
          digitalWrite(relayPin, relayState);
          delay(flashDelay);
      }
      digitalWrite(relayPin, currentRelayState); // Khôi phục trạng thái ban đầu
      relayState = currentRelayState;
      flashing = false;
  }
}

void handleRoot() {
    String html = "<h1>Light Configuration</h1>";
    html += "<form action='/config' method='get'>";
    html += "<p>Mode: <select name='mode'><option value='click' " + String(clickForOnOff ? "selected" : "") + ">Click On/Off</option><option value='hold' " + String(!clickForOnOff ? "selected" : "") + ">Hold On/Off</option></select></p>";
    html += "<p>Flash Delay (ms): <input type='number' name='delay' value='" + String(flashDelay) + "'></p>";
    html += "<p><input type='submit' value='Save'></p>";
    html += "</form>";
    server.send(200, "text/html", html);
}

void handleConfig() {
    String mode = server.arg("mode");
    String delayStr = server.arg("delay");
    clickForOnOff = (mode == "click");
    flashDelay = delayStr.toInt();
    
    preferences.begin("light_config");
    preferences.putBool("click_on_off", clickForOnOff);
    preferences.putUInt("flash_delay", flashDelay);
    preferences.end();
    
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", ""); // Redirect to root
}