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

#include "EchoFC32.h"
#include "config.h"
#include "esc.h"


/**** CLI Related ****/
String serial_cmd;


void setup() {
    Serial.begin(115200);
    delay(1000);

    escInit(); 

    xTaskCreatePinnedToCore(taskESC, "TaskESC", 10000, NULL, 1, &TaskESC, 0);

    delay(1000);
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
