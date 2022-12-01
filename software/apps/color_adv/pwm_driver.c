#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "pwm_driver.h"
#include "nrf52840dk.h"

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// Holds duty cycle values to trigger PWM toggle
nrf_pwm_values_common_t sequence_data[LED_DUTY_CYCLE_ARRAY_LENGTH];

// Sequence structure for configuring DMA
nrf_pwm_sequence_t pwm_sequence = {
  .values.p_common = sequence_data,
  .length = LED_DUTY_CYCLE_ARRAY_LENGTH,
  .repeats = 0,
  .end_delay = 0,
};


void pwm_init(void) {
  // Initialize the PWM
  nrfx_pwm_config_t local_config;
  local_config.output_pins[0] = LED_STRIP_PIN;
  for (int i = 1; i < 4; i++) {
    local_config.output_pins[i] = NRFX_PWM_PIN_NOT_USED;
  } 
  local_config.base_clock = NRF_PWM_CLK_8MHz;
  local_config.count_mode = NRF_PWM_MODE_UP;
  local_config.load_mode = NRF_PWM_LOAD_COMMON;
  local_config.step_mode = NRF_PWM_STEP_AUTO;
  local_config.top_value = 8000000/400000;

  nrfx_pwm_init(&PWM_INST, &local_config, NULL);
}

void display_color(color_t color) {
  // Stop the PWM (and wait until its finished)
  // Second argument blocks function until finished if true
  nrfx_pwm_stop(&PWM_INST, true);

  uint16_t color_array[24];
  for (uint32_t i = 0; i < 24; i++) {
    color_array[i] = ((1 << i) & color.val) ? HIGH : LOW;
  }

  for (uint32_t i = 0; i < 30; i++) {
    for (uint32_t j = 0; j < 24; j++) {
      sequence_data[(i * 24) + j] = color_array[j];
    }
  }
  
  for (uint32_t i = LED_DUTY_CYCLE_ARRAY_LENGTH - 24; i < LED_DUTY_CYCLE_ARRAY_LENGTH; i++) {
    sequence_data[i] = 0;
  }

  nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP);
}

