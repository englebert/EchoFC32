/*
 * Filename: config.h
 * Description:
 * Configuration Headers
 *
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define ESC1 1
#define ESC2 2
#define ESC3 3
#define ESC4 4

// The throttle contains 11-bit data sent from the flight controller to the ESC.
// It gives a resolution of 2048 values.
// The values 0-47 are reserved, 1-47 are for the telemetry settings, and 0 is for disarming.
#define DSHOT_MAX 2047
#define DSHOT_MIN 48

#define PIN_ESC1 13
#define PIN_ESC2 12
#define PIN_ESC3 14
#define PIN_ESC4 27
#define ESC_SAMPLE_RATE 12.5                  // 12.5ns sample rate. Based on 80MHz
#define ESC_MAX_COUNT 4

#endif
