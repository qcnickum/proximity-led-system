// BLE Advertisement Raw app
//
// Sends a BLE advertisement with raw bytes

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_gpio.h"
#include "pwm_driver.h"
#include "simple_ble.h"
#include "nrf_delay.h"

#include "nrf52840dk.h"

static color_t RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK;
color_t colors[8];
int8_t color_index = 0;

void increment_color_index() {
  color_index = color_index + 1 > 7 ? 0 : color_index + 1;
}

void decrement_color_index() {
  color_index = color_index - 1 < 0 ? 7 : color_index - 1;
}

void update_color() {
  printf("COLOR INDEX: %d\n\n", color_index);
  advertising_stop();
  uint8_t new_color[3] = { colors[color_index].green,
                           colors[color_index].red,
                           colors[color_index].blue };
  simple_ble_adv_manuf_data(new_color, 3);
}

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
        // c0:98:e5:4e:xx:xx
        .platform_id       = 0x4E,   // used as 4th octect in device BLE address
        .device_id         = 0xCCDD, // must be unique on each device you program!
        .adv_name          = "CS397/497", // used in advertisements if there is room
        .adv_interval      = MSEC_TO_UNITS(1000, UNIT_0_625_MS),
        .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
        .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),
};

/*******************************************************************************
 *   State for this application
 ******************************************************************************/
// Main application state
simple_ble_app_t* simple_ble_app;

int main(void) {
  RED.val = 0;
  ORANGE.val = 0;
  YELLOW.val = 0;
  GREEN.val = 0;
  CYAN.val = 0;
  BLUE.val = 0;
  PURPLE.val = 0;
  PINK.val = 0;

  RED.red = 0xFF;

  ORANGE.red = 0xFF;
  ORANGE.green = 0xFF;

  YELLOW.red = 0xFF;
  YELLOW.green = 0xFF;

  GREEN.green = 0xFF;
 
  CYAN.green = 0xFF;
  CYAN.blue = 0xFF;

  BLUE.blue = 0xFF;

  PURPLE.blue = 0xFF;
  PURPLE.red = 0xFF;

  PINK.red = 0xFF;
  PINK.blue = 0xFF;
  
  colors[0] = RED;
  colors[1] = ORANGE;
  colors[2] = YELLOW;
  colors[3] = GREEN;
  colors[4] = CYAN;
  colors[5] = BLUE;
  colors[6] = PURPLE;
  colors[7] = PINK;

  nrf_gpio_cfg_input(BUTTON1, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(BUTTON2, NRF_GPIO_PIN_PULLUP);

  printf("Board started. Initializing BLE: \n\n");

  // Setup BLE
  // Note: simple BLE is our own library. You can find it in `nrf5x-base/lib/simple_ble/`
  simple_ble_app = simple_ble_init(&ble_config);

  // Start Advertising 
  uint8_t first_color[3] = { colors[color_index].green,
                             colors[color_index].red,
                             colors[color_index].blue };

  simple_ble_adv_manuf_data(first_color, 3);
  printf("Started BLE advertisements\n\n");

  while(1) {
    if (!nrf_gpio_pin_read(BUTTON1)) {
      decrement_color_index();
      update_color();
      nrf_delay_ms(500);
    }
    if (!nrf_gpio_pin_read(BUTTON2)) {
      increment_color_index();
      update_color();
      nrf_delay_ms(500);
    }
    //power_manage();
  }
}

