// takes some brightness value up to 100.0
color_t make_color_of_brightness(color_t input_color, float brightness);

int has_known_id(uint16_t id);

int get_device_index(uint16_t id);

int is_same_color(color_t color_1, color_t color_2);
