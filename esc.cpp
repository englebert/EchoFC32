/*
 * Filename: esc.cpp
 * Description:
 * ESC Driver Logic
 *
 */

#include "Arduino.h"
#include "esc.h"
#include "EchoFC32.h"
#include "config.h"

rmt_data_t  esc_dshotPacket[16];
rmt_obj_t*  esc[]               = {NULL, NULL, NULL, NULL};
uint8_t     esc_pin[]           = {PIN_ESC1, PIN_ESC2, PIN_ESC3, PIN_ESC4};
uint16_t    esc_val[]           = {0, 0, 0, 0};

// Testing
bool reverse_esc = false;
bool forward_esc = false;

/**** ESC Related ****/
TaskHandle_t TaskESC;

bool armed = false;
bool command_trigger = false;
uint16_t command_val = 0;


void escInit() {
    for(int i = 0; i < ESC_MAX_COUNT; i++) {
        if((esc[i] = rmtInit(esc_pin[i], true, RMT_MEM_64)) == NULL)
            Serial.printf("ESC [%i] FAILED!\n", i);
        else
            Serial.printf("ESC [%i] OK!\n", i);

        float tick = rmtSetTick(esc[i], ESC_SAMPLE_RATE);
        Serial.printf("ESC [%i] at PIN [%i] - Tick: %fns\n", i, esc_pin[i], tick);
    }
}


void taskESC(void *pvParameters) {
    // Infinite loop
    while(true) {
        if(armed == true || command_trigger == true) {
            if(reverse_esc && command_trigger) {
                delay(9);
                for(int i=10; i > 0; i--) {
                    delay(1);
                    txDshot(ESC1, 0, true);
                    txDshot(ESC2, 0, true);
                    txDshot(ESC3, 0, true);
                    txDshot(ESC4, 0, true);
                }
                delay(1);
                for(int i=10; i > 0; i--) {
                    delay(1);
                    txDshot(ESC1, 21, true);
                    txDshot(ESC2, 21, true);
                    txDshot(ESC3, 21, true);
                    txDshot(ESC4, 21, true);
                }
                delay(1);
                command_trigger = false;
                reverse_esc = false;
            }
            if(forward_esc && command_trigger) {
                delay(9);
                for(int i=10; i > 0; i--) {
                    delay(1);
                    txDshot(ESC1, 0, true);
                    txDshot(ESC2, 0, true);
                    txDshot(ESC3, 0, true);
                    txDshot(ESC4, 0, true);
                }
                delay(1);
                for(int i=10; i > 0; i--) {
                    delay(1);
                    txDshot(ESC1, 20, true);
                    txDshot(ESC2, 20, true);
                    txDshot(ESC3, 20, true);
                    txDshot(ESC4, 20, true);
                }
                delay(1);
                command_trigger = false;
                forward_esc = false;
            }

            if(command_trigger) {
                delay(9);
                for(int i=10; i > 0; i--) {
                    delay(1);
                    txDshot(ESC1, command_val, true);
                    txDshot(ESC2, command_val, true);
                    txDshot(ESC3, command_val, true);
                    txDshot(ESC4, command_val, true);
                }
                delay(1);
                command_trigger = false;
            } else {
                txDshot(ESC1, esc_val[0], false);
                txDshot(ESC2, esc_val[1], false);
                txDshot(ESC3, esc_val[2], false);
                txDshot(ESC4, esc_val[3], false);
                delay(1);
            }
        } else {
            delay(1);
        }
    }
}


// DSHOT Protocol to ESC. Very hyper sensitive on timing. One slight change in this section of the 
// code will changed the behaviour of the timing. Every change on this txDshot must re-test all 
// the possibilities.
void txDshot(uint8_t esc_id, uint16_t value, bool telemetry) {
    uint16_t dshot_packet;
    dshot_packet = (value << 1) | telemetry;

    int csum = 0;
    int csum_data = dshot_packet;
    for(int i = 0; i < 3; i++) {
        csum ^= csum_data;
        csum_data >>= 4;
    }
    csum &= 0xf;
    dshot_packet = (dshot_packet << 4) | csum;

    // Ref:
    // https://github.com/JyeSmith/dshot-esc-tester
    //
    // durations are for dshot600
    // https://blck.mn/2016/11/dshot-the-new-kid-on-the-block/
    // Bit length (total timing period) is 1.67 microseconds (T0H + T0L or T1H + T1L).
    // For a bit to be 1, the pulse width is 1250 nanoseconds (T1H – time the pulse is high for a bit value of ONE)
    // For a bit to be 0, the pulse width is 625 nanoseconds (T0H – time the pulse is high for a bit value of ZERO)
    // Using DSHOT150 - As the beginning of the Proof Of Concept
    for(int i = 0; i < 16; i++) {
        // T1H + T1L
        if(dshot_packet & 0x8000) {
            esc_dshotPacket[i].level0    = 1;
            esc_dshotPacket[i].duration0 = 395;
            esc_dshotPacket[i].level1    = 0;
            esc_dshotPacket[i].duration1 = 130;

        // T0H + T0L
        } else {
            esc_dshotPacket[i].level0    = 1;
            esc_dshotPacket[i].duration0 = 180;
            esc_dshotPacket[i].level1    = 0;
            esc_dshotPacket[i].duration1 = 335;
        }

        dshot_packet <<= 1;
    }

    rmtWrite(esc[esc_id - 1], esc_dshotPacket, 16);
}
