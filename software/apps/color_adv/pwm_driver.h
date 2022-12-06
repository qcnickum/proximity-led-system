#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "nrf52840dk.h"

#define LED_STRIP_PIN NRF_GPIO_PIN_MAP(1, 8)
// Need enough for 30 LED * 3 colors per LED * 8 bits per byte + 1 byte of 0's to mark end
#define LED_DUTY_CYCLE_ARRAY_LENGTH 408

// LED expects colors in GRB format (G7, ..., G0, R7, ..., B7,..., B0)
// G7 most significat bit
typedef union color
{
  uint32_t val;
  struct
  {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
    uint8_t padding;
  };
} color_t;

enum duty_cycle
{
  HIGH = (1 << 15) | 7,
  LOW = (1 << 15) | 3
};

void pwm_init(void);

void display_color(color_t color);

void display_color_options(color_t *color_options);
