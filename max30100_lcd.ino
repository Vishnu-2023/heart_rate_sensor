#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// =====================================================
// ESP32 DEVKIT V1 + MAX30100 + 16x2 I2C LCD
// Fast BPM + SpO2
// No MAX30100 library required
// =====================================================


// =====================================================
// I2C
// =====================================================

#define SDA_PIN 21
#define SCL_PIN 22


// =====================================================
// MAX30100
// =====================================================

#define MAX30100_ADDR 0x57

#define REG_INT_STATUS        0x00
#define REG_INT_ENABLE        0x01
#define REG_FIFO_WR_PTR       0x02
#define REG_OVF_COUNTER       0x03
#define REG_FIFO_RD_PTR       0x04
#define REG_FIFO_DATA         0x05
#define REG_MODE_CONFIG       0x06
#define REG_SPO2_CONFIG       0x07
#define REG_LED_CONFIG        0x09
#define REG_TEMP_INT          0x16
#define REG_TEMP_FRAC         0x17
#define REG_REV_ID            0xFE
#define REG_PART_ID           0xFF


// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);


// =====================================================
// SAMPLING
// =====================================================

unsigned long lastSampleTime = 0;

const unsigned long SAMPLE_INTERVAL = 10;
// 10 ms = approximately 100 samples/second


// =====================================================
// DATA BUFFER
// =====================================================

const int BUFFER_SIZE = 200;

float irBuffer[BUFFER_SIZE];
float redBuffer[BUFFER_SIZE];

int bufferIndex = 0;

bool bufferFull = false;


// =====================================================
// HEART RATE
// =====================================================

unsigned long lastBeatTime = 0;

float bpm = 0;

bool beatState = false;


// =====================================================
// SpO2
// =====================================================

float spo2 = 0;


// =====================================================
// FINGER DETECTION
// =====================================================

bool fingerDetected = false;


// =====================================================
// WRITE MAX30100 REGISTER
// =====================================================

void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MAX30100_ADDR);

  Wire.write(reg);
  Wire.write(value);

  Wire.endTransmission();
}


// =====================================================
// READ MAX30100 REGISTER
// =====================================================

uint8_t readRegister(uint8_t reg)
{
  Wire.beginTransmission(MAX30100_ADDR);

  Wire.write(reg);

  Wire.endTransmission(false);

  Wire.requestFrom(MAX30100_ADDR, (uint8_t)1);

  if (Wire.available())
  {
    return Wire.read();
  }

  return 0;
}


// =====================================================
// READ MAX30100 FIFO
// =====================================================

bool readFIFO(uint16_t &ir, uint16_t &red)
{
  Wire.beginTransmission(MAX30100_ADDR);

  Wire.write(REG_FIFO_DATA);

  Wire.endTransmission(false);

  Wire.requestFrom(MAX30100_ADDR, (uint8_t)4);

  if (Wire.available() < 4)
  {
    return false;
  }


  uint8_t redHigh = Wire.read();
  uint8_t redLow  = Wire.read();

  uint8_t irHigh = Wire.read();
  uint8_t irLow  = Wire.read();


  red = ((uint16_t)redHigh << 8) | redLow;

  ir = ((uint16_t)irHigh << 8) | irLow;


  return true;
}


// =====================================================
// INITIALIZE MAX30100
// =====================================================

bool initMAX30100()
{
  uint8_t partID = readRegister(REG_PART_ID);


  Serial.print("MAX30100 Part ID: 0x");

  Serial.println(partID, HEX);


  // MAX30100 normally has Part ID 0x11

  if (partID != 0x11)
  {
    Serial.println("MAX30100 NOT DETECTED!");

    return false;
  }


  Serial.println("MAX30100 detected!");


  // ---------------------------------------------------
  // Reset
  // ---------------------------------------------------

  writeRegister(REG_MODE_CONFIG, 0x40);

  delay(100);


  // ---------------------------------------------------
  // Clear FIFO
  // ---------------------------------------------------

  writeRegister(REG_FIFO_WR_PTR, 0x00);

  writeRegister(REG_OVF_COUNTER, 0x00);

  writeRegister(REG_FIFO_RD_PTR, 0x00);


  // ---------------------------------------------------
  // Disable interrupts
  // ---------------------------------------------------

  writeRegister(REG_INT_ENABLE, 0x00);


  // ---------------------------------------------------
  // SpO2 configuration
  // 16-bit ADC
  // 100 Hz sample rate
  // ---------------------------------------------------

  writeRegister(REG_SPO2_CONFIG, 0x47);


  // ---------------------------------------------------
  // LED current
  // RED = approximately 11 mA
  // IR  = approximately 11 mA
  // ---------------------------------------------------

  writeRegister(REG_LED_CONFIG, 0x77);


  // ---------------------------------------------------
  // SpO2 mode
  // ---------------------------------------------------

  writeRegister(REG_MODE_CONFIG, 0x03);


  delay(100);


  return true;
}


// =====================================================
// FAST HEART RATE CALCULATION
// =====================================================

void calculateHeartRate()
{
  // Static variables keep their values between calls

  static float baseline = 0;

  static float filteredSignal = 0;

  static float previousSignal = 0;

  static float signalMin = 0;

  static float signalMax = 0;

  static bool initialized = false;


  // ---------------------------------------------------
  // Get newest IR value
  // ---------------------------------------------------

  int latestIndex =
      (bufferIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE;


  float irValue = irBuffer[latestIndex];


  // ---------------------------------------------------
  // Initialize
  // ---------------------------------------------------

  if (!initialized)
  {
    baseline = irValue;

    filteredSignal = 0;

    previousSignal = 0;

    signalMin = 0;

    signalMax = 0;

    initialized = true;

    return;
  }


  // ---------------------------------------------------
  // 1. Calculate DC baseline
  // ---------------------------------------------------

  baseline =
      (baseline * 0.95) +
      (irValue * 0.05);


  // ---------------------------------------------------
  // 2. Remove DC component
  // ---------------------------------------------------

  float acSignal =
      irValue - baseline;


  // ---------------------------------------------------
  // 3. Smooth the signal
  // ---------------------------------------------------

  filteredSignal =
      (filteredSignal * 0.75) +
      (acSignal * 0.25);


  // ---------------------------------------------------
  // 4. Update signal minimum
  // ---------------------------------------------------

  if (filteredSignal < signalMin)
  {
    signalMin = filteredSignal;
  }


  // ---------------------------------------------------
  // 5. Update signal maximum
  // ---------------------------------------------------

  if (filteredSignal > signalMax)
  {
    signalMax = filteredSignal;
  }


  // ---------------------------------------------------
  // 6. Slowly reduce old min/max
  // ---------------------------------------------------

  signalMin *= 0.995;

  signalMax *= 0.995;


  // ---------------------------------------------------
  // 7. Calculate signal range
  // ---------------------------------------------------

  float signalRange =
      signalMax - signalMin;


  // ---------------------------------------------------
  // Not enough pulse signal
  // ---------------------------------------------------

  if (signalRange < 20)
  {
    previousSignal = filteredSignal;

    return;
  }


  // ---------------------------------------------------
  // 8. Adaptive threshold
  // ---------------------------------------------------

  float threshold =
      signalMin +
      (signalRange * 0.55);


  // ---------------------------------------------------
  // 9. Current time
  // ---------------------------------------------------

  unsigned long now = millis();


  // ---------------------------------------------------
  // 10. Minimum time between beats
  // ---------------------------------------------------

  bool enoughTimePassed = true;


  if (lastBeatTime > 0)
  {
    unsigned long timeSinceLastBeat =
        now - lastBeatTime;


    // Prevent double detection

    if (timeSinceLastBeat < 350)
    {
      enoughTimePassed = false;
    }
  }


  // ---------------------------------------------------
  // 11. Detect rising edge
  // ---------------------------------------------------

  if (!beatState &&
      enoughTimePassed &&
      previousSignal < threshold &&
      filteredSignal >= threshold)
  {
    // -------------------------------------------------
    // HEARTBEAT DETECTED
    // -------------------------------------------------

    if (lastBeatTime > 0)
    {
      unsigned long interval =
          now - lastBeatTime;


      // ------------------------------------------------
      // Accept realistic heartbeat interval
      //
      // 350 ms = ~171 BPM
      // 1800 ms = ~33 BPM
      // ------------------------------------------------

      if (interval >= 350 &&
          interval <= 1800)
      {
        float newBPM =
            60000.0 / interval;


        // ----------------------------------------------
        // First BPM
        // ----------------------------------------------

        if (bpm == 0)
        {
          bpm = newBPM;
        }


        // ----------------------------------------------
        // Smooth BPM
        // ----------------------------------------------

        else
        {
          float difference =
              fabs(newBPM - bpm);


          // Small change

          if (difference < 15)
          {
            bpm =
                (bpm * 0.70) +
                (newBPM * 0.30);
          }


          // Medium change

          else if (difference < 30)
          {
            bpm =
                (bpm * 0.80) +
                (newBPM * 0.20);
          }


          // Large change

          else
          {
            bpm =
                (bpm * 0.90) +
                (newBPM * 0.10);
          }
        }


        // ----------------------------------------------
        // Serial output
        // ----------------------------------------------

        Serial.print("Heartbeat detected");

        Serial.print(" | New BPM: ");

        Serial.print(newBPM, 1);

        Serial.print(" | Stable BPM: ");

        Serial.println(bpm, 1);
      }
    }


    // -------------------------------------------------
    // Save current beat time
    // -------------------------------------------------

    lastBeatTime = now;


    // Prevent duplicate detection

    beatState = true;
  }


  // ---------------------------------------------------
  // 12. Reset beat state
  // ---------------------------------------------------

  if (filteredSignal < threshold)
  {
    beatState = false;
  }


  // ---------------------------------------------------
  // 13. Save current signal
  // ---------------------------------------------------

  previousSignal = filteredSignal;
}


// =====================================================
// CALCULATE SpO2
// =====================================================

void calculateSpO2()
{
  // SpO2 still requires a complete buffer

  if (!bufferFull)
  {
    return;
  }


  // ---------------------------------------------------
  // Calculate DC values
  // ---------------------------------------------------

  float irDC = 0;

  float redDC = 0;


  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    irDC += irBuffer[i];

    redDC += redBuffer[i];
  }


  irDC /= BUFFER_SIZE;

  redDC /= BUFFER_SIZE;


  // ---------------------------------------------------
  // Calculate AC values
  // ---------------------------------------------------

  float irAC = 0;

  float redAC = 0;


  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    irAC += fabs(irBuffer[i] - irDC);

    redAC += fabs(redBuffer[i] - redDC);
  }


  irAC /= BUFFER_SIZE;

  redAC /= BUFFER_SIZE;


  // ---------------------------------------------------
  // Check values
  // ---------------------------------------------------

  if (irDC <= 0 ||
      redDC <= 0 ||
      irAC <= 0 ||
      redAC <= 0)
  {
    return;
  }


  // ---------------------------------------------------
  // Ratio of ratios
  // ---------------------------------------------------

  float R =
      (redAC / redDC) /
      (irAC / irDC);


  // ---------------------------------------------------
  // Approximate SpO2 equation
  // ---------------------------------------------------

  float calculatedSpO2 =
      110.0 - (25.0 * R);


  // ---------------------------------------------------
  // Limit range
  // ---------------------------------------------------

  if (calculatedSpO2 > 100)
  {
    calculatedSpO2 = 100;
  }


  if (calculatedSpO2 < 70)
  {
    calculatedSpO2 = 70;
  }


  // ---------------------------------------------------
  // Smooth SpO2
  // ---------------------------------------------------

  if (spo2 == 0)
  {
    spo2 = calculatedSpO2;
  }
  else
  {
    spo2 =
        (spo2 * 0.80) +
        (calculatedSpO2 * 0.20);
  }
}


// =====================================================
// UPDATE LCD
// =====================================================

void updateLCD()
{
  static bool previousFingerState = false;

  static int previousBPM = -1;

  static int previousSpO2 = -1;


  // ---------------------------------------------------
  // No finger
  // ---------------------------------------------------

  if (!fingerDetected)
  {
    if (previousFingerState)
    {
      lcd.clear();
    }


    lcd.setCursor(0, 0);

    lcd.print("Place Finger    ");


    lcd.setCursor(0, 1);

    lcd.print("on MAX30100     ");


    previousFingerState = false;

    previousBPM = -1;

    previousSpO2 = -1;

    return;
  }


  // ---------------------------------------------------
  // Finger detected
  // ---------------------------------------------------

  if (!previousFingerState)
  {
    lcd.clear();
  }


  previousFingerState = true;


  // ---------------------------------------------------
  // BPM
  // ---------------------------------------------------

  lcd.setCursor(0, 0);

  lcd.print("BPM: ");


  if (bpm > 0)
  {
    int displayBPM = (int)bpm;

    lcd.print(displayBPM);

    lcd.print("     ");

    previousBPM = displayBPM;
  }
  else
  {
    lcd.print("--   ");

    previousBPM = -1;
  }


  // ---------------------------------------------------
  // SpO2
  // ---------------------------------------------------

  lcd.setCursor(0, 1);

  lcd.print("SpO2: ");


  if (spo2 > 0)
  {
    int displaySpO2 = (int)spo2;

    lcd.print(displaySpO2);

    lcd.print("%    ");

    previousSpO2 = displaySpO2;
  }
  else
  {
    lcd.print("--%   ");

    previousSpO2 = -1;
  }
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);


  // ---------------------------------------------------
  // I2C
  // ---------------------------------------------------

  Wire.begin(SDA_PIN, SCL_PIN);

  Wire.setClock(400000);


  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  lcd.init();

  lcd.backlight();


  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("ESP32 MAX30100");


  lcd.setCursor(0, 1);

  lcd.print("Initializing...");


  delay(1500);


  // ---------------------------------------------------
  // Initialize MAX30100
  // ---------------------------------------------------

  if (!initMAX30100())
  {
    lcd.clear();

    lcd.setCursor(0, 0);

    lcd.print("MAX30100 ERROR");


    lcd.setCursor(0, 1);

    lcd.print("Check Wiring");


    while (1)
    {
      delay(1000);
    }
  }


  // ---------------------------------------------------
  // MAX30100 ready
  // ---------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("MAX30100 OK");


  lcd.setCursor(0, 1);

  lcd.print("Place Finger");


  delay(1500);


  Serial.println();

  Serial.println("======================");

  Serial.println("MAX30100 STARTED");

  Serial.println("======================");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  // ---------------------------------------------------
  // Sample every 10 ms
  // ---------------------------------------------------

  if (millis() - lastSampleTime >= SAMPLE_INTERVAL)
  {
    lastSampleTime = millis();


    uint16_t irValue;

    uint16_t redValue;


    // -------------------------------------------------
    // Read MAX30100
    // -------------------------------------------------

    if (readFIFO(irValue, redValue))
    {
      // ------------------------------------------------
      // Serial raw data
      // ------------------------------------------------

      Serial.print("IR = ");

      Serial.print(irValue);

      Serial.print("    RED = ");

      Serial.println(redValue);


      // ------------------------------------------------
      // Finger detection
      // ------------------------------------------------

      if (irValue > 1000)
      {
        fingerDetected = true;


        // ----------------------------------------------
        // Store IR
        // ----------------------------------------------

        irBuffer[bufferIndex] =
            irValue;


        // ----------------------------------------------
        // Store RED
        // ----------------------------------------------

        redBuffer[bufferIndex] =
            redValue;


        // ----------------------------------------------
        // Move buffer index
        // ----------------------------------------------

        bufferIndex++;


        if (bufferIndex >= BUFFER_SIZE)
        {
          bufferIndex = 0;

          bufferFull = true;
        }


        // ----------------------------------------------
        // FAST BPM
        // ----------------------------------------------

        calculateHeartRate();


        // ----------------------------------------------
        // SpO2
        // ----------------------------------------------

        calculateSpO2();
      }


      // ------------------------------------------------
      // Finger removed
      // ------------------------------------------------

      else
      {
        fingerDetected = false;


        bpm = 0;

        spo2 = 0;


        bufferIndex = 0;

        bufferFull = false;


        lastBeatTime = 0;

        beatState = false;
      }
    }
  }


  // ---------------------------------------------------
  // LCD update every 500 ms
  // ---------------------------------------------------

  static unsigned long lastLCDUpdate = 0;


  if (millis() - lastLCDUpdate >= 500)
  {
    lastLCDUpdate = millis();

    updateLCD();
  }
}