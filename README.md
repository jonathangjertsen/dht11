# DHT11 Temperature and Humidity sensor driver library

[![Build Status](https://travis-ci.com/jonathangjertsen/dht11.svg?branch=master)](https://travis-ci.com/jonathangjertsen/dht11)

## Arduino sketch

To use the sketch, open `Arduino/dht11/dht11.ino` and go to `Sketch --> Add File`, then add `lib/inc/dht11.h` and `lib/src/dht11.c`

## Standalone library

The library is written in C. It currently uses the Arduino API - if you want to use it in another project, you will have to provide a mock of the digital pin API that Arduino.h provides:

```
unsigned long micros(void);
int digitalRead(int pin);
void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
void delay(int duration);

const int OUTPUT;
const int LOW;
const int HIGH;
const int INPUT;
const int INPUT_PULLUP;
```