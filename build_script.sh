#!/bin/bash
set -ev

# Run unit tests
mkdir build
cd build
cmake .. -G Ninja
ninja ctest

# Copy lib onto sketch
cp ../lib/inc/dht11lib.h ../lib/src/dht11lib.c ../arduino/dht11

# Compile sketch
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
./bin/arduino-cli version # for debug
./bin/arduino-cli core install arduino:avr
./bin/arduino-cli compile -b arduino:avr:uno ../arduino/dht11
