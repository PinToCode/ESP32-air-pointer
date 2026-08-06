/*
  ==============================================================================
   ESP32-C3 SuperMini + MPU9250 Air Mouse (Gyro Direct Delta + TTP223 Gestures)
  ==============================================================================

============================================================================================
| Gesture                |   Action                                                         |
============================================================================================
| Single Tap             |   Left Click                                                     |
| Double Tap             |   Double Left Click (Selects Word)                               |
| Triple Tap             |   Right Click                                                    |
| Single Tap & Hold      |   Click & Drag (Holds Left Click down while moving the mouse)    |
| Double Tap & Hold      |   Scroll Mode (Gyro Pitch controls scrolling)                    |
============================================================================================
*/

#include <Wire.h>
#include <HijelHID_BLEMouse.h>

// ---------------------------------------------------------------------------
// Production Tuning Parameters (Locked In)
// ---------------------------------------------------------------------------
static const uint32_t SAMPLE_RATE_HZ = 125;  // Matched with BLE report rate (125Hz)
static const float DT = 1.0f / SAMPLE_RATE_HZ;

// Base speed multipliers
static const float SENSITIVITY_X = 1.0f;
static const float SENSITIVITY_Y = 1.0f;

// Noise threshold: ignore gyro rates below this (degrees/sec)
static const float GYRO_DEADZONE_DPS = 1.3f;

// Precision Threshold: speeds below this (dps) will be heavily dampened
static const float PRECISION_THRESHOLD_DPS = 15.0f;

// Low-pass filter weight (Tremor suppression)
static const float FILTER_ALPHA = 0.18f;

// Scroll sensitivity scaling
static const float SCROLL_SENSITIVITY = 0.15f;

// ---------------------------------------------------------------------------
// TTP223 Touch Sensor Configuration (ESP32-C3 SuperMini Pin)
// ---------------------------------------------------------------------------
static const int TOUCH_SIG_PIN = 3;   // GPIO 3 on ESP32-C3 SuperMini

static const uint32_t DEBOUNCE_MS        = 30;   // Ignore glitches under 30ms
static const uint32_t TAP_GAP_WINDOW     = 300;  // Gap allowed between taps (ms)
static const uint32_t DRAG_HOLD_MS       = 200;  // Single hold duration to start Left Drag (ms)
static const uint32_t SCROLL_HOLD_MS     = 200;  // Double tap hold duration to start Scroll (ms)

// ---------------------------------------------------------------------------
// MPU9250 Registers
// ---------------------------------------------------------------------------
static const uint8_t MPU_ADDR = 0x68;
static const uint8_t REG_PWR_MGMT_1 = 0x6B;
static const uint8_t REG_SMPLRT_DIV = 0x19;
static const uint8_t REG_CONFIG = 0x1A;
static const uint8_t REG_GYRO_CONFIG = 0x1B;
static const uint8_t REG_ACCEL_CONFIG = 0x1C;
static const uint8_t REG_ACCEL_XOUT_H = 0x3B;

static const float GYRO_SENS = 65.5f;  // LSB/(deg/s) @ ±500dps

HijelBLEMouse mouse("ESP32 Air Pointer", "DIY");

float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;
float smoothDx = 0.0f, smoothDy = 0.0f;
float carryX = 0.0f, carryY = 0.0f;

// Scroll sub-unit accumulator
float scrollCarryY = 0.0f;

uint32_t loopIntervalUs;
uint32_t nextLoopUs;

// Gesture state variables
bool touchActive = false;
bool isDragging  = false;
bool isScrolling = false;

uint32_t touchStartTime  = 0;
uint32_t lastReleaseTime = 0;

uint8_t tapCount         = 0;

void mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void mpuReadRaw(int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(REG_ACCEL_XOUT_H + 8);  // Jump directly to gyro bytes
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU_ADDR, 6);

  gx = (Wire.read() << 8) | Wire.read();
  gy = (Wire.read() << 8) | Wire.read();
  gz = (Wire.read() << 8) | Wire.read();
}

void mpuInit() {
  mpuWrite(REG_PWR_MGMT_1, 0x00);  // Wake up
  delay(50);
  mpuWrite(REG_SMPLRT_DIV, 7);      // 1kHz / (1+7) = 125Hz
  mpuWrite(REG_CONFIG, 0x03);       // DLPF ~42Hz bandwidth
  mpuWrite(REG_GYRO_CONFIG, 0x08);  // ±500 dps full scale
  delay(50);
}

void calibrateGyro(uint16_t samples = 500) {
  double sumX = 0, sumY = 0, sumZ = 0;
  int16_t gx, gy, gz;

  for (uint16_t i = 0; i < samples; i++) {
    mpuReadRaw(gx, gy, gz);
    sumX += gx;
    sumY += gy;
    sumZ += gz;
    delay(2);
  }

  gyroBiasX = (float)(sumX / samples) / GYRO_SENS;
  gyroBiasY = (float)(sumY / samples) / GYRO_SENS;
  gyroBiasZ = (float)(sumZ / samples) / GYRO_SENS;
}

// Applies non-linear response curve to suppress micro-movements
float applyPrecisionCurve(float dps) {
  float absDps = fabsf(dps);

  if (absDps < GYRO_DEADZONE_DPS) return 0.0f;

  float sign = (dps > 0.0f) ? 1.0f : -1.0f;

  if (absDps < PRECISION_THRESHOLD_DPS) {
    float norm = (absDps - GYRO_DEADZONE_DPS) / (PRECISION_THRESHOLD_DPS - GYRO_DEADZONE_DPS);
    return sign * (norm * norm) * PRECISION_THRESHOLD_DPS;
  }

  return dps;
}

// ---------------------------------------------------------------------------
// Gesture-Rich Touch Handling
// ---------------------------------------------------------------------------
void handleTTP223Gestures() {
  uint32_t nowMs = millis();
  bool isTouched = (digitalRead(TOUCH_SIG_PIN) == HIGH);

  // --- 1. TOUCH PRESS EVENT ---
  if (isTouched && !touchActive) {
    touchActive = true;
    touchStartTime = nowMs;
  } 
  
  // --- 2. TOUCH HOLDING EVENTS ---
  else if (isTouched && touchActive) {
    uint32_t holdDuration = nowMs - touchStartTime;

    // Single Tap & Hold (>= 200ms) -> Engage Left Click Drag (For text selection)
    if (tapCount == 0 && holdDuration >= DRAG_HOLD_MS && !isDragging) {
      if (mouse.isPaired()) {
        mouse.press((MouseButton)1); // Hold Left Mouse Button DOWN
      }
      isDragging = true;
    } 
    // Double Tap & Hold (>= 200ms) -> Activate Scroll Mode
    else if (tapCount == 1 && holdDuration >= SCROLL_HOLD_MS && !isScrolling) {
      isScrolling = true;
    }
  } 
  
  // --- 3. TOUCH RELEASE EVENT ---
  else if (!isTouched && touchActive) {
    touchActive = false;
    uint32_t pressDuration = nowMs - touchStartTime;

    // Release Left-Click Drag
    if (isDragging) {
      if (mouse.isPaired()) {
        mouse.release((MouseButton)1); // Release Left Mouse Button
      }
      isDragging = false;
      tapCount = 0;
    } 
    // Release Scroll Mode
    else if (isScrolling) {
      isScrolling = false;
      tapCount = 0;
    } 
    // Record quick tap
    else if (pressDuration >= DEBOUNCE_MS) {
      tapCount++;
      lastReleaseTime = nowMs;

      // Triple Tap -> Right Click
      if (tapCount == 3) {
        if (mouse.isPaired()) {
          mouse.click((MouseButton)2); // Right Click
        }
        tapCount = 0;
      }
    }
  }

  // --- 4. MULTI-TAP TIMEOUTS ---
  if (!touchActive && tapCount > 0 && (nowMs - lastReleaseTime > TAP_GAP_WINDOW)) {
    // Single Tap -> Single Left Click
    if (tapCount == 1) {
      if (mouse.isPaired()) {
        mouse.click((MouseButton)1);
      }
    }
    // Double Tap -> Double Left Click
    else if (tapCount == 2) {
      if (mouse.isPaired()) {
        mouse.click((MouseButton)1);
        delay(30);
        mouse.click((MouseButton)1);
      }
    }
    tapCount = 0; // Reset
  }
}

void setup() {
  // Initialize I2C for ESP32-C3 SuperMini (SDA = GPIO8, SCL = GPIO9)
  Wire.begin(4, 5);
  Wire.setClock(400000);

  pinMode(TOUCH_SIG_PIN, INPUT);

  mpuInit();
  calibrateGyro();

  mouse.setUpdateRate(HIDRate::Hz125);
  mouse.begin();

  loopIntervalUs = 1000000UL / SAMPLE_RATE_HZ;
  nextLoopUs = micros() + loopIntervalUs;
}

void loop() {
  uint32_t now = micros();
  if ((int32_t)(now - nextLoopUs) < 0) return;
  nextLoopUs += loopIntervalUs;

  handleTTP223Gestures();

  int16_t rawGx, rawGy, rawGz;
  mpuReadRaw(rawGx, rawGy, rawGz);

  // Convert raw readings to degrees per second minus offset bias
  float gxDps = (rawGx / GYRO_SENS) - gyroBiasX;  // Pitch rate (Up / Down)
  float gzDps = (rawGz / GYRO_SENS) - gyroBiasZ;  // Yaw rate   (Left / Right)

  // Pass through Non-Linear Precision Curve
  float processedGz = applyPrecisionCurve(gzDps);
  float processedGx = applyPrecisionCurve(gxDps);

  // --- SCROLL MODE ACTIVE ---
  if (isScrolling) {
    if (mouse.isPaired()) {
      float rawScrollY = -processedGx * SCROLL_SENSITIVITY;

      float totalScrollY = rawScrollY + scrollCarryY;
      int16_t sendScrollY = (int16_t)totalScrollY;
      scrollCarryY = totalScrollY - sendScrollY;

      if (sendScrollY != 0) {
        mouse.scroll(sendScrollY);  // Single-parameter vertical scroll
      }
    }
    carryX = 0.0f;
    carryY = 0.0f;
    return;
  }

  // --- STANDARD CURSOR MOVEMENT ACTIVE ---
  float rawDx = -processedGz * SENSITIVITY_X;
  float rawDy = processedGx * SENSITIVITY_Y;

  // Exponential filter removes jitter/shaking
  smoothDx = (FILTER_ALPHA * rawDx) + ((1.0f - FILTER_ALPHA) * smoothDx);
  smoothDy = (FILTER_ALPHA * rawDy) + ((1.0f - FILTER_ALPHA) * smoothDy);

  // Accumulate sub-pixel remainders
  float totalDx = smoothDx + carryX;
  float totalDy = smoothDy + carryY;

  int16_t sendDx = (int16_t)totalDx;
  int16_t sendDy = (int16_t)totalDy;

  carryX = totalDx - sendDx;
  carryY = totalDy - sendDy;

  // Transmit over BLE HID
  if (mouse.isPaired() && (sendDx != 0 || sendDy != 0)) {
    mouse.move(sendDx, sendDy);
  }
}
