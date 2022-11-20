#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "nrf52840dk.h"

#define LED_STRIP_PIN NRF_GPIO_PIN_MAP(1, 8)
// Need enough for 30 LED * 3 colors per LED * 8 bits per byte + 1 byte of 0's to mark end
#define LED_DUTY_CYCLE_ARRAY_LENGTH 728

typedef struct color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} color_t;

enum duty_cycle {HIGH = 7, LOW = 3};

void pwm_init(void);

void display_color(void);
