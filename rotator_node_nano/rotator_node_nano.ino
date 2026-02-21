/*
 * Arduino Rotátor - node NANO (verze 2.0)
 * Čtení potenciometru lokálně a odesílání ADC hodnoty po 1-wire UART (open-collector)
 */

#include <SoftwareSerial.h>

#define POT_PIN A0
#define TX_PIN 8
#define RX_UNUSED_PIN 9

#define DEBUG_SERIAL 1

constexpr long LINK_BAUD = 9600;
constexpr uint8_t OVERSAMPLE_COUNT = 32;
constexpr float EMA_ALPHA = 0.2f;
constexpr uint16_t SEND_PERIOD_MS = 50; // 20 Hz

// Inverted logic so DATA line is standard UART (idle HIGH) through NPN.
SoftwareSerial linkSerial(RX_UNUSED_PIN, TX_PIN, true);

float emaAdc = 0.0f;
bool emaInitialized = false;
unsigned long lastSendMs = 0;

uint8_t computeCrc(const char *payload) {
  uint8_t crc = 0;
  while (*payload) {
    crc ^= static_cast<uint8_t>(*payload++);
  }
  return crc;
}

int readAdcOversampled() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < OVERSAMPLE_COUNT; i++) {
    sum += analogRead(POT_PIN);
    delayMicroseconds(200);
  }
  return static_cast<int>(sum / OVERSAMPLE_COUNT);
}

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW); // Inverted UART: idle LOW on TX => NPN OFF, DATA pulled HIGH.
  delay(50);
  linkSerial.begin(LINK_BAUD);
#if DEBUG_SERIAL
  Serial.begin(9600);
  Serial.println("NANO v2: link up");
#endif
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendMs < SEND_PERIOD_MS) {
    return;
  }
  lastSendMs = now;

  int adc = readAdcOversampled();
  if (!emaInitialized) {
    emaAdc = static_cast<float>(adc);
    emaInitialized = true;
  } else {
    emaAdc = emaAdc * (1.0f - EMA_ALPHA) + adc * EMA_ALPHA;
  }

  int adcFiltered = static_cast<int>(emaAdc + 0.5f);
  if (adcFiltered < 0) adcFiltered = 0;
  if (adcFiltered > 1023) adcFiltered = 1023;

  char payload[16];
  snprintf(payload, sizeof(payload), "P,%d", adcFiltered);
  uint8_t crc = computeCrc(payload);

  char line[24];
  snprintf(line, sizeof(line), "%s,%u\n", payload, crc);
  linkSerial.print(line);
#if DEBUG_SERIAL
  Serial.print("TX: ");
  Serial.print(line);
#endif
}
