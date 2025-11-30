#include <Arduino.h>

// Pin definitions
const int button1Pin = 21;
const int button2Pin = 16;
const int relay1Pin  = 14;
const int relay2Pin  = 13;
const int led1Pin    = 11;
const int led2Pin    = 10;

// Timing constants
const unsigned long relayOnTime = 20000;  // 20 sec
const unsigned long ledOnTime   = 5000;  // 50 sec
const unsigned long fadeTime    = 5000;   // 5 sec
const unsigned long morseDash   = 200;    // 200 ms per dash

// LED PWM channels
const int led1Channel = 0;
const int led2Channel = 1;
const int pwmFreq = 5000;
const int pwmResolution = 8;

// State variables
unsigned long relay1Start = 0;
unsigned long relay2Start = 0;
bool relay1Active = false;
bool relay2Active = false;

unsigned long ledCycleStart = 0;
int fadeStage = 0; // 0=on,1=fadeOut,2=fadeIn
int ledBrightness = 255;

unsigned long morseStart = 0;
int morseStep = 0;
bool morseActive = false;

void setup() {
  Serial.begin(115200);

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);

  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, LOW);

  // Setup PWM for LEDs
  ledcSetup(led1Channel, pwmFreq, pwmResolution);
  ledcAttachPin(led1Pin, led1Channel);

  ledcSetup(led2Channel, pwmFreq, pwmResolution);
  ledcAttachPin(led2Pin, led2Channel);

  ledCycleStart = millis();
}

void loop() {
  unsigned long now = millis();

  // --- Handle buttons and relays ---
  if (digitalRead(button1Pin) == LOW && !relay1Active) { // Button pressed (active low)
    relay1Active = true;
    relay1Start = now;
    digitalWrite(relay1Pin, HIGH);
    Serial.println("Relay 1 activated");
  }

  if (digitalRead(button2Pin) == LOW && !relay2Active) {
    relay2Active = true;
    relay2Start = now;
    digitalWrite(relay2Pin, HIGH);
    morseActive = true;
    morseStart = now;
    morseStep = 0;
    Serial.println("Relay 2 activated, morse sequence started");
  }

  if (relay1Active && now - relay1Start >= relayOnTime) {
    relay1Active = false;
    digitalWrite(relay1Pin, LOW);
    Serial.println("Relay 1 deactivated");
  }

  if (relay2Active && now - relay2Start >= relayOnTime) {
    relay2Active = false;
    digitalWrite(relay2Pin, LOW);
    Serial.println("Relay 2 deactivated");
  }

  // --- Handle LED fade cycle ---
  unsigned long elapsed = now - ledCycleStart;
  switch (fadeStage) {
    case 0: // ON
      ledBrightness = 255;
      if (elapsed >= ledOnTime) {
        fadeStage = 1;
        ledCycleStart = now;
      }
      break;
    case 1: // Fade out
      ledBrightness = map(elapsed, 0, fadeTime, 255, 0);
      if (elapsed >= fadeTime) {
        fadeStage = 2;
        ledCycleStart = now;
      }
      break;
    case 2: // Fade in
      ledBrightness = map(elapsed, 0, fadeTime, 0, 255);
      if (elapsed >= fadeTime) {
        fadeStage = 0;
        ledCycleStart = now;
      }
      break;
  }

  // --- Handle LED2 morse dash-dash-dash ---
  if (morseActive) {
    int dashIndex = morseStep % 2; // 0=on,1=off
    if (now - morseStart >= dashIndex * morseDash + morseStep * morseDash) {
      ledcWrite(led2Channel, (dashIndex == 0) ? 255 : 0);
      morseStep++;
      if (morseStep >= 6) { // 3 dashes
        morseActive = false;
        ledcWrite(led2Channel, ledBrightness); // back to normal PWM
      }
    }
  } else {
    // Normal fading
    ledcWrite(led1Channel, ledBrightness);
    ledcWrite(led2Channel, ledBrightness);
  }

  delay(10); // tiny delay for stability
}
