#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BME680.h>
#include <RTClib.h>

// ---------------- PIN CONFIG ----------------
#define UV_PIN A0
#define LORA_SS 10
#define LORA_RST 9
#define LORA_DIO0 2

// ---------------- OBJECTS ----------------
Adafruit_BME680 bme;
RTC_DS3231 rtc;

// ---------------- SAMPLING ----------------
uint32_t normalInterval = 10000;  // 10 sec
uint32_t fastInterval   = 2000;   // 2 sec
uint32_t currentInterval = 10000;
uint32_t lastSampleTime = 0;

int prevUV = 0;
const int CHANGE_THRESHOLD = 8;

// ---------------- BUFFER ----------------
#define BUFFER_SIZE 500

struct TelemetryData {
  uint32_t timestamp;
  uint16_t sequence;
  uint16_t uv;            // mW/cm² x1000
  int16_t temperature;    // °C x100
  uint16_t humidity;      // % x100
  uint32_t pressure;      // hPa x100
};

TelemetryData buffer[BUFFER_SIZE];

uint16_t writeIndex = 0;
uint16_t readIndex  = 0;
uint16_t sequenceNumber = 0;

// ---------------- CRC16 ----------------
uint16_t crc16(uint8_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);
  Wire.begin();
  analogReadResolution(12);

  if (!bme.begin()) {
    Serial.println("BME680 ERROR!");
    while (1);
  }

  if (!rtc.begin()) {
    Serial.println("RTC ERROR!");
    while (1);
  }

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa ERROR!");
    while (1);
  }

  Serial.println("Telemetry TX Ready");
}

// ---------------- LOOP ----------------
void loop() {

  uint32_t now = millis();

  // -------- DATA ACQUISITION (ALWAYS RUNS) --------
  if (now - lastSampleTime >= currentInterval) {

    lastSampleTime = now;

    int uvRaw = analogRead(UV_PIN);

    if (abs(uvRaw - prevUV) > CHANGE_THRESHOLD)
      currentInterval = fastInterval;
    else
      currentInterval = normalInterval;

    prevUV = uvRaw;

    if (!bme.performReading())
      return;

    DateTime time = rtc.now();

    float uvIntensity = (uvRaw * 3.3 / 4095.0);
    uvIntensity *= 0.05;  // adjust if needed

    buffer[writeIndex].timestamp = time.unixtime();
    buffer[writeIndex].sequence = sequenceNumber++;
    buffer[writeIndex].uv = (uint16_t)(uvIntensity * 1000);
    buffer[writeIndex].temperature = (int16_t)(bme.temperature * 100);
    buffer[writeIndex].humidity = (uint16_t)(bme.humidity * 100);
    buffer[writeIndex].pressure = (uint32_t)((bme.pressure / 100.0) * 100);

    writeIndex = (writeIndex + 1) % BUFFER_SIZE;
  }

  // -------- CONTACT WINDOW CONTROL --------
  static bool transmitting = false;
  static uint32_t phaseStart = 0;

  // Start transmission after 2 minutes
  if (!transmitting && now > 120000) {
    transmitting = true;
    phaseStart = now;
    Serial.println("\n===== CONTACT WINDOW STARTED =====\n");
  }

  if (transmitting) {
    transmitOnePacket();
  }

  if (transmitting && (now - phaseStart > 60000)) {
    transmitting = false;
    Serial.println("\n===== CONTACT WINDOW ENDED =====\n");
  }
}

// ---------------- TRANSMIT ONE PACKET ----------------
void transmitOnePacket() {

  if (readIndex == writeIndex)
    return;  // nothing new to send

  uint8_t packet[32];
  int index = 0;

  packet[index++] = 0x01;   // Version
  packet[index++] = 0x10;   // Node ID

  memcpy(&packet[index], &buffer[readIndex].sequence, 2); index += 2;
  memcpy(&packet[index], &buffer[readIndex].timestamp, 4); index += 4;
  memcpy(&packet[index], &buffer[readIndex].uv, 2); index += 2;
  memcpy(&packet[index], &buffer[readIndex].temperature, 2); index += 2;
  memcpy(&packet[index], &buffer[readIndex].humidity, 2); index += 2;
  memcpy(&packet[index], &buffer[readIndex].pressure, 4); index += 4;

  uint16_t crc = crc16(packet, index);
  memcpy(&packet[index], &crc, 2); index += 2;

  // ---- LoRa TX ----
  LoRa.beginPacket();
  LoRa.write(packet, index);
  LoRa.endPacket();

  // ---- PRINT HEX PACKET ----
  Serial.print("Packet HEX: ");
  for (int i = 0; i < index; i++) {
    if (packet[i] < 16) Serial.print("0");
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // ---- PRINT DECODED VALUES ----
  Serial.print("Seq: ");
  Serial.print(buffer[readIndex].sequence);

  Serial.print(" | UV: ");
  Serial.print(buffer[readIndex].uv / 1000.0);
  Serial.print(" mW/cm²");

  Serial.print(" | Temp: ");
  Serial.print(buffer[readIndex].temperature / 100.0);
  Serial.print(" °C");

  Serial.print(" | Hum: ");
  Serial.print(buffer[readIndex].humidity / 100.0);
  Serial.print(" %");

  Serial.print(" | Press: ");
  Serial.print(buffer[readIndex].pressure / 100.0);
  Serial.println(" hPa");

  Serial.println("--------------------------------------------------");

  readIndex = (readIndex + 1) % BUFFER_SIZE;
}
