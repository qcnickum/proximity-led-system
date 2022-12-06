// BLE Scanning app
//
// Receives BLE advertisements

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "simple_ble.h"
#include "pwm_driver.h"
#include "helpers.h"
#include "app_timer.h"
#include "nrf52840dk.h"

// BLE configuration
static simple_ble_config_t ble_config = {
    // BLE address is c0:98:e5:4e:00:02
    .platform_id = 0x4E,                                    // used as 4th octet in device BLE address
    .device_id = 0x0002,                                    // used as the 5th and 6th octet in the device BLE address, you will need to change this for each device you have
    .adv_name = "CS397/497",                                // irrelevant in this example
    .adv_interval = MSEC_TO_UNITS(1000, UNIT_0_625_MS),     // send a packet once per second (minimum is 20 ms)
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),  // irrelevant if advertising only
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS), // irrelevant if advertising only
};
simple_ble_app_t *simple_ble_app;

color_t DARKNESS;

APP_TIMER_DEF(device_1_ttl_timer);
APP_TIMER_DEF(device_2_ttl_timer);
app_timer_id_t device_ttl_timers[2] = {device_1_ttl_timer, device_2_ttl_timer};
const uint32_t TTL_MS = 1500;

APP_TIMER_DEF(device_1_undim_timer);
APP_TIMER_DEF(device_2_undim_timer);
app_timer_id_t device_undim_timers[2] = {device_1_undim_timer, device_2_undim_timer};

const uint32_t ANIMATION_STEP_SIZE = 10; // how much (in percent) the brightness changes in each frame of animation.
const uint32_t ANIMATION_MS = 100;

typedef struct animation_state
{
  uint8_t device_id;
  float brightness;
  uint8_t is_undimming;
} animation_state_t;

animation_state_t device_1_animation_state = {.device_id = 0, .brightness = 0.0, .is_undimming = 0};
animation_state_t device_2_animation_state = {.device_id = 1, .brightness = 0.0, .is_undimming = 0};

color_t animation_colors[2];
color_t actual_device_color[2];

color_t calculate_combined_color()
{
  color_t final_color;
  final_color.val = 0x00;

  uint8_t green = animation_colors[0].green + animation_colors[1].green;
  green = green >= animation_colors[0].green ? green : 255;

  uint8_t red = animation_colors[0].red + animation_colors[1].red;
  red = red >= animation_colors[0].red ? red : 255;

  uint8_t blue = animation_colors[0].blue + animation_colors[1].blue;
  blue = blue >= animation_colors[0].blue ? blue : 255;

  final_color.green = green;
  final_color.red = red;
  final_color.blue = blue;

  return final_color;
}

void dim_device(void *animation_state_ptr)
{
  animation_state_t *state = (animation_state_t *)animation_state_ptr;
  uint8_t device_id = state->device_id;

  app_timer_stop(device_undim_timers[device_id]);
  state->is_undimming = 0;

  float brightness = state->brightness;                     // read
  brightness = fmax(0.0, brightness - (ANIMATION_STEP_SIZE + 5)); // update: reduce brightness
  state->brightness = brightness;                           // write back

  animation_colors[device_id] = make_color_of_brightness(actual_device_color[device_id], brightness);
  display_color(calculate_combined_color());

  if (brightness == 0.0)
  {
    app_timer_stop(device_ttl_timers[device_id]);
  }
}

void undim_device(void *animation_state_ptr)
{
  animation_state_t *state = (animation_state_t *)animation_state_ptr;
  uint8_t device_id = state->device_id;

  float brightness = state->brightness;                       // read
  brightness = fmin(100.0, brightness + ANIMATION_STEP_SIZE); // update: increase brightness
  state->brightness = brightness;                             // write back

  animation_colors[device_id] = make_color_of_brightness(actual_device_color[device_id], brightness);

  if (brightness == 100.0)
  {
    app_timer_stop(device_undim_timers[device_id]);
    state->is_undimming = 0;
  }
}

// Callback handler for advertisement reception
void ble_evt_adv_report(ble_evt_t const *p_ble_evt)
{
  // extract the fields we care about
  ble_gap_evt_adv_report_t const *adv_report = &(p_ble_evt->evt.gap_evt.params.adv_report);
  uint8_t const *ble_addr = adv_report->peer_addr.addr; // array of 6 bytes of the address
  uint8_t *adv_buf = adv_report->data.p_data;           // array of up to 31 bytes of advertisement payload data
  int8_t adv_rssi = adv_report->rssi;

  uint16_t adv_id = (ble_addr[1] << 8) + ble_addr[0];
  if (!has_known_id(adv_id))
  {
    return;
  }

  if (adv_rssi < -48)
  {
    return;
  }

  app_timer_stop(device_ttl_timers[get_device_index(adv_id)]);

  color_t adv_color;
  adv_color.val = 0x00;
  adv_color.green = adv_buf[7];
  adv_color.red = adv_buf[8];
  adv_color.blue = adv_buf[9];

  uint8_t device_id = get_device_index(adv_id);
  animation_state_t *animation_state = device_id == 0 ? &device_1_animation_state : &device_2_animation_state;

  if (!is_same_color(actual_device_color[device_id], adv_color))
  {
    actual_device_color[device_id] = adv_color;
    animation_colors[device_id] = make_color_of_brightness(actual_device_color[device_id], animation_state->brightness);
  }

  if (!animation_state->is_undimming && animation_state->brightness < 100.0)
  {
    // start the undimming process if the light has not yet fully undimmed upon entry
    animation_state->is_undimming = 1;
    undim_device((void *)animation_state);
    app_timer_start(device_undim_timers[device_id], APP_TIMER_TICKS(ANIMATION_MS), animation_state);
  }

  display_color(calculate_combined_color());
  app_timer_start(device_ttl_timers[device_id], APP_TIMER_TICKS(TTL_MS), animation_state);
}

int main(void)
{
  DARKNESS.val = 0x00;

  animation_colors[0] = DARKNESS;
  animation_colors[1] = DARKNESS;

  // Setup BLE
  // Note: simple BLE is our own library. You can find it in `nrf5x-base/lib/simple_ble/`
  simple_ble_app = simple_ble_init(&ble_config);
  advertising_stop();

  // Start scanning
  scanning_start();

  pwm_init();
  display_color(DARKNESS);

  // init/create timers, and start them
  app_timer_init();
  app_timer_create(&device_1_ttl_timer, APP_TIMER_MODE_REPEATED, dim_device);
  app_timer_create(&device_2_ttl_timer, APP_TIMER_MODE_REPEATED, dim_device);

  app_timer_create(&device_1_undim_timer, APP_TIMER_MODE_REPEATED, undim_device);
  app_timer_create(&device_2_undim_timer, APP_TIMER_MODE_REPEATED, undim_device);

  // go into low power mode
  while (1)
  {
    power_manage();
  }
}
