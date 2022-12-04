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

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
    // c0:98:e5:4e:xx:xx
    .platform_id = 0x4E,     // used as 4th octect in device BLE address
    .device_id = 0xCCDD,     // must be unique on each device you program!
    .adv_name = "CS397/497", // used in advertisements if there is room
    .adv_interval = MSEC_TO_UNITS(1000, UNIT_0_625_MS),
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),
};

// Main application state
simple_ble_app_t *simple_ble_app;

APP_TIMER_DEF(blinking_timer);

static color_t DARKNESS, RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK;
color_t color_options[8];
color_t displayed_colors[8];
int8_t color_index = 0;
uint8_t is_in_select_mode = 0; // 0 means not in select mode

int8_t increment_color_index(int8_t index)
{
  return index + 1 > 7 ? 0 : index + 1;
}

int8_t decrement_color_index(int8_t index)
{
  return index - 1 < 0 ? 7 : index - 1;
}

void update_color()
{
  advertising_stop();
  uint8_t new_color[3] = {color_options[color_index].green,
                          color_options[color_index].red,
                          color_options[color_index].blue};
  simple_ble_adv_manuf_data(new_color, 3);
}

void blink_animation()
{
  if (displayed_colors[color_index].val == DARKNESS.val)
  {
    displayed_colors[color_index] = color_options[color_index];
  }
  else
  {
    displayed_colors[color_index] = DARKNESS;
  }

  display_color_options(displayed_colors);
}

void reset_displayed_colors()
{
  for (uint8_t i = 0; i < 8; i++)
  {
    displayed_colors[i] = color_options[i];
  }
}

void set_color_options()
{
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

  color_options[0] = RED;
  color_options[1] = ORANGE;
  color_options[2] = YELLOW;
  color_options[3] = GREEN;
  color_options[4] = CYAN;
  color_options[5] = BLUE;
  color_options[6] = PURPLE;
  color_options[7] = PINK;
}

int main(void)
{
  set_color_options();
  reset_displayed_colors();

  nrf_gpio_cfg_input(BUTTON1, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(BUTTON2, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(BUTTON3, NRF_GPIO_PIN_PULLUP);

  printf("Board started. Initializing BLE: \n\n");
  simple_ble_app = simple_ble_init(&ble_config);
  pwm_init();

  // display user's current color
  display_color(color_options[color_index]);

  app_timer_init();
  app_timer_create(&blinking_timer, APP_TIMER_MODE_REPEATED, blink_animation);

  uint8_t first_color[3] = {color_options[color_index].green,
                            color_options[color_index].red,
                            color_options[color_index].blue};
  simple_ble_adv_manuf_data(first_color, 3);
  printf("Started BLE advertisements\n\n");

  while (1)
  {
    if (!nrf_gpio_pin_read(BUTTON1) && is_in_select_mode)
    {
      color_index = decrement_color_index(color_index);
      update_color();
      reset_displayed_colors();
      nrf_delay_ms(500);
    }
    if (!nrf_gpio_pin_read(BUTTON2) && is_in_select_mode)
    {
      color_index = increment_color_index(color_index);
      update_color();
      reset_displayed_colors();
      nrf_delay_ms(500);
    }
    if (!nrf_gpio_pin_read(BUTTON3))
    {
      if (is_in_select_mode)
      {
        // set selection and leave select mode
        app_timer_stop(blinking_timer);
        is_in_select_mode = 0;

        display_color(DARKNESS);
        nrf_delay_ms(750);
        display_color(color_options[color_index]);
      }
      else
      {
        // enter select mode, display color options
        is_in_select_mode = 1;

        display_color(DARKNESS);
        nrf_delay_ms(750);
        display_color_options(displayed_colors);

        app_timer_start(blinking_timer, APP_TIMER_TICKS(750), NULL);
      }
      nrf_delay_ms(500);
    }
  }
}
