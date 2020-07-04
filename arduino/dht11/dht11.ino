#include "dht11lib.h"

// Definitions for this app
#define DHT11_APP_PIN 4
#define DHT11_APP_AVG_EXPONENT 4

void printSetup(void) {
  Serial.println("SETUP START");
  Serial.print("AVG_EXPONENT=");
  Serial.println(DHT11_APP_AVG_EXPONENT);
  Serial.print("MIN_WAIT_S=");
  Serial.println((DHT11_TYP_SAMPLE_DURATION_MS + DHT11_MIN_TIME_BETWEEN_SAMPLES_MS * (1 << DHT11_APP_AVG_EXPONENT) + 500) / 1000);
  Serial.println("SETUP END");
  Serial.println("MEASUREMENT START");
}

void printResult(int16_t humidity, int16_t temperature, int status)
{
  Serial.println("MEASUREMENT END");
  Serial.println("RESULTS START");
  Serial.print("STATUS=");
  Serial.println(status);
  Serial.print("HUMIDITY=");
  Serial.println((humidity + (1 << 7)) >> 8);
  Serial.print("TEMPERATURE=");
  Serial.println((temperature + (1 << 7)) >> 8);
  Serial.println("RESULTS END");
}

void setup() {
  pinMode(DHT11_APP_PIN, INPUT_PULLUP);

  Serial.begin(19200);

  // Print initial line break so that the client can read line-by-line and check exactly against the "BOOTED" token
  Serial.println("");
  Serial.println("BOOTED");
}

void loop()
{
  int16_t humidity = 1234;
  int16_t temperature = 1234;

  printSetup();
  int status = Dht11_ReadAveragedBlocking(DHT11_APP_PIN, DHT11_APP_AVG_EXPONENT, &humidity, &temperature);
  printResult(humidity, temperature, status);
  delay(DHT11_MIN_TIME_BETWEEN_SAMPLES_MS);
}
