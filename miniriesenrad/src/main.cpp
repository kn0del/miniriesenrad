#include <Arduino.h>

// ==================== PIN DEFINITIONS ====================
const int switch1Pin = 21;  
const int switch2Pin = 16;

const int relay1Pin = 14;
const int relay2Pin = 13;

const int led1Pin = 11;
const int led2Pin = 10;

// ==================== TIMING ====================
const unsigned long relayDuration = 20000; // 20 seconds
const unsigned long ledOnTime = 50000;     // 50 sec
const unsigned long fadeTime = 5000;       // 5 sec

// ==================== STATES ====================
unsigned long relay1End = 0;
unsigned long relay2End = 0;

unsigned long led1CycleStart = 0;
unsigned long led2CycleStart = 0;

bool led1FadingOut = false;
bool led2FadingOut = false;

// Morse blink state
bool led2BlinkActive = false;
int blinkStep = 0;
unsigned long blinkTimer = 0;

// ==================== HELPERS ====================
void setLED(int channel, int value) {
    ledcWrite(channel, value);
}

void handleRelay(int switchPin, int relayPin, unsigned long &relayEndTime) {
    if (digitalRead(switchPin) == LOW) {
        digitalWrite(relayPin, HIGH);
        relayEndTime = millis() + relayDuration;

        if (relayPin == relay2Pin) {
            // trigger blink for LED2 only
            led2BlinkActive = true;
            blinkStep = 0;
            blinkTimer = millis();
        }
    }

    if (relayEndTime > 0 && millis() >= relayEndTime) {
        digitalWrite(relayPin, LOW);
        relayEndTime = 0;
    }
}

void handleBlinkingLED2() {
    if (!led2BlinkActive) return;

    const unsigned long blinkInterval = 150; // fast blink
    if (millis() - blinkTimer < blinkInterval) return;

    blinkTimer = millis();
    blinkStep++;

    if (blinkStep % 2 == 1)
        setLED(1, 255); // max brightness
    else
        setLED(1, 0);   // off

    if (blinkStep >= 6) {
        led2BlinkActive = false;
        setLED(1, 255);
        led2CycleStart = millis(); // restart normal cycle
    }
}

void handleLED(const int channel, unsigned long &cycleStart, bool &fadingOut) {
    unsigned long now = millis();
    unsigned long elapsed = now - cycleStart;

    if (elapsed < ledOnTime) {
        setLED(channel, 255);
        return;
    }

    elapsed -= ledOnTime;

    if (elapsed < fadeTime) {
        int pwm = 255 - map(elapsed, 0, fadeTime, 0, 255);
        setLED(channel, pwm);
        return;
    }

    elapsed -= fadeTime;

    if (elapsed < fadeTime) {
        int pwm = map(elapsed, 0, fadeTime, 0, 255);
        setLED(channel, pwm);
        return;
    }

    // loop cycle
    cycleStart = now;
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);

    pinMode(switch1Pin, INPUT_PULLUP);
    pinMode(switch2Pin, INPUT_PULLUP);

    pinMode(relay1Pin, OUTPUT);
    pinMode(relay2Pin, OUTPUT);

    digitalWrite(relay1Pin, LOW);
    digitalWrite(relay2Pin, LOW);

    ledcAttachPin(led1Pin, 0);
    ledcAttachPin(led2Pin, 1);

    ledcSetup(0, 5000, 8);
    ledcSetup(1, 5000, 8);

    led1CycleStart = millis();
    led2CycleStart = millis();
}

// ==================== MAIN LOOP ====================
void loop() {
    handleRelay(switch1Pin, relay1Pin, relay1End);
    handleRelay(switch2Pin, relay2Pin, relay2End);

    handleBlinkingLED2();

    handleLED(0, led1CycleStart, led1FadingOut);

    if (!led2BlinkActive)
        handleLED(1, led2CycleStart, led2FadingOut);

    delay(5);
}
