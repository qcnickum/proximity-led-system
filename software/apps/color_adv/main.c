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
#include "app_timer.h"

#include "nrf52840dk.h"

APP_TIMER_DEF(blinking_timer);

static color_t DARKNESS, RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK;
color_t colors[8];
color_t displayed_colors[8];
int8_t color_index = 0;
uint8_t is_in_select_mode = 0; // 0 means not in select mode

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

void blink_animation() {
  if (displayed_colors[color_index].val == DARKNESS.val) {
    displayed_colors[color_index] = colors[color_index];    		  
  } else {
    displayed_colors[color_index] = DARKNESS;
  }
  
  display_color_options(displayed_colors);
}

void reset_displayed_colors() {
  for (uint8_t i=0; i < 8; i++) {
    displayed_colors[i] = colors[i];
  }
}

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
        // c0:98:e5:4e:xx:xx
        .platform_id       = 0x4E,   // used as 4th octect in device BLE address
        .device_id         = 0xAABB, // must be unique on each device you program!
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
  DARKNESS.val = 0;
 	
  RED.val = 0;
  ORANGE.val = 0;
  YELLOW.val = 0;
  GREEN.val = 0;
  CYAN.val = 0;
  BLUE.val = 0;
  PURPLE.val = 0;
  PINK.val = 0;

  RED.red = 0x8F;
  
  ORANGE.red = 0x8F;
  ORANGE.green = 0x40;

  YELLOW.red = 0x8F;
  YELLOW.green = 0x8F;

  GREEN.green = 0x8F;
 
  CYAN.green = 0x8F;
  CYAN.blue = 0x8F;

  BLUE.blue = 0x8F;

  PURPLE.blue = 0x8F;
  PURPLE.red = 0x60;

  PINK.red = 0x8F;
  PINK.blue = 0x8F;
  
  colors[0] = RED;
  colors[1] = ORANGE;
  colors[2] = YELLOW;
  colors[3] = GREEN;
  colors[4] = CYAN;
  colors[5] = BLUE;
  colors[6] = PURPLE;
  colors[7] = PINK;

  reset_displayed_colors();

  nrf_gpio_cfg_input(BUTTON1, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(BUTTON2, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(BUTTON3, NRF_GPIO_PIN_PULLUP);


  printf("Board started. Initializing BLE: \n\n");

  // Setup BLE
  // Note: simple BLE is our own library. You can find it in `nrf5x-base/lib/simple_ble/`
  simple_ble_app = simple_ble_init(&ble_config);

  // Start Advertising 
  uint8_t first_color[3] = { colors[color_index].green,
                             colors[color_index].red,
                             colors[color_index].blue };
  pwm_init();
  //display menu of color options
  display_color(colors[color_index]);
  //display_color(colors[color_index]);
  
  //timer stuff for blink animation
  app_timer_init();
  app_timer_create(&blinking_timer, APP_TIMER_MODE_REPEATED, blink_animation);
  

  simple_ble_adv_manuf_data(first_color, 3);
  printf("Started BLE advertisements\n\n");

  while(1) {
    if (!nrf_gpio_pin_read(BUTTON1) && is_in_select_mode) {
      decrement_color_index();
      update_color();
      reset_displayed_colors();
      nrf_delay_ms(500);
    }
    if (!nrf_gpio_pin_read(BUTTON2) && is_in_select_mode) {
      increment_color_index();
      update_color();
      reset_displayed_colors();
      nrf_delay_ms(500);
    }
    if (!nrf_gpio_pin_read(BUTTON3)) {
      if (is_in_select_mode) {
	//setting selection and leaving select mode
        is_in_select_mode = 0;
	display_color(colors[color_index]);
	app_timer_stop(blinking_timer);
      } else {
	//enter select mode, display color options
        is_in_select_mode = 1;
	display_color(DARKNESS);
	nrf_delay_ms(1000);
	display_color_options(displayed_colors);
	app_timer_start(blinking_timer, APP_TIMER_TICKS(750), NULL);
      }
      nrf_delay_ms(500);
    }

    //power_manage();
  }
}

