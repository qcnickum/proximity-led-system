// BLE Scanning app
//
// Receives BLE advertisements

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "simple_ble.h"
#include "pwm_driver.h"

#include "nrf52840dk.h"

struct device_state {
  uint16_t id;
  color_t color;   
};

device_state global_device_state[2];

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
  color_t adv_color;
  adv_color.val = 0x00;
  adv_color.green = adv_buf[7];
  adv_color.red = adv_buf[8];
  adv_color.blue = adv_buf[9];
  

  display_color(adv_color);
}


int main(void) {
  device_state device_1;
  device_1.id = 0xAABB;
  device_1.color = (uint32_t) 0x00;

  device_state device_2;
  device_2.id = 0xCCDD;
  device_2.color = (uint32_t) 0x00;

  global_device_state[0] = device_1;
  gloval_device_state[1] = device_2;

  // Setup BLE
  // Note: simple BLE is our own library. You can find it in `nrf5x-base/lib/simple_ble/`
  simple_ble_app = simple_ble_init(&ble_config);
  advertising_stop();

  // Start scanning
  scanning_start();

  pwm_init();

  // go into low power mode
  while(1) {
    power_manage();
  }
}



