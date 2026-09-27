#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <BluetoothSerial.h>
#include <math.h>

// =====================================================
// ESP32 HEALTH MONITOR
// MAX30100 + DHT11 + LCD + RELAY + BLUETOOTH
// =====================================================

// ================= I2C =================
#define SDA_PIN 21
#define SCL_PIN 22

// ================= MAX30100 =================
#define MAX30100_ADDR 0x57

#define REG_INT_ENABLE      0x01
#define REG_FIFO_WR_PTR     0x02
#define REG_OVF_COUNTER     0x03
#define REG_FIFO_RD_PTR     0x04
#define REG_FIFO_DATA       0x05
#define REG_MODE_CONFIG     0x06
#define REG_SPO2_CONFIG     0x07
#define REG_LED_CONFIG      0x09
#define REG_PART_ID         0xFF

// ================= DHT11 =================
#define DHT_PIN 27

DHTesp dht;

// ================= RELAY =================
#define RELAY_PIN 26

// Relay limits
#define HIGH_BPM 95.0
#define HIGH_TEMPERATURE 37.0

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= BLUETOOTH =================
BluetoothSerial SerialBT;


// =====================================================
// MAX30100 SAMPLING
// =====================================================

unsigned long lastSampleTime = 0;

const unsigned long SAMPLE_INTERVAL = 10;


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
// FINGER
// =====================================================

bool fingerDetected = false;


// =====================================================
// TEMPERATURE
// =====================================================

float temperature = 0;


// =====================================================
// LCD
// =====================================================

int lcdPage = 0;

unsigned long lastLCDPageChange = 0;

const unsigned long LCD_PAGE_TIME = 2000;


// =====================================================
// BLUETOOTH
// =====================================================

unsigned long lastBluetoothUpdate = 0;

const unsigned long BLUETOOTH_UPDATE_TIME = 2000;


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

  if (partID != 0x11)
  {
    Serial.println("MAX30100 NOT DETECTED!");

    return false;
  }

  Serial.println("MAX30100 detected!");

  // Reset
  writeRegister(REG_MODE_CONFIG, 0x40);

  delay(100);

  // Clear FIFO
  writeRegister(REG_FIFO_WR_PTR, 0x00);

  writeRegister(REG_OVF_COUNTER, 0x00);

  writeRegister(REG_FIFO_RD_PTR, 0x00);

  // Disable interrupts
  writeRegister(REG_INT_ENABLE, 0x00);

  // 16-bit ADC
  // 100 Hz sampling
  writeRegister(REG_SPO2_CONFIG, 0x47);

  // RED = 11mA
  // IR = 11mA
  writeRegister(REG_LED_CONFIG, 0x77);

  // SpO2 mode
  writeRegister(REG_MODE_CONFIG, 0x03);

  delay(100);

  return true;
}


// =====================================================
// HEART RATE CALCULATION
// =====================================================

void calculateHeartRate()
{
  static float baseline = 0;
  static float filteredSignal = 0;
  static float previousSignal = 0;

  static float signalMin = 0;
  static float signalMax = 0;

  static bool initialized = false;

  int latestIndex =
      (bufferIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE;

  float irValue = irBuffer[latestIndex];

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

  // Remove DC component
  baseline =
      (baseline * 0.95) +
      (irValue * 0.05);

  float acSignal = irValue - baseline;

  // Smooth signal
  filteredSignal =
      (filteredSignal * 0.75) +
      (acSignal * 0.25);

  // Find signal minimum
  if (filteredSignal < signalMin)
  {
    signalMin = filteredSignal;
  }

  // Find signal maximum
  if (filteredSignal > signalMax)
  {
    signalMax = filteredSignal;
  }

  // Slowly reduce old values
  signalMin *= 0.995;

  signalMax *= 0.995;

  float signalRange =
      signalMax - signalMin;

  if (signalRange < 20)
  {
    previousSignal = filteredSignal;

    return;
  }

  // Adaptive threshold
  float threshold =
      signalMin + (signalRange * 0.55);

  unsigned long now = millis();

  bool enoughTimePassed = true;

  if (lastBeatTime > 0)
  {
    if ((now - lastBeatTime) < 350)
    {
      enoughTimePassed = false;
    }
  }

  // Heartbeat detection
  if (!beatState &&
      enoughTimePassed &&
      previousSignal < threshold &&
      filteredSignal >= threshold)
  {
    if (lastBeatTime > 0)
    {
      unsigned long interval =
          now - lastBeatTime;

      if (interval >= 350 &&
          interval <= 1800)
      {
        float newBPM =
            60000.0 / interval;

        // Smooth BPM
        if (bpm == 0)
        {
          bpm = newBPM;
        }
        else
        {
          float difference =
              fabs(newBPM - bpm);

          if (difference < 15)
          {
            bpm =
                (bpm * 0.70) +
                (newBPM * 0.30);
          }
          else if (difference < 30)
          {
            bpm =
                (bpm * 0.80) +
                (newBPM * 0.20);
          }
          else
          {
            bpm =
                (bpm * 0.90) +
                (newBPM * 0.10);
          }
        }

        Serial.print("BPM: ");
        Serial.println(bpm, 1);
      }
    }

    lastBeatTime = now;

    beatState = true;
  }

  // Reset heartbeat state
  if (filteredSignal < threshold)
  {
    beatState = false;
  }

  previousSignal = filteredSignal;
}


// =====================================================
// SpO2 CALCULATION
// =====================================================

void calculateSpO2()
{
  if (!bufferFull)
  {
    return;
  }

  float irDC = 0;
  float redDC = 0;

  // DC component
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    irDC += irBuffer[i];

    redDC += redBuffer[i];
  }

  irDC /= BUFFER_SIZE;
  redDC /= BUFFER_SIZE;

  float irAC = 0;
  float redAC = 0;

  // AC component
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    irAC += abs(irBuffer[i] - irDC);

    redAC += abs(redBuffer[i] - redDC);
  }

  irAC /= BUFFER_SIZE;
  redAC /= BUFFER_SIZE;

  if (irDC <= 0 ||
      redDC <= 0 ||
      irAC <= 0)
  {
    return;
  }

  float R =
      (redAC / redDC) /
      (irAC / irDC);

  // Approximate SpO2
  float calculatedSpO2 =
      110.0 - (25.0 * R);

  if (calculatedSpO2 > 100)
  {
    calculatedSpO2 = 100;
  }

  if (calculatedSpO2 < 70)
  {
    calculatedSpO2 = 70;
  }

  // Smooth SpO2
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
// READ DHT11
// =====================================================

void readDHT()
{
  TempAndHumidity data =
      dht.getTempAndHumidity();

  if (isnan(data.temperature))
  {
    Serial.println("DHT11 ERROR");

    return;
  }

  temperature = data.temperature;

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");
}


// =====================================================
// RELAY CONTROL
// =====================================================

void controlRelay()
{
  bool highBPM = false;
  bool highTemperature = false;

  // BPM condition
  if (fingerDetected && bpm >= HIGH_BPM)
  {
    highBPM = true;
  }

  // Temperature condition
  if (temperature >= HIGH_TEMPERATURE)
  {
    highTemperature = true;
  }

  // Relay ON if either condition is high
  if (highBPM || highTemperature)
  {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
  }
}


// =====================================================
// LCD DISPLAY
// =====================================================

void updateLCD()
{
  lcd.clear();

  // =========================================
  // PAGE 0 - BPM + SpO2
  // =========================================

  if (lcdPage == 0)
  {
    if (!fingerDetected)
    {
      lcd.setCursor(0, 0);
      lcd.print("Place Finger");

      lcd.setCursor(0, 1);
      lcd.print("on MAX30100");

      return;
    }

    lcd.setCursor(0, 0);

    lcd.print("BPM: ");

    if (bpm > 0)
    {
      lcd.print((int)bpm);
    }
    else
    {
      lcd.print("--");
    }

    lcd.setCursor(0, 1);

    lcd.print("SpO2: ");

    if (spo2 > 0)
    {
      lcd.print((int)spo2);
      lcd.print("%");
    }
    else
    {
      lcd.print("--%");
    }
  }

  // =========================================
  // PAGE 1 - TEMPERATURE
  // =========================================

  else if (lcdPage == 1)
  {
    lcd.setCursor(0, 0);

    lcd.print("Temperature");

    lcd.setCursor(0, 1);

    lcd.print(temperature, 1);

    lcd.print((char)223);

    lcd.print("C");
  }
}


// =====================================================
// BLUETOOTH DISPLAY
// =====================================================

void sendBluetoothData()
{
  SerialBT.println();
  SerialBT.println("====================");

  SerialBT.println("HEALTH MONITOR");

  SerialBT.println("====================");

  // BPM
  if (fingerDetected)
  {
    SerialBT.print("BPM: ");

    if (bpm > 0)
    {
      SerialBT.println(bpm, 1);
    }
    else
    {
      SerialBT.println("--");
    }

    // SpO2
    SerialBT.print("SpO2: ");

    if (spo2 > 0)
    {
      SerialBT.print(spo2, 1);
      SerialBT.println("%");
    }
    else
    {
      SerialBT.println("--");
    }
  }
  else
  {
    SerialBT.println("BPM: Place Finger");
    SerialBT.println("SpO2: --");
  }

  // Temperature
  SerialBT.print("Temperature: ");
  SerialBT.print(temperature, 1);
  SerialBT.println(" C");

  // High value/status
  SerialBT.println("--------------------");

  if (fingerDetected && bpm >= HIGH_BPM)
  {
    SerialBT.println("HIGH BPM");
    SerialBT.println("BPM >= 95");
  }

  if (temperature >= HIGH_TEMPERATURE)
  {
    SerialBT.println("HIGH TEMPERATURE");
    SerialBT.println("TEMP >= 37 C");
  }

  if ((!fingerDetected || bpm < HIGH_BPM) &&
      temperature < HIGH_TEMPERATURE)
  {
    SerialBT.println("VALUES NORMAL");
  }

  // Relay status
  if (digitalRead(RELAY_PIN) == HIGH)
  {
   // SerialBT.println("RELAY: ON");
  }
  else
  {
   // SerialBT.println("RELAY: OFF");
  }

  SerialBT.println("====================");
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // LCD
  lcd.init();

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("HEALTH MONITOR");

  lcd.setCursor(0, 1);

  lcd.print("Initializing...");

  delay(1500);

  // DHT11
  dht.setup(DHT_PIN, DHTesp::DHT11);

  // Relay
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);

  // Bluetooth
  SerialBT.begin("HEALTH_MONITOR");

  Serial.println("Bluetooth Started");

  Serial.println("Device: HEALTH_MONITOR");

  // MAX30100
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

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("MAX30100 OK");

  lcd.setCursor(0, 1);

  lcd.print("Place Finger");

  delay(1500);

  Serial.println();
  Serial.println("======================");
  Serial.println("HEALTH MONITOR STARTED");
  Serial.println("======================");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  // ===================================================
  // MAX30100 SAMPLE
  // ===================================================

  if (millis() - lastSampleTime >= SAMPLE_INTERVAL)
  {
    lastSampleTime = millis();

    uint16_t irValue;
    uint16_t redValue;

    if (readFIFO(irValue, redValue))
    {
      // Finger detection
      if (irValue > 1000)
      {
        fingerDetected = true;

        // Store samples
        irBuffer[bufferIndex] = irValue;

        redBuffer[bufferIndex] = redValue;

        bufferIndex++;

        if (bufferIndex >= BUFFER_SIZE)
        {
          bufferIndex = 0;

          bufferFull = true;
        }

        // BPM
        calculateHeartRate();

        // SpO2
        calculateSpO2();
      }
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


  // ===================================================
  // DHT11 EVERY 2 SECONDS
  // ===================================================

  static unsigned long lastDHTRead = 0;

  if (millis() - lastDHTRead >= 2000)
  {
    lastDHTRead = millis();

    readDHT();
  }


  // ===================================================
  // RELAY CONTROL
  // ===================================================

  controlRelay();


  // ===================================================
  // LCD PAGE CHANGE
  // ===================================================

  if (millis() - lastLCDPageChange >= LCD_PAGE_TIME)
  {
    lastLCDPageChange = millis();

    lcdPage++;

    if (lcdPage > 1)
    {
      lcdPage = 0;
    }

    updateLCD();
  }


  // First LCD update
  static bool firstLCD = true;

  if (firstLCD)
  {
    firstLCD = false;

    updateLCD();
  }


  // ===================================================
  // BLUETOOTH UPDATE
  // ===================================================

  if (millis() - lastBluetoothUpdate >= BLUETOOTH_UPDATE_TIME)
  {
    lastBluetoothUpdate = millis();

    sendBluetoothData();
  }
}