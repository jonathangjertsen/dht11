#ifndef DHT11LIB_H____
#define DHT11LIB_H____

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// Time constants for the sensor
#define DHT11_MIN_TIME_BETWEEN_SAMPLES_MS 1500
#define DHT11_TYP_SAMPLE_DURATION_MS 24
#define DHT11_TIMEOUT_MARGIN_US 20
#define DHT11_MCU_PULL_DOWN_DURATION_MS 18
#define DHT11_MAX_RESPONSE_LATENCY_US (40 + DHT11_TIMEOUT_MARGIN_US)
#define DHT11_MAX_RESPONSE_PULSE_HALF_WIDTH_US (80 + DHT11_TIMEOUT_MARGIN_US)
#define DHT11_MAX_BIT_SEPARATION_US (50 + DHT11_TIMEOUT_MARGIN_US)
#define DHT11_MAX_BIT_DURATION_US (70 + DHT11_TIMEOUT_MARGIN_US)
#define DHT11_N_BYTES 5
#define DHT11_MAX_AVG_EXPONENT 14

// Error codes
#define DHT11_SUCCESS 0
#define DHT11_CRC_ERROR (1 << 0)
#define DHT11_MISSED_EDGE (1 << 1)
#define DHT11_TIMEOUT (1 << 2)
#define DHT11_BAD_PARAMETER (1 << 3)

/**
 * Performs a blocking read of the DHT11
 * Parameters:
 *   dht11Pin: the digital pin for the single-wire bus
 *   humidityEncoded: humidity (encoded with Q8.8 fixed point notation) is stored here
 *   temperatureEncoded: temperature (encoded with Q8.8 fixed point notation) is stored here
 * Returns true if the 
 */
int Dht11_ReadBlocking(int dht11Pin, int16_t *humidityEncoded, int16_t *temperatureEncoded);

/**
 * Returns the average of a number of blocking reads of the DHT11
 * Parameters:
 *   dht11Pin: the digital pin for the single-wire bus
 *   nSamplesLog2: 2^nSamplesLog2 samples will be averaged.
 *                 Maximum is set to 14 (will take a little over a day)
 *   humidityEncoded: humidity (encoded with Q8.8 fixed point notation) is stored here
 *   temperatureEncoded: temperature (encoded with Q8.8 fixed point notation) is stored here
 * Returns true if the 
 */
int Dht11_ReadAveragedBlocking(int dht11Pin, int nSamplesLog2, int16_t *humidityEncoded, int16_t *temperatureEncoded);

#ifdef __cplusplus
}
#endif

#endif
