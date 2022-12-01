// BLE Scanning app
//
// Receives BLE advertisements

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "simple_ble.h"
#include "pwm_driver.h"
#include "app_timer.h"
#include "nrf52840dk.h"

color_t DARKNESS;

APP_TIMER_DEF(device_1_timer);
APP_TIMER_DEF(device_2_timer);

app_timer_id_t device_timers[2] = { device_1_timer, device_2_timer };
color_t global_device_state[2];

int has_known_id(uint16_t id) {
  return id == 0xAABB || id == 0xCCDD;
}

int get_device_index(uint16_t id) {
  if (id == 0xAABB) {
    return 0;
  }
  
  if (id == 0xCCDD) {
    return 1;
  }
  
  // device doesnt exist
  return -1;
}

int is_same_color(color_t color_1, color_t color_2) {
  return color_1.green == color_2.green && color_1.red == color_2.red && color_1.blue == color_2.blue;
}

color_t calculate_combined_color() {
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

void reset_device_1() {
  global_device_state[0] = DARKNESS;
  display_color(calculate_combined_color());  
}

void reset_device_2() {
  global_device_state[1] = DARKNESS;
  display_color(calculate_combined_color());
}

// BLE configuration
// This is mostly irrelevant since we are scanning only
static simple_ble_config_t ble_config = {
        // BLE address is c0:98:e5:4e:00:02
        .platform_id       = 0x4E,    // used as 4th octet in device BLE address
        .device_id         = 0x0002,  // used as the 5th and 6th octet in the device BLE address, you will need to change this for each device you have
        .adv_name          = "CS397/497", // irrelevant in this example
        .adv_interval      = MSEC_TO_UNITS(1000, UNIT_0_625_MS), // send a packet once per second (minimum is 20 ms)
        .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS), // irrelevant if advertising only
        .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS), // irrelevant if advertising only
};
simple_ble_app_t* simple_ble_app;


// Callback handler for advertisement reception
void ble_evt_adv_report(ble_evt_t const* p_ble_evt) {

  // extract the fields we care about
  ble_gap_evt_adv_report_t const* adv_report = &(p_ble_evt->evt.gap_evt.params.adv_report);
  uint8_t const* ble_addr = adv_report->peer_addr.addr; // array of 6 bytes of the address
  uint8_t* adv_buf = adv_report->data.p_data; // array of up to 31 bytes of advertisement payload data
  uint16_t adv_len = adv_report->data.len; // length of advertisement payload data
  int8_t adv_rssi = adv_report->rssi;

  uint16_t adv_id = (ble_addr[1] << 8) + ble_addr[0];
  if (!has_known_id(adv_id)) {
    return; 
  }

  if (adv_rssi < -45) {
    return;
  }

  //printf("Found our device with address 0x%X%X\n", ble_addr[1], ble_addr[0]);
  //printf("It has data: ");
  //for (uint16_t i=0; i < adv_len; i++) {
  //  printf("0x%x  ", adv_buf[i]);
  //}
  //printf("RSSI IS: %d\n\n", adv_rssi);
  app_timer_stop(device_timers[get_device_index(adv_id)]);
  color_t adv_color;
  adv_color.val = 0x00;
  adv_color.green = adv_buf[7];
  adv_color.red = adv_buf[8];
  adv_color.blue = adv_buf[9];
  
  color_t curr_device_color = global_device_state[get_device_index(adv_id)];
  if (is_same_color(curr_device_color, adv_color)) {
    app_timer_start(device_timers[get_device_index(adv_id)], APP_TIMER_TICKS(1500), NULL);
    return;
  }
  
  global_device_state[get_device_index(adv_id)] = adv_color;
  display_color(calculate_combined_color());
  app_timer_start(device_timers[get_device_index(adv_id)], APP_TIMER_TICKS(1500), NULL);
}


int main(void) {
  DARKNESS.val = 0x00;

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
  app_timer_create(&device_1_timer, APP_TIMER_MODE_REPEATED, reset_device_1);
  app_timer_create(&device_2_timer, APP_TIMER_MODE_REPEATED, reset_device_2);
  app_timer_start(device_1_timer, APP_TIMER_TICKS(1500), NULL);
  app_timer_start(device_2_timer, APP_TIMER_TICKS(1500), NULL);

  // go into low power mode
  while(1) {
    power_manage();
  }
}



