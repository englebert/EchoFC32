/*
 * REF:
 * - https://techtutorialsx.com/2017/05/16/esp32-dual-core-execution-speedup/
 * - https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
 * - https://www.speedgoat.com/help/slrt/page/io_main/refentry_dshot_usage_notes
 * - https://backyardrobotics.eu/2018/03/14/on-esc-protocols-part-ii/
 * - https://medium.com/@reefwing/arduino-esc-tester-adding-dshot-a0c3c0dd85c1
 * - https://www.swallenhardware.io/battlebots/2019/4/20/a-developers-guide-to-dshot-escs
 * - https://oscarliang.com/setup-turtle-mode-flip-over-after-crash/
 * - https://github.com/dmrlawson/raspberrypi-dshot
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Arduino.h"

bool armed = false;
bool command_trigger = false;
uint16_t command_val = 0;

// Testing
bool reverse_esc = false;
bool forward_esc = false;

/**** ESC Related ****/
TaskHandle_t TaskESC;

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
#define ESC_T0L 400
#define ESC_T0H 134
#define ESC_T1L 199
#define ESC_T1H 335
#define ESC_MAX_COUNT 4

rmt_data_t  esc_dshotPacket[16];
rmt_obj_t*  esc[]               = {NULL, NULL, NULL, NULL};
uint8_t     esc_pin[]           = {PIN_ESC1, PIN_ESC2, PIN_ESC3, PIN_ESC4};
uint16_t    esc_val[]           = {0, 0, 0, 0};

/**** CLI Related ****/
String serial_cmd;


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


void setup() {
    Serial.begin(115200);
    delay(1000);

    escInit(); 

    xTaskCreatePinnedToCore(taskESC, "TaskESC", 10000, NULL, 1, &TaskESC, 0);

    delay(1000);

    /*
    for(int i = 0; i < 10; i++) {
        txDshot(ESC1, 21, true);
        txDshot(ESC2, 21, true);
        txDshot(ESC3, 21, true);
        txDshot(ESC4, 21, true);
        delay(1);
    }
    for(int i = 0; i < 10; i++) {
        txDshot(ESC1, 12, true);
        txDshot(ESC2, 12, true);
        txDshot(ESC3, 12, true);
        txDshot(ESC4, 12, true);
        delay(1);
    }
    */
}


void loop() {
    Serial.printf("ESC Speed: STOP\n");
    esc_val[0] = 50;
    esc_val[1] = 50;
    esc_val[2] = 50;
    esc_val[3] = 50;

    delay(5000);
    armed = true;
    esc_val[0] = 0;
    esc_val[1] = 0;
    esc_val[2] = 0;
    esc_val[3] = 0;
    
    Serial.println("Started!");
    delay(3000);

    command_trigger = true;
    command_val = 20;

    esc_val[0] = 48;
    esc_val[1] = 48;
    esc_val[2] = 48;
    esc_val[3] = 48;
    armed = true;

    delay(200);
/*
    for(int i = 1; i < 100; i++) {
        int val = i * 10 + 48;
        esc_val[0] = val;
        esc_val[1] = val;
        esc_val[2] = val;
        esc_val[3] = val;
        Serial.printf("ESC Speed: %i\n", i);

        delay(100);
    }    
    for(int i = 100; i > 1; i--) {
        int val = i * 10 + 48;
        esc_val[0] = val;
        esc_val[1] = val;
        esc_val[2] = val;
        esc_val[3] = val;
        Serial.printf("ESC Speed: %i\n", i);

        delay(100);
    } 
    esc_val[0] = 0;
    esc_val[1] = 0;
    esc_val[2] = 0;
    esc_val[3] = 0;
    armed = false;
*/
    Serial.println("CLI READY!");
    while(true) {
        if(Serial.available() > 0) {
            serial_cmd = Serial.readStringUntil('\n');
            serial_cmd.trim();

            if(serial_cmd == "spin10") {
                esc_val[0] = 200;
                esc_val[1] = 200;
                esc_val[2] = 200;
                esc_val[3] = 200;
                armed = true;
                Serial.println("spin10");
            } else if(serial_cmd == "spin20") {
                esc_val[0] = 400;
                esc_val[1] = 400;
                esc_val[2] = 400;
                esc_val[3] = 400;
                armed = true;
                Serial.println("spin20");
            } else if(serial_cmd == "stop") {
                esc_val[0] = 48;
                esc_val[1] = 48;
                esc_val[2] = 48;
                esc_val[3] = 48;
                armed = true;
                Serial.println("stop");
            } else if(serial_cmd == "halt") {
                esc_val[0] = 0;
                esc_val[1] = 0;
                esc_val[2] = 0;
                esc_val[3] = 0;
                armed = true;
                Serial.println("halt");
            } else if(serial_cmd == "reset") {
                esc_val[0] = 0;
                esc_val[1] = 0;
                esc_val[2] = 0;
                esc_val[3] = 0;
                command_trigger = true;
                armed = false;
                Serial.println("reset");
            } else if(serial_cmd == "disarm") {
                esc_val[0] = 0;
                esc_val[1] = 0;
                esc_val[2] = 0;
                esc_val[3] = 0;
                armed = false;
                Serial.println("disarm");
            } else if(serial_cmd == "reverse") {
                esc_val[0] = 21;
                esc_val[1] = 21;
                esc_val[2] = 21;
                esc_val[3] = 21;
                command_trigger = true;
                armed = false;
                Serial.println("reverse");
            } else if(serial_cmd == "forward") {
                esc_val[0] = 20;
                esc_val[1] = 20;
                esc_val[2] = 20;
                esc_val[3] = 20;
                command_trigger = true;
                armed = false;
                Serial.println("forward");
            } else if(serial_cmd == "test") {
                esc_val[0] = 400;
                esc_val[1] = 400;
                esc_val[2] = 400;
                esc_val[3] = 400;
                armed = true;
                Serial.println("Test before delay");
                delay(10000);
                Serial.println("Test after delay");
                esc_val[0] = 48;
                esc_val[1] = 48;
                esc_val[2] = 48;
                esc_val[3] = 48;
            } else if(serial_cmd == "r1") {
                reverse_esc = true;
                command_trigger = true;
                Serial.println("r1");
            } else if(serial_cmd == "f1") {
                forward_esc = true;
                command_trigger = true;
                Serial.println("f1");
            } else if(serial_cmd == "demo") {
                esc_val[0] = 0;
                esc_val[1] = 0;
                esc_val[2] = 0;
                esc_val[3] = 0;
                armed = true;
                delay(50);

                for(int i = 10; i < 40; i++) {
                    int val = i * 10 + 48;
                    esc_val[0] = val;
                    esc_val[1] = val;
                    esc_val[2] = val;
                    esc_val[3] = val;
                    Serial.printf("ESC Speed: %i\n", i);
            
                    delay(100);
                }    
                for(int i = 40; i > 10; i--) {
                    int val = i * 10 + 48;
                    esc_val[0] = val;
                    esc_val[1] = val;
                    esc_val[2] = val;
                    esc_val[3] = val;
                    Serial.printf("ESC Speed: %i\n", i);
            
                    delay(100);
                } 
                esc_val[0] = 100;
                esc_val[1] = 100;
                esc_val[2] = 100;
                esc_val[3] = 100;
                armed = true;
            } else if(serial_cmd == "rdemo") {
                Serial.println("rdemo");
                reverse_esc = true;
                command_trigger = true;
                delay(20);

                esc_val[0] = 0;
                esc_val[1] = 0;
                esc_val[2] = 0;
                esc_val[3] = 0;
                armed = true;
                delay(50);

                for(int i = 10; i < 40; i++) {
                    int val = i * 10 + 48;
                    esc_val[0] = val;
                    esc_val[1] = val;
                    esc_val[2] = val;
                    esc_val[3] = val;
                    Serial.printf("ESC Speed: %i\n", i);
            
                    delay(100);
                }    
                for(int i = 40; i > 10; i--) {
                    int val = i * 10 + 48;
                    esc_val[0] = val;
                    esc_val[1] = val;
                    esc_val[2] = val;
                    esc_val[3] = val;
                    Serial.printf("ESC Speed: %i\n", i);
            
                    delay(100);
                } 
                esc_val[0] = 100;
                esc_val[1] = 100;
                esc_val[2] = 100;
                esc_val[3] = 100;
                armed = true;

            } else if(serial_cmd == "fdemo") {
                Serial.println("fdemo");
                forward_esc = true;
                command_trigger = true;
                delay(20);

                esc_val[0] = 0;
                esc_val[1] = 0;
                esc_val[2] = 0;
                esc_val[3] = 0;
                armed = true;
                delay(50);

                for(int i = 10; i < 40; i++) {
                    int val = i * 10 + 48;
                    esc_val[0] = val;
                    esc_val[1] = val;
                    esc_val[2] = val;
                    esc_val[3] = val;
                    Serial.printf("ESC Speed: %i\n", i);
            
                    delay(100);
                }    
                for(int i = 40; i > 10; i--) {
                    int val = i * 10 + 48;
                    esc_val[0] = val;
                    esc_val[1] = val;
                    esc_val[2] = val;
                    esc_val[3] = val;
                    Serial.printf("ESC Speed: %i\n", i);
            
                    delay(100);
                } 
                esc_val[0] = 100;
                esc_val[1] = 100;
                esc_val[2] = 100;
                esc_val[3] = 100;
                armed = true;
            }
        } 
    }
}
