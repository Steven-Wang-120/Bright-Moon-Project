#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8U)

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_load_bitmap(const uint8_t *bitmap);
void ssd1306_update(void);
void ssd1306_draw_pixel(int16_t x, int16_t y, bool on);
void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on);
void ssd1306_draw_circle(int16_t x0, int16_t y0, int16_t radius, bool on);
void ssd1306_fill_circle(int16_t x0, int16_t y0, int16_t radius, bool on);

#endif
