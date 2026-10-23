#ifndef ADC420_APP_H
#define ADC420_APP_H

#include <stdint.h>

#define ADC420_SENSOR_COUNT 10U
#define ADC420_CAN_PERIOD_MS 100U
#define ADC420_CAN_FRAME_0 0x310U
#define ADC420_CAN_FRAME_1 0x311U
#define ADC420_CAN_FRAME_2 0x312U
#define ADC420_CAN_STATUS  0x313U

/* Board channels are ADC_2..ADC_11, *not* ADS1118 AIN indices. */
typedef struct {
    uint8_t board_channel;
    uint8_t ads_chip;
    uint8_t ads_ain;
} ADC420_ChannelMap;

extern const ADC420_ChannelMap adc420_channel_map[ADC420_SENSOR_COUNT];
extern volatile int16_t adc420_raw[ADC420_SENSOR_COUNT];
extern volatile uint16_t adc420_current_ua[ADC420_SENSOR_COUNT];
extern volatile uint16_t adc420_valid_mask;
extern volatile uint16_t adc420_fault_mask;
extern volatile uint8_t adc420_sequence;
extern volatile uint8_t adc420_can_error_count;

uint16_t ADC420_RawToMicroamps(int16_t raw);
void ADC420_App_RunCycle(uint8_t can_ready);

#endif
