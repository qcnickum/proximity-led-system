#include "pwm_driver.h"

// takes some brightness value up to 100.0
color_t make_color_of_brightness(color_t input_color, float brightness)
{
  float brightness_percent = brightness / 100.0;
  color_t result_color;
  result_color.val = 0;

  result_color.green = input_color.green * brightness_percent;
  result_color.red = input_color.red * brightness_percent;
  result_color.blue = input_color.blue * brightness_percent;

  return result_color;
}

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