/*
 * REF:
 * - https://techtutorialsx.com/2017/05/16/esp32-dual-core-execution-speedup/
 * - https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
 * - https://www.speedgoat.com/help/slrt/page/io_main/refentry_dshot_usage_notes
 * - https://backyardrobotics.eu/2018/03/14/on-esc-protocols-part-ii/
 * - https://medium.com/@reefwing/arduino-esc-tester-adding-dshot-a0c3c0dd85c1
 * - https://www.swallenhardware.io/battlebots/2019/4/20/a-developers-guide-to-dshot-escs
 * - https://oscarliang.com/setup-turtle-mode-flip-over-after-crash/
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Arduino.h"

bool armed = false;
bool command_trigger = false;
uint16_t command_val = 0;

/**** ESC Related ****/
TaskHandle_t TaskESC;
rmt_obj_t* esc1 = NULL;
rmt_obj_t* esc2 = NULL;
rmt_obj_t* esc3 = NULL;
rmt_obj_t* esc4 = NULL;
rmt_data_t esc1_dshotPacket[16];
rmt_data_t esc2_dshotPacket[16];
rmt_data_t esc3_dshotPacket[16];
rmt_data_t esc4_dshotPacket[16];

float esc1Tick, esc2Tick, esc3Tick, esc4Tick;
uint16_t esc1_val, esc2_val, esc3_val, esc4_val; 

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


/**** CLI Related ****/
String serial_cmd;



void escInit() {
    if((esc1 = rmtInit(PIN_ESC1, true, RMT_MEM_64)) == NULL) {
        Serial.println("ESC1: FAILED");
    }
    if((esc2 = rmtInit(PIN_ESC2, true, RMT_MEM_64)) == NULL) {
        Serial.println("ESC2: FAILED");
    }
    if((esc3 = rmtInit(PIN_ESC3, true, RMT_MEM_64)) == NULL) {
        Serial.println("ESC3: FAILED");
    }
    if((esc4 = rmtInit(PIN_ESC4, true, RMT_MEM_64)) == NULL) {
        Serial.println("ESC4: FAILED");
    }

    esc1Tick = rmtSetTick(esc1, ESC_SAMPLE_RATE);
    esc2Tick = rmtSetTick(esc2, ESC_SAMPLE_RATE);
    esc3Tick = rmtSetTick(esc3, ESC_SAMPLE_RATE);
    esc4Tick = rmtSetTick(esc4, ESC_SAMPLE_RATE);

    Serial.printf("ESC1 Tick: %fns\n", esc1Tick);
    Serial.printf("ESC2 Tick: %fns\n", esc2Tick);
    Serial.printf("ESC3 Tick: %fns\n", esc3Tick);
    Serial.printf("ESC4 Tick: %fns\n", esc4Tick);

    esc1_val = 0;
    esc2_val = 0;
    esc3_val = 0;
    esc4_val = 0;
}


void txDshot(uint8_t esc_id, uint16_t value, bool telemetry) {
    uint16_t dshot_packet;

    if(telemetry) {
        dshot_packet = (value << 1) | 1;
    } else {
        dshot_packet = (value << 1) | 0;
    }

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
    for(int i = 0; i < 16; i++) {
        if(dshot_packet & 0x8000) {
            if(esc_id == 1) {
                esc1_dshotPacket[i].level0    = 1;
                esc1_dshotPacket[i].duration0 = 400;
                esc1_dshotPacket[i].level1    = 0;
                esc1_dshotPacket[i].duration1 = 134;
            } else if(esc_id == 2) {
                esc2_dshotPacket[i].level0    = 1;
                esc2_dshotPacket[i].duration0 = 400;
                esc2_dshotPacket[i].level1    = 0;
                esc2_dshotPacket[i].duration1 = 134;
            } else if(esc_id == 3) {
                esc3_dshotPacket[i].level0    = 1;
                esc3_dshotPacket[i].duration0 = 400;
                esc3_dshotPacket[i].level1    = 0;
                esc3_dshotPacket[i].duration1 = 134;
            } else if(esc_id == 4) {
                esc4_dshotPacket[i].level0    = 1;
                esc4_dshotPacket[i].duration0 = 400;
                esc4_dshotPacket[i].level1    = 0;
                esc4_dshotPacket[i].duration1 = 134;
            }
        } else {
            if(esc_id == 1) {
                esc1_dshotPacket[i].level0    = 1;
                esc1_dshotPacket[i].duration0 = 199;
                esc1_dshotPacket[i].level1    = 0;
                esc1_dshotPacket[i].duration1 = 335;
            } else if(esc_id == 2) {
                esc2_dshotPacket[i].level0    = 1;
                esc2_dshotPacket[i].duration0 = 199;
                esc2_dshotPacket[i].level1    = 0;
                esc2_dshotPacket[i].duration1 = 335;
            } else if(esc_id == 3) {
                esc3_dshotPacket[i].level0    = 1;
                esc3_dshotPacket[i].duration0 = 199;
                esc3_dshotPacket[i].level1    = 0;
                esc3_dshotPacket[i].duration1 = 335;
            } else if(esc_id == 4) {
                esc4_dshotPacket[i].level0    = 1;
                esc4_dshotPacket[i].duration0 = 199;
                esc4_dshotPacket[i].level1    = 0;
                esc4_dshotPacket[i].duration1 = 335;
            }
        }

        dshot_packet <<= 1;
    }

    if(esc_id == 1) {
        rmtWrite(esc1, esc1_dshotPacket, 16);
    } else if(esc_id == 2) {
        rmtWrite(esc2, esc2_dshotPacket, 16);
    } else if(esc_id == 3) {
        rmtWrite(esc3, esc3_dshotPacket, 16);
    } else if(esc_id == 4) {
        rmtWrite(esc4, esc4_dshotPacket, 16);
    }
}


void taskESC(void *pvParameters) {
    // Infinite loop
    while(true) {
        if(armed == true || command_trigger == true) {
            if(command_trigger) {
                for(int i=6; i > 0; i--) {
                    txDshot(ESC1, command_val, true);
                    txDshot(ESC2, command_val, true);
                    txDshot(ESC3, command_val, true);
                    txDshot(ESC4, command_val, true);
                    delay(1);
                }
                command_trigger = false;
            } else {
                // void txDshot(uint8_t esc_id, uint16_t value, bool telemetry) {
                txDshot(ESC1, esc1_val, false);
                txDshot(ESC2, esc2_val, false);
                txDshot(ESC3, esc3_val, false);
                txDshot(ESC4, esc4_val, false);
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
    delay(5000);
    
    command_trigger = true;
    command_val = 21;


    esc1_val = 58;
    esc2_val = 58;
    esc3_val = 58;
    esc4_val = 58;
    armed = true;

    Serial.println("Started!");
    delay(3000);
    for(int i = 1; i < 100; i++) {
        int val = i * 10 + 48;
        esc1_val = val;
        esc2_val = val;
        esc3_val = val;
        esc4_val = val;
        Serial.printf("ESC Speed: %i\n", i);

        delay(100);
    }    
    for(int i = 100; i > 1; i--) {
        int val = i * 10 + 48;
        esc1_val = val;
        esc2_val = val;
        esc3_val = val;
        esc4_val = val;
        Serial.printf("ESC Speed: %i\n", i);

        delay(100);
    } 

    esc1_val = 0;
    esc2_val = 0;
    esc3_val = 0;
    esc4_val = 0;
    armed = false;

    Serial.println("Bye!");
    while(true) {
        if(Serial.available() > 0) {
            serial_cmd = Serial.readStringUntil('\n');
            serial_cmd.trim();

            if(serial_cmd == "spin10") {
                esc1_val = 200;
                esc2_val = 200;
                esc3_val = 200;
                esc4_val = 200;
                armed = true;
            } else if(serial_cmd == "stop") {
                esc1_val = 48;
                esc2_val = 48;
                esc3_val = 48;
                esc4_val = 48;
                armed = false;
            } else if(serial_cmd == "reverse") {
                esc1_val = 21;
                esc2_val = 21;
                esc3_val = 21;
                esc4_val = 21;
                command_trigger = true;
                armed = false;
            } else if(serial_cmd == "forward") {
                esc1_val = 20;
                esc2_val = 20;
                esc3_val = 20;
                esc4_val = 20;
                command_trigger = true;
                armed = false;
            }
        } 
    }
}
