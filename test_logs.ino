//======================================================
// LIBRARIES
//======================================================

#include "HX711.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


//======================================================
// PIN DEFINITIONS
//======================================================

// HX711 Load Cell
#define DOUT 3
#define SCK  2

// IR RPM Sensor
const byte SENSOR_PIN = 19;


//======================================================
// CALIBRATION
//======================================================

#define CALIBRATION_FACTOR 420.0f


//======================================================
// ESC CONFIGURATION
//======================================================

#define PWM_FREQ      50
#define ESC_CHANNEL   0

#define ESC_MIN_US    1000
#define ESC_MAX_US    2000


//======================================================
// SAFETY SETTINGS
//======================================================

#define WATCHDOG_MS   5000


//======================================================
// GLOBAL VARIABLES
//======================================================

// RPM Counter (updated inside interrupt)
volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;

// ESC State
uint16_t currentThrottleUs  = ESC_MIN_US;
uint8_t  currentThrottlePct = 0;
bool     armed              = false;

float rpm = 0;
float thrust_g = 0;
float thrust_N = 0;

// Watchdog Timer
unsigned long lastCmdMs = 0;

//======================================================
// AUTO TEST LOGGER
//======================================================

bool autoTest = false;

const uint8_t STEP_SIZE = 5;          // 5% increments
const uint32_t STABILIZE_TIME = 5000; // wait 5 sec
const uint32_t SAMPLE_TIME = 2000;    // average for 2 sec

uint8_t testThrottle = 0;

unsigned long stateStart = 0;

enum TestState
{
  TEST_IDLE,
  TEST_STABILIZE,
  TEST_SAMPLE,
  TEST_DONE
};

TestState testState = TEST_IDLE;

// Averaging
float rpmSum = 0;
float thrustSum = 0;
int sampleCount = 0;
//======================================================
// OBJECTS
//======================================================

// Load Cell Amplifier
HX711 scale;

// PCA9685 PWM Driver
Adafruit_PWMServoDriver pwm(0x40);


// ── Helpers ───────────────────────────────────────────────
void pulse()
{
    unsigned long now = micros();

    unsigned long interval = now - lastPulseTime;

    // Ignore impossible pulses
    if(interval > 100)
    {
        pulseInterval = interval;
        lastPulseTime = now;
    }
}

void flushSerial() {
  while (Serial.available()) Serial.read();
}

uint16_t usToPulse(uint16_t us) {
  return (uint16_t)((float)us / 20000.0f * 4096.0f);
}

// Convert 0–100% to 1000–2000 us
uint16_t pctToUs(uint8_t pct) {
  pct = constrain(pct, 0, 100);
  return ESC_MIN_US + (uint16_t)((float)pct / 100.0f * (ESC_MAX_US - ESC_MIN_US));
}

void setThrottlePct(uint8_t pct) {
  currentThrottlePct = constrain(pct, 0, 100);
  currentThrottleUs  = pctToUs(currentThrottlePct);
  pwm.setPWM(ESC_CHANNEL, 0, usToPulse(currentThrottleUs));
}

void cutMotor() {
  setThrottlePct(0);
  armed = false;
  Serial.println("[SAFETY] Motor cut. Send 'arm' to re-arm.");
}

void printHelp()
{
    Serial.println(F("======================================"));
    Serial.println(F("Available Commands"));
    Serial.println(F("--------------------------------------"));
    Serial.println(F("arm      : Arm ESC"));
    Serial.println(F("disarm   : Stop motor"));
    Serial.println(F("0-100    : Set throttle (%)"));
    Serial.println(F("tare     : Zero the load cell"));
    Serial.println(F("status   : Display current status"));
    Serial.println(F("help     : Show this menu"));
    Serial.println(F("======================================"));
}


// loops functions ---------------------------------------------------

//======================================================
// RPM TASK
//======================================================

#define PULSES_PER_REV 9.0f      // Adjust after testing

void updateRPM()
{
    static unsigned long lastPrint = 0;

    if (millis() - lastPrint < 250)
        return;

    lastPrint = millis();

    noInterrupts();
    unsigned long interval = pulseInterval;
    unsigned long lastPulse = lastPulseTime;
    interrupts();

    if (micros() - lastPulse > 500000)
    {
        rpm = 0;
    }
    else if (interval > 0)
    {
        rpm = 60000000.0f / (interval * PULSES_PER_REV);
    }

    Serial.print("RPM : ");
    Serial.println(rpm);
}


//======================================================
// THRUST TASK
//======================================================

void updateThrust()
{
    static unsigned long lastPrint = 0;

    if (millis() - lastPrint >= 250)
    {
        lastPrint = millis();

        thrust_g = scale.get_units(3);

        if (thrust_g < 0)
            thrust_g = 0;

        thrust_N = thrust_g * 0.00981;

        Serial.print("Thrust : ");
        Serial.print(thrust_g, 1);
        Serial.print(" g | ");

        Serial.print(thrust_N, 3);
        Serial.print(" N | ");

        Serial.print("Throttle : ");
        Serial.print(currentThrottlePct);
        Serial.print("% (");
        Serial.print(currentThrottleUs);
        Serial.print(" us) | ");

        Serial.println(armed ? "ARMED" : "DISARMED");
    }
}

//======================================================
// WATCHDOG TASK
//======================================================

void checkWatchdog()
{
    if (!armed)
        return;

    if (millis() - lastCmdMs > WATCHDOG_MS)
    {
        Serial.println(F("[WATCHDOG] No command received."));
        cutMotor();
    }
}

//======================================================
// SERIAL COMMAND TASK
//======================================================

void processSerial()
{
    if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "arm") {
      if (!armed) {
        armed = true;
        lastCmdMs = millis();
        Serial.println("Armed. Throttle at 0%.");
      } else {
        Serial.println("Already armed.");
      }

    } else if (cmd == "disarm") {
      cutMotor();

    } else if (cmd == "tare" || cmd == "zero") {
      bool wasArmed = armed;
      if (armed) cutMotor();
      Serial.println("Remove load and wait...");
      delay(1000);
      scale.tare();
      Serial.println("Tared.");
      if (wasArmed) Serial.println("Re-arm when ready.");

    } else if (cmd == "status") {
      float thrust_g = scale.get_units(20);
      thrust_g = max(0.0f, thrust_g);
      Serial.print("Throttle: "); Serial.print(currentThrottlePct); Serial.print("% (");
      Serial.print(currentThrottleUs); Serial.println(" us)");
      Serial.print("Thrust:   "); Serial.print(thrust_g, 1); Serial.println(" g");
      Serial.print("State:    "); Serial.println(armed ? "ARMED" : "DISARMED");

    } else if (cmd == "help") {
      printHelp();

    } else if(cmd == "test")
    
    {
        if(!armed)
        {
            Serial.println("Arm first.");
            return;
        }
    
        Serial.println();
        Serial.println("Throttle(%),RPM,Thrust(g)");
    
        autoTest = true;
        testThrottle = 0;
        testState = TEST_STABILIZE;
    
        setThrottlePct(testThrottle);
    
        stateStart = millis();
    }
 
    
    else {
      // Try parsing as a number (throttle percentage)
      // isDigit check handles accidental empty lines
      bool isNumber = true;
      for (int i = 0; i < cmd.length(); i++) {
        if (!isDigit(cmd[i])) { isNumber = false; break; }
      }

      if (isNumber && cmd.length() > 0) {
        int pct = cmd.toInt();
        if (pct < 0 || pct > 100) {
          Serial.println("Out of range. Enter 0 to 100.");
        } else if (!armed) {
          Serial.println("Not armed. Send 'arm' first.");
        } else {
          setThrottlePct(pct);
          lastCmdMs = millis();
          Serial.print("Throttle → ");
          Serial.print(currentThrottlePct);
          Serial.print("% = ");
          Serial.print(currentThrottleUs);
          Serial.println(" us");
        }
      } else if (cmd.length() > 0) {
        Serial.print("Unknown command: '");
        Serial.print(cmd);
        Serial.println("'. Send 'help' for list.");
      }
    }

  }
}

void autoTestTask()
{
    if(!autoTest)
        return;

    switch(testState)
    {
        case TEST_STABILIZE:

            if(millis() - stateStart >= STABILIZE_TIME)
            {
                rpmSum = 0;
                thrustSum = 0;
                sampleCount = 0;

                stateStart = millis();

                testState = TEST_SAMPLE;
            }

            break;


        case TEST_SAMPLE:

            rpmSum += rpm;
            thrustSum += thrust_g;
            sampleCount++;

            delay(50);

            if(millis() - stateStart >= SAMPLE_TIME)
            {
                float avgRPM = rpmSum / sampleCount;
                float avgThrust = thrustSum / sampleCount;

                Serial.print(testThrottle);
                Serial.print(",");
                Serial.print(avgRPM,0);
                Serial.print(",");
                Serial.println(avgThrust,1);

                testThrottle += STEP_SIZE;

                if(testThrottle > 100)
                {
                    setThrottlePct(0);
                    autoTest = false;
                    testState = TEST_DONE;

                    Serial.println("TEST COMPLETE");
                }
                else
                {
                    setThrottlePct(testThrottle);
                    stateStart = millis();
                    testState = TEST_STABILIZE;
                }
            }

            break;

        default:
            break;
    }
}
 
// ── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulse, FALLING);

  // Load cell
  scale.begin(DOUT, SCK);
  scale.set_scale(CALIBRATION_FACTOR);
  Serial.println("Taring load cell — ensure no load is applied...");
  delay(1000);
  scale.tare();
  Serial.println("Load cell ready.");

  // PCA9685
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);


  // Send 0% (1000 us) immediately for ESC arming signal
  setThrottlePct(0);
  Serial.println("PWM output started at 0% (1000 us).");
  Serial.println("Power on your ESC/LiPo now, then send 'arm' after the ESC beeps.");

  lastCmdMs = millis();
  
  printHelp();
}

// ── Loop ──────────────────────────────────────────────────
void loop()
{
    updateRPM();

    updateThrust();

    checkWatchdog();

    processSerial();

    autoTestTask();
}
