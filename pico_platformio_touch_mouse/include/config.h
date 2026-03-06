/*
 * config.h — Framework include aggregator.
 *
 * Provides Arduino.h and SPI.h to all translation units via a single
 * include.  The SPI peripheral pin mapping is configured in platformio.ini
 * via build flags (PICO_DEFAULT_SPI_*).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>

#endif // CONFIG_H