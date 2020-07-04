#include <Arduino.h>
#include "dht11lib.h"

/**
 * Converts 2 bytes to a signed 16-bit integer
 * Parameters:
 *  higherByte: the upper byte of the data
 *  loweRByte: the lower byte of the data
 */
static int16_t twoBytesToSignedInt16(uint8_t higherByte, uint8_t lowerByte) {
  uint16_t uintResult = higherByte;
  uintResult <<= 8;
  uintResult |= lowerByte;
  return (int16_t)uintResult;
}

static int pollWithTimeout(int dht11Pin, int awaitedValue, unsigned long timeout) {
  unsigned long tStart = micros();

  while (digitalRead(dht11Pin) != awaitedValue) {
    // This will work even if micros() rolls over due to overflow/underflow semantics
    unsigned long tElapsed = micros() - tStart;
    if (tElapsed > timeout) {
      return;
    }
  }

  return DHT11_SUCCESS;
}

int Dht11_ReadBlocking(int dht11Pin, int16_t *humidityEncoded, int16_t *temperatureEncoded) {
  int statusCode;
  
  // Start condition: pull down for several milliseconds, then up for several microseconds
  pinMode(dht11Pin, OUTPUT);
  digitalWrite(dht11Pin, LOW);
  delay(DHT11_MCU_PULL_DOWN_DURATION_MS);

  // Yield control to DHT11 and wait for the rise time
  pinMode(dht11Pin, INPUT_PULLUP);

  // Wait for initial falling edge (20-40 us)
  statusCode = pollWithTimeout(dht11Pin, LOW, DHT11_MAX_RESPONSE_LATENCY_US);
  if (statusCode != DHT11_SUCCESS) {
    return statusCode;
  }

  // Wait for initial rising edge (80 us)
  statusCode = pollWithTimeout(dht11Pin, HIGH, DHT11_MAX_RESPONSE_PULSE_HALF_WIDTH_US);
  if (statusCode != DHT11_SUCCESS) {
    return statusCode;
  }

  // Wait for last falling edge before data (80 us)
  statusCode = pollWithTimeout(dht11Pin, LOW, DHT11_MAX_RESPONSE_PULSE_HALF_WIDTH_US);
  if (statusCode != DHT11_SUCCESS) {
    return statusCode;
  }

  // Read
  uint8_t data[DHT11_N_BYTES] = { 0 };
  for (int byteNo = 0; byteNo < DHT11_N_BYTES; byteNo++) {
    for (int bitNo = 7; bitNo >= 0; bitNo--) {
      // Wait for inter-sample period (50 us)
      statusCode = pollWithTimeout(dht11Pin, HIGH, DHT11_MAX_BIT_SEPARATION_US);
      if (statusCode != DHT11_SUCCESS) {
        return statusCode;
      }
  
      // Sample how long the line is high
      int ti = micros();
      statusCode = pollWithTimeout(dht11Pin, LOW, DHT11_MAX_BIT_DURATION_US);
      int tf = micros();
      if (statusCode != DHT11_SUCCESS) {
        return statusCode;
      }

      if (tf - ti > 40) {
        data[byteNo] |= (1 << bitNo);
      }
    }
  }

  // Populate result and return true if the checksum matches,
  // otherwise return false and leave the result alone
  // Checksum is the last byte of the sum of the data
  if (data[4] == (data[0] + data[1] + data[2] + data[3])) {
    *humidityEncoded = twoBytesToSignedInt16(data[0], data[1]);
    *temperatureEncoded = twoBytesToSignedInt16(data[2], data[3]);
    return DHT11_SUCCESS;
  } else {
    return DHT11_CRC_ERROR;
  }
}

int Dht11_ReadAveragedBlocking(int dht11Pin, int nSamplesLog2, int16_t *humidityEncoded, int16_t *temperatureEncoded) {
  if (nSamplesLog2 > DHT11_MAX_AVG_EXPONENT) {
    return DHT11_BAD_PARAMETER;
  }
  
  int nSamples = 1 << nSamplesLog2;
  uint32_t humidityTotal = 0;
  uint32_t temperatureTotal = 0;
  for (int i = 0; i < nSamples; i++) {
    if (i > 0) {
      delay(DHT11_MIN_TIME_BETWEEN_SAMPLES_MS);
    }
    
    int16_t humidity, temperature;
    int statusCode = Dht11_ReadBlocking(dht11Pin, &humidity, &temperature);
    if (statusCode != DHT11_SUCCESS) {
      return statusCode;
    }
    humidityTotal += humidity;
    temperatureTotal += temperature;
  }

  humidityTotal >>= nSamplesLog2;
  temperatureTotal >>= nSamplesLog2;

  *humidityEncoded = (int16_t)humidityTotal;
  *temperatureEncoded = (int16_t)temperatureTotal;

  return DHT11_SUCCESS;
}
