### How To Enable DSHOT Commands
1. ESC must be disabled.
2. delay at least 9mS (From BetaFlight: DSHOT_INITIAL_DELAY_US - DSHOT_COMMAND_DELAY_US)
3. Repeat below for 10 times:
   1. delay 1ms
   2. set value with telemetry = true
   3. send to ESC
4. delay 1ms

Spin20
stop
rdemo
stop
fdemo

### PLANS

--- ESC Numbering ---

(4) (2)
  \ /
  / \
(3) (1)


--- MODULES --- 
+------------------+  +------------+  +---------------+  +----------------+  +----------+  +-------------+
| ESC DSHOT DRIVER |  | PID LOGICS |  | RECEIVER UNIT |  | TELEMETRY UNIT |  | OSD UNIT |  | SWITCH UNIT |
+------------------+  +------------+  +---------------+  +----------------+  +----------+  +-------------+

1. Due to DSHOT150 requires precise timing, it will just keep looping at CORE 0.
2. Core 1 focus on calculations, receiving commands, detecting sensors, output to ESCs Driver and also display on OSD.
3. Will have "Turtle Mode" feature. It wil requires a flag to disarmed then trigger "Turtle Mode". During "Turtle Mode",
   only two ESCs will be runing either ESC4 and ESC3 or ESC2 and ESC1 based on the Roll values: 
   > 127: ESC1 & ESC2 run
   < 127: ESC4 & ESC3 run
   = 127: STOP


### FLOW (MAIN)

[ START ] ---> [ Initialization ] ---> [ ESP32 set to 12.5nS tick ] ---> [ Start DSHOT signal generation at Core 0 ]
