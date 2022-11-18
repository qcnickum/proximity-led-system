#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "nrf52840dk.h"

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// Holds duty cycle values to trigger PWM toggle
nrf_pwm_values_common_t sequence_data[1] = {0};

// Sequence structure for configuring DMA
nrf_pwm_sequence_t pwm_sequence = {
  .values.p_common = sequence_data,
  .length = 1,
  .repeats = 0,
  .end_delay = 0,
};


static void pwm_init(void) {
  // Initialize the PWM
  // SPEAKER_OUT is the output pin, mark the others as NRFX_PWM_PIN_NOT_USED
  // Set the clock to 500 kHz, count mode to Up, and load mode to Common
  // The Countertop value doesn't matter for now. We'll set it in play_tone()
  // TODO
  nrfx_pwm_config_t local_config;
  local_config.output_pins[0] = SPEAKER_OUT;
  for (int i = 1; i < 4; i++) {
    local_config.output_pins[i] = NRFX_PWM_PIN_NOT_USED;
  } 
  local_config.base_clock = NRF_PWM_CLK_500kHz;
  local_config.count_mode = NRF_PWM_MODE_UP;
  local_config.load_mode = NRF_PWM_LOAD_COMMON;
  local_config.step_mode = NRF_PWM_STEP_AUTO;

  nrfx_pwm_init(&PWM_INST, &local_config, NULL);
}

static void play_tone(uint16_t frequency) {
  // Stop the PWM (and wait until its finished)
  // Second argument blocks function until finished if true
  nrfx_pwm_stop(&PWM_INST, true);

  // Set a countertop value based on desired tone frequency
  // You can access it as NRF_PWM0->COUNTERTOP
  int countertop = 500000/frequency;
  NRF_PWM0->COUNTERTOP = countertop;
  

  // Modify the sequence data to be a 25% duty cycle
  sequence_data[0] = countertop/2;


  // Start playback of the samples and loop indefinitely
  nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
}
