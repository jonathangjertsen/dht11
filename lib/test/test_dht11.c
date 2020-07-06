#include "unity.h"

#include "dht11lib.h"

#include "stdint.h"
#include "string.h"

#define DHT11_PIN 1
#define DHT11_MIN_REQUEST_DURATION_US 18000
#define DHT11_HOLD_LOW_US 80
#define DHT11_HOLD_HIGH_US 80
#define DHT11_BETWEEN_BITS_US 50
#define DHT11_0_DURATION_US 20
#define DHT11_1_DURATION_US 70

#define N_DATAS 8
#define DATA_N_BITS_PER (DHT11_N_BYTES * 8)
#define DATA_N_BITS_TOTAL (DATA_N_BITS_PER * N_DATAS)

#define DEBUG_TEST 0
#if DEBUG_TEST
#define DEBUG printf
#else
#define DEBUG(...)
#endif

typedef enum {
    INACTIVE = 0,
    MCU_HOLDS_LOW = 1,
    MCU_WAITS_FOR_RESPONSE = 2,
    SENSOR_HOLDS_LOW = 3,
    SENSOR_HOLDS_HIGH = 4,
    BETWEEN_BITS = 5,
    IN_BIT = 6
} sensorModelState_t;

typedef struct {
    int pinState;
    int pinMode;
    unsigned long micros;
    unsigned long lastChangeMicros;
    int bitNo;
    int data[DATA_N_BITS_TOTAL];
    int responseLatencyUs;
    bool sendingData;
    sensorModelState_t state;
} sensorModel_t;
static sensorModel_t g_model;

void setModelState(int state)
{
    DEBUG("%u: Model state update: %d -> %d\n", g_model.micros, g_model.state, state);
    if (state != INACTIVE)
    {
        TEST_ASSERT_NOT_EQUAL(g_model.state, state);
    }
    g_model.state = state;
    g_model.lastChangeMicros = g_model.micros;
}

void updateModel()
{
    switch(g_model.state)
    {
        case INACTIVE:
        {
            if (g_model.pinState == LOW)
            {
                setModelState(MCU_HOLDS_LOW);
            }
            break;
        }
        case MCU_HOLDS_LOW:
        {
            if (g_model.pinState == HIGH)
            {
                if ((g_model.micros - g_model.lastChangeMicros) >= DHT11_MIN_REQUEST_DURATION_US)
                {
                    setModelState(MCU_WAITS_FOR_RESPONSE);
                }
            }
            break;
        }
        case MCU_WAITS_FOR_RESPONSE:
        {
            if ((g_model.micros - g_model.lastChangeMicros) >= g_model.responseLatencyUs)
            {
                TEST_ASSERT_EQUAL(g_model.pinMode, INPUT_PULLUP);
                g_model.pinState = LOW;
                setModelState(SENSOR_HOLDS_LOW);
            }
            break;
        }
        case SENSOR_HOLDS_LOW:
        {
            if ((g_model.micros - g_model.lastChangeMicros) >= DHT11_HOLD_LOW_US)
            {
                g_model.pinState = HIGH;
                setModelState(SENSOR_HOLDS_HIGH);
            }
            break;
        }
        case SENSOR_HOLDS_HIGH:
        {
            if ((g_model.micros - g_model.lastChangeMicros) >= DHT11_HOLD_HIGH_US)
            {
                g_model.pinState = LOW;
                setModelState(BETWEEN_BITS);
            }
            break;
        }
        case BETWEEN_BITS:
        {
            if ((g_model.micros - g_model.lastChangeMicros) >= DHT11_BETWEEN_BITS_US)
            {
                g_model.pinState = HIGH;

                if (g_model.sendingData && (g_model.bitNo > 0) && ((g_model.bitNo % DATA_N_BITS_PER) == 0))
                {
                    setModelState(INACTIVE);
                    g_model.sendingData = false;
                }
                else
                {
                    DEBUG("Sending bit %d == %d\n", g_model.bitNo, g_model.data[g_model.bitNo]);
                    setModelState(IN_BIT);
                    g_model.sendingData = true;
                }
            }
            break;
        }
        case IN_BIT:
        {
            unsigned long waitDuration;
            if(g_model.data[g_model.bitNo])
            {
                waitDuration = DHT11_1_DURATION_US;
            }
            else
            {
                waitDuration = DHT11_0_DURATION_US;
            }

            if ((g_model.micros - g_model.lastChangeMicros) >= waitDuration)
            {
                g_model.pinState = LOW;
                g_model.bitNo += 1;
                setModelState(BETWEEN_BITS);
            }
            break;
        }
        default:
        {
            TEST_FAIL_MESSAGE("Invalid state");
            break;
        }
    }
}

unsigned long micros(void)
{
    // Advance timer so that while-loops can terminate
    g_model.micros += 1;
    updateModel();

    return g_model.micros;
}

int digitalRead(int pin)
{
    TEST_ASSERT_EQUAL(DHT11_PIN, pin);
    return g_model.pinState;
}

void pinMode(int pin, int mode)
{
    TEST_ASSERT_EQUAL(DHT11_PIN, pin);
    g_model.pinMode = mode;
    if (mode == INPUT_PULLUP)
    {
        g_model.pinState = HIGH;
    }
    updateModel();
}

void digitalWrite(int pin, int value)
{
    TEST_ASSERT_EQUAL(DHT11_PIN, pin);
    g_model.pinState = value;
    updateModel();
}

void delay(int duration)
{
    g_model.micros += duration * 1000;
    updateModel();
}

void setUp(void)
{
    g_model.pinMode = INPUT;
    g_model.pinState = LOW;
    g_model.micros = 0;
    g_model.responseLatencyUs = 20;
    g_model.bitNo = 0;
    g_model.sendingData = false;
    memset(g_model.data, 0, sizeof(int) * DATA_N_BITS_TOTAL);

    setModelState(INACTIVE);
}

void tearDown(void)
{
}

void test_ReadBlocking_allHigh(void)
{
    int allHigh[DATA_N_BITS_PER] = {
        1, 1, 1, 1,  1, 1, 1, 1,
        1, 1, 1, 1,  1, 1, 1, 1,

        1, 1, 1, 1,  1, 1, 1, 1,
        1, 1, 1, 1,  1, 1, 1, 1,

        1, 1, 1, 1,  1, 1, 1, 1
    };
    memcpy(g_model.data, allHigh, sizeof(int) * DATA_N_BITS_PER);

    int16_t humidityEncoded = 1234;
    int16_t temperatureEncoded = 1234;
    int status = Dht11_ReadBlocking(DHT11_PIN, &humidityEncoded, &temperatureEncoded);

    TEST_ASSERT_EQUAL(DHT11_CHECKSUM_ERROR, status);
    TEST_ASSERT_EQUAL(1234, humidityEncoded);
    TEST_ASSERT_EQUAL(1234, temperatureEncoded);
}

void test_ReadBlocking_allLow(void)
{
    int allLow[DATA_N_BITS_PER] = {
        0, 0, 0, 0,  0, 0, 0, 0,
        0, 0, 0, 0,  0, 0, 0, 0,

        0, 0, 0, 0,  0, 0, 0, 0,
        0, 0, 0, 0,  0, 0, 0, 0,

        0, 0, 0, 0,  0, 0, 0, 0
    };
    memcpy(g_model.data, allLow, sizeof(int) * DATA_N_BITS_PER);

    int16_t humidityEncoded = 1234;
    int16_t temperatureEncoded = 1234;
    int status = Dht11_ReadBlocking(DHT11_PIN, &humidityEncoded, &temperatureEncoded);

    TEST_ASSERT_EQUAL(DHT11_ALL_LOW, status);
    TEST_ASSERT_EQUAL(1234, humidityEncoded);
    TEST_ASSERT_EQUAL(1234, temperatureEncoded);
}

void test_ReadBlocking_validChecksum(void)
{
    int validData[DATA_N_BITS_PER] = {
        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170

        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136

        1, 1, 0, 0,  0, 0, 1, 1  // (255 + 170 + 146 + 136) % 256 == 195
    };
    memcpy(g_model.data, validData, sizeof(int) * DATA_N_BITS_PER);

    int16_t humidityEncoded = 1234;
    int16_t temperatureEncoded = 1234;
    int status = Dht11_ReadBlocking(DHT11_PIN, &humidityEncoded, &temperatureEncoded);

    TEST_ASSERT_EQUAL(DHT11_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT16(0b1111111110101010, humidityEncoded);
    TEST_ASSERT_EQUAL_INT16(0b1001001010001000, temperatureEncoded);
}

void test_ReadBlocking_wrongChecksum(void)
{
    int dataWithError[DATA_N_BITS_PER] = {
        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170

        1, 0, 0, 1,  0, 0, 1, 1, // 146, but with last bit flipped => 147
        1, 0, 0, 0,  1, 0, 0, 0, // 136

        1, 1, 0, 0,  0, 0, 1, 0  // (255 + 170 + 146 + 136) % 256 == 195
    };
    memcpy(g_model.data, dataWithError, sizeof(int) * DATA_N_BITS_PER);

    int16_t humidityEncoded = 1234;
    int16_t temperatureEncoded = 1234;
    int status = Dht11_ReadBlocking(DHT11_PIN, &humidityEncoded, &temperatureEncoded);

    TEST_ASSERT_EQUAL(DHT11_CHECKSUM_ERROR, status);
    TEST_ASSERT_EQUAL_INT16(1234, humidityEncoded);
    TEST_ASSERT_EQUAL_INT16(1234, temperatureEncoded);
}

void test_ReadBlocking_timeout(void)
{
    int validData[DATA_N_BITS_PER] = {
        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170

        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136

        1, 1, 0, 0,  0, 0, 1, 1  // (255 + 170 + 146 + 136) % 256 == 195
    };
    memcpy(g_model.data, validData, sizeof(int) * DATA_N_BITS_PER);

    int16_t humidityEncoded = 1234;
    int16_t temperatureEncoded = 1234;
    g_model.responseLatencyUs = 100;
    int status = Dht11_ReadBlocking(DHT11_PIN, &humidityEncoded, &temperatureEncoded);

    TEST_ASSERT_EQUAL(DHT11_TIMEOUT, status);
    TEST_ASSERT_EQUAL_INT16(1234, humidityEncoded);
    TEST_ASSERT_EQUAL_INT16(1234, temperatureEncoded);
}

void test_ReadAveragedBlocking_validData(void)
{
    int validData[DATA_N_BITS_TOTAL] = {
        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        1, 1, 0, 0,  0, 0, 1, 1, // (255 + 170 + 146 + 136) % 256 == 195

        0, 1, 1, 1,  1, 1, 1, 1, // 127
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        0, 1, 0, 0,  0, 0, 1, 1, // (127 + 170 + 146 + 136) % 256 == 67

        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        1, 1, 0, 0,  0, 0, 1, 1, // (255 + 170 + 146 + 136) % 256 == 195

        0, 1, 1, 1,  1, 1, 1, 1, // 127
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        0, 1, 0, 0,  0, 0, 1, 1, // (127 + 170 + 146 + 136) % 256 == 67

        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        1, 1, 0, 0,  0, 0, 1, 1, // (255 + 170 + 146 + 136) % 256 == 195

        0, 1, 1, 1,  1, 1, 1, 1, // 127
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        0, 1, 0, 0,  0, 0, 1, 1, // (127 + 170 + 146 + 136) % 256 == 67

        1, 1, 1, 1,  1, 1, 1, 1, // 255
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        1, 1, 0, 0,  0, 0, 1, 1, // (255 + 170 + 146 + 136) % 256 == 195

        0, 1, 1, 1,  1, 1, 1, 1, // 127
        1, 0, 1, 0,  1, 0, 1, 0, // 170
        1, 0, 0, 1,  0, 0, 1, 0, // 146
        1, 0, 0, 0,  1, 0, 0, 0, // 136
        0, 1, 0, 0,  0, 0, 1, 1, // (127 + 170 + 146 + 136) % 256 == 67
    };
    memcpy(g_model.data, validData, sizeof(int) * DATA_N_BITS_TOTAL);

    int16_t humidityEncoded = 1234;
    int16_t temperatureEncoded = 1234;
    int status = Dht11_ReadAveragedBlocking(DHT11_PIN, 3, &humidityEncoded, &temperatureEncoded);

    TEST_ASSERT_EQUAL(DHT11_SUCCESS, status);

    // Consider making less bogus values so that it makes sense to check the values
    TEST_ASSERT_NOT_EQUAL(1234, temperatureEncoded);
    TEST_ASSERT_NOT_EQUAL(1234, temperatureEncoded);
}
