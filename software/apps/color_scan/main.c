// BLE Scanning app
//
// Receives BLE advertisements

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "simple_ble.h"
#include "pwm_driver.h"
#include "app_timer.h"
#include "nrf52840dk.h"

color_t DARKNESS;

APP_TIMER_DEF(device_1_timer);
APP_TIMER_DEF(device_2_timer);
uint32_t ttl_ms = 1500;

APP_TIMER_DEF(device_1_undim_timer);
APP_TIMER_DEF(device_2_undim_timer);
app_timer_id_t device_undim_timers[2] = {device_1_undim_timer, device_2_undim_timer};

uint32_t animation_duration_ms = 3000; // how long we want it to take (in ms) to dim on or off
uint32_t animation_step_size = 10;     // how much (in percent) the brightness changes in each frame of animation.
uint32_t ms_per_frame;
// consider that a step size of 10 will result in there being 10 frames of animation. Then, duration and step size will together control framerate (initialized in main).
// number_of_frames = (100 / step_size)
// ms_per_frame = duration / number_of_frames

typedef struct animation_state
{
  uint8_t device_id;
  float brightness;
  uint8_t is_undimming;
} animation_state_t;

animation_state_t device_1_animation_state = {.device_id = 0, .brightness = 0.0};
animation_state_t device_2_animation_state = {.device_id = 1, .brightness = 0.0};

app_timer_id_t device_timers[2] = {device_1_timer, device_2_timer};
color_t global_device_state[2];

int has_known_id(uint16_t id)
{
  return id == 0xAABB || id == 0xCCDD;
}

int get_device_index(uint16_t id)
{
  if (id == 0xAABB)
  {
    return 0;
  }

  if (id == 0xCCDD)
  {
    return 1;
  }

  // device doesnt exist
  return -1;
}

int is_same_color(color_t color_1, color_t color_2)
{
  return color_1.green == color_2.green && color_1.red == color_2.red && color_1.blue == color_2.blue;
}

color_t calculate_combined_color()
{
  color_t final_color;
  final_color.val = 0x00;

  uint8_t green = global_device_state[0].green + global_device_state[1].green;
  green = green >= global_device_state[0].green ? green : 255;

  uint8_t red = global_device_state[0].red + global_device_state[1].red;
  red = red >= global_device_state[0].red ? red : 255;

  uint8_t blue = global_device_state[0].blue + global_device_state[1].blue;
  blue = blue >= global_device_state[0].blue ? blue : 255;

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

  float brightness = state->brightness;                    // read
  brightness = max(0.0, brightness - animation_step_size); // update: reduce brightness
  state->brightness = brightness;                          // write back

  global_device_state[device_id] = set_brightness(global_device_state[device_id], brightness);
  display_color(calculate_combined_color());

  if (brightness > 0)
  {
    app_timer_start(device_undim_timers[device_id], APP_TIMER_TICKS(ms_per_frame), animation_state_ptr);
  }
}

void undim_device(void *animation_state_ptr)
{
  animation_state_t *state = (animation_state_t *)animation_state_ptr;
  uint8_t device_id = state->device_id;
  float brightness = state->brightness;                      // read
  brightness = min(100.0, brightness + animation_step_size); // update: increase brightness
  state->brightness = brightness;                            // write back

  if (brightness < 100.0)
  {
    app_timer_start(device_timers[device_id], APP_TIMER_TICKS(ms_per_frame), animation_state_ptr);
  }
  else
  {
    state->is_undimming = 0;
  }
}

// BLE configuration
// This is mostly irrelevant since we are scanning only
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

// Callback handler for advertisement reception
void ble_evt_adv_report(ble_evt_t const *p_ble_evt)
{

  // extract the fields we care about
  ble_gap_evt_adv_report_t const *adv_report = &(p_ble_evt->evt.gap_evt.params.adv_report);
  uint8_t const *ble_addr = adv_report->peer_addr.addr; // array of 6 bytes of the address
  uint8_t *adv_buf = adv_report->data.p_data;           // array of up to 31 bytes of advertisement payload data
  uint16_t adv_len = adv_report->data.len;              // length of advertisement payload data
  int8_t adv_rssi = adv_report->rssi;

  uint16_t adv_id = (ble_addr[1] << 8) + ble_addr[0];
  if (!has_known_id(adv_id))
  {
    return;
  }

  if (adv_rssi < -80)
  {
    return;
  }

  app_timer_stop(device_timers[get_device_index(adv_id)]);
  color_t adv_color;
  adv_color.val = 0x00;
  adv_color.green = adv_buf[7];
  adv_color.red = adv_buf[8];
  adv_color.blue = adv_buf[9];

  color_t curr_device_color = global_device_state[get_device_index(adv_id)];
  if (!is_same_color(curr_device_color, adv_color))
  {
    global_device_state[get_device_index(adv_id)] = adv_color;
  }

  animation_state_t *animation_state = get_device_index(adv_id) == 0 ? &device_1_animation_state : &device_2_animation_state;
  if (!animation_state->is_undimming && animation_state->brightness < 100.0)
  {
    // start the undimming process if the light has not yet fully undimmed upon entry
    animation_state->is_undimming = 1;
    app_timer_start(device_undim_timers[get_device_index(adv_id)], APP_TIMER_TICKS(ms_per_frame), &device_1_animation_state);
  }

  display_color(calculate_combined_color());
  app_timer_start(device_timers[get_device_index(adv_id)], APP_TIMER_TICKS(1500), NULL);
}

int main(void)
{
  DARKNESS.val = 0x00;

  float number_of_frames = ceil(100 / animation_step_size); // we need to *always* round number of frames *up* rather than down when converting to int
  ms_per_frame = (100 / number_of_frames) / animation_duration_ms;

  device_1_animation_state.device_id = 0;
  device_1_animation_state.brightness = 0;
  device_1_animation_state.is_undimming = 0; // false

  device_2_animation_state.device_id = 0;
  device_2_animation_state.brightness = 0;
  device_2_animation_state.is_undimming = 0;

  global_device_state[0] = DARKNESS;
  global_device_state[1] = DARKNESS;

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
  app_timer_create(&device_1_timer, APP_TIMER_MODE_SINGLE_SHOT, dim_device);
  app_timer_create(&device_2_timer, APP_TIMER_MODE_SINGLE_SHOT, dim_device);
  app_timer_start(device_1_timer, APP_TIMER_TICKS(ttl_ms), &device_1_animation_state);
  app_timer_start(device_2_timer, APP_TIMER_TICKS(ttl_ms), &device_2_animation_state);

  app_timer_create(&device_1_undim_timer, APP_TIMER_MODE_SINGLE_SHOT, undim_device);
  app_timer_create(&device_2_undim_timer, APP_TIMER_MODE_SINGLE_SHOT, undim_device);

  // go into low power mode
  while (1)
  {
    power_manage();
  }
}
