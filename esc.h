/*
 * Filename: esc.h
 * Description:
 * ESC Driver Header
 *
 */

#ifndef ESC_H_
#define ESC_H_

void escInit(void);
void taskESC(void *pvParameters);
void txDshot(uint8_t esc_id, uint16_t value, bool telemetry);

extern rmt_data_t  esc_dshotPacket[16];
extern rmt_obj_t*  esc[];
extern uint8_t     esc_pin[];
extern uint16_t    esc_val[];

// Testing
extern bool reverse_esc;
extern bool forward_esc;

extern TaskHandle_t TaskESC;

extern bool armed;
extern bool command_trigger;
extern uint16_t command_val;

#endif
