#include "ssd1306.h"

#include "board.h"
#include "mcu.h"
#include "systick.h"

static uint8_t g_framebuffer[SSD1306_BUFFER_SIZE];

static void oled_gpio_clock_enable(void) {
    if (OLED_GPIO_BASE == GPIOA_BASE) {
        RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
        return;
    }
    if (OLED_GPIO_BASE == GPIOB_BASE) {
        RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
        return;
    }
    if (OLED_GPIO_BASE == GPIOC_BASE) {
        RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    }
}

static void i2c_delay_short(void) {
    for (volatile uint32_t i = 0; i < 24U; ++i) {
        __asm volatile ("nop");
    }
}

static void sda_high(void) {
    gpio_write_high(OLED_GPIO_BASE, OLED_SDA_PIN);
}

static void sda_low(void) {
    gpio_write_low(OLED_GPIO_BASE, OLED_SDA_PIN);
}

static void scl_high(void) {
    gpio_write_high(OLED_GPIO_BASE, OLED_SCL_PIN);
}

static void scl_low(void) {
    gpio_write_low(OLED_GPIO_BASE, OLED_SCL_PIN);
}

static uint8_t sda_read(void) {
    return gpio_read(OLED_GPIO_BASE, OLED_SDA_PIN);
}

static void i2c_init_pins(void) {
    oled_gpio_clock_enable();

    gpio_set_mode(OLED_GPIO_BASE, OLED_SCL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(OLED_GPIO_BASE, OLED_SCL_PIN, GPIO_OTYPE_OD);
    gpio_set_speed(OLED_GPIO_BASE, OLED_SCL_PIN, GPIO_SPEED_HIGH);
    gpio_set_pull(OLED_GPIO_BASE, OLED_SCL_PIN, GPIO_PULL_UP);

    gpio_set_mode(OLED_GPIO_BASE, OLED_SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(OLED_GPIO_BASE, OLED_SDA_PIN, GPIO_OTYPE_OD);
    gpio_set_speed(OLED_GPIO_BASE, OLED_SDA_PIN, GPIO_SPEED_HIGH);
    gpio_set_pull(OLED_GPIO_BASE, OLED_SDA_PIN, GPIO_PULL_UP);

    sda_high();
    scl_high();
}

static void i2c_start(void) {
    sda_high();
    scl_high();
    i2c_delay_short();
    sda_low();
    i2c_delay_short();
    scl_low();
    i2c_delay_short();
}

static void i2c_stop(void) {
    sda_low();
    i2c_delay_short();
    scl_high();
    i2c_delay_short();
    sda_high();
    i2c_delay_short();
}

static uint8_t i2c_write_byte(uint8_t value) {
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        if ((value & 0x80U) != 0U) {
            sda_high();
        } else {
            sda_low();
        }
        i2c_delay_short();
        scl_high();
        i2c_delay_short();
        scl_low();
        i2c_delay_short();
        value <<= 1U;
    }

    sda_high();
    i2c_delay_short();
    scl_high();
    i2c_delay_short();
    const uint8_t ack = (uint8_t)(sda_read() == 0U ? 1U : 0U);
    scl_low();
    i2c_delay_short();
    return ack;
}

static void ssd1306_write_command(uint8_t command) {
    i2c_start();
    (void)i2c_write_byte((uint8_t)(OLED_I2C_ADDRESS << 1U));
    (void)i2c_write_byte(0x00U);
    (void)i2c_write_byte(command);
    i2c_stop();
}

static void ssd1306_write_data_block(const uint8_t *data, uint32_t length) {
    i2c_start();
    (void)i2c_write_byte((uint8_t)(OLED_I2C_ADDRESS << 1U));
    (void)i2c_write_byte(0x40U);
    while (length > 0U) {
        (void)i2c_write_byte(*data++);
        --length;
    }
    i2c_stop();
}

void ssd1306_init(void) {
    i2c_init_pins();
    systick_delay_ms(100U);

    ssd1306_write_command(0xAEU);
    ssd1306_write_command(0x20U);
    ssd1306_write_command(0x00U);
    ssd1306_write_command(0xB0U);
    ssd1306_write_command(0xC8U);
    ssd1306_write_command(0x00U);
    ssd1306_write_command(0x10U);
    ssd1306_write_command(0x40U);
    ssd1306_write_command(0x81U);
    ssd1306_write_command(0x7FU);
    ssd1306_write_command(0xA1U);
    ssd1306_write_command(0xA6U);
    ssd1306_write_command(0xA8U);
    ssd1306_write_command(0x3FU);
    ssd1306_write_command(0xA4U);
    ssd1306_write_command(0xD3U);
    ssd1306_write_command(0x00U);
    ssd1306_write_command(0xD5U);
    ssd1306_write_command(0x80U);
    ssd1306_write_command(0xD9U);
    ssd1306_write_command(0xF1U);
    ssd1306_write_command(0xDAU);
    ssd1306_write_command(0x12U);
    ssd1306_write_command(0xDBU);
    ssd1306_write_command(0x40U);
    ssd1306_write_command(0x8DU);
    ssd1306_write_command(0x14U);
    ssd1306_write_command(0xAFU);

    ssd1306_clear();
    ssd1306_update();
}

void ssd1306_clear(void) {
    for (uint32_t i = 0U; i < SSD1306_BUFFER_SIZE; ++i) {
        g_framebuffer[i] = 0U;
    }
}

void ssd1306_load_bitmap(const uint8_t *bitmap) {
    if (bitmap == 0) {
        return;
    }

    for (uint32_t i = 0U; i < SSD1306_BUFFER_SIZE; ++i) {
        g_framebuffer[i] = bitmap[i];
    }
}

void ssd1306_update(void) {
    for (uint8_t page = 0U; page < 8U; ++page) {
        ssd1306_write_command((uint8_t)(0xB0U + page));
        ssd1306_write_command(0x00U);
        ssd1306_write_command(0x10U);
        ssd1306_write_data_block(&g_framebuffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
    }
}

void ssd1306_draw_pixel(int16_t x, int16_t y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }

    const uint32_t index = (uint32_t)x + ((uint32_t)y / 8U) * SSD1306_WIDTH;
    const uint8_t mask = (uint8_t)(1U << ((uint32_t)y % 8U));

    if (on) {
        g_framebuffer[index] |= mask;
    } else {
        g_framebuffer[index] &= (uint8_t)~mask;
    }
}

void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on) {
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y0 < y1) ? (y0 - y1) : (y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        ssd1306_draw_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int16_t err2 = (int16_t)(2 * err);
        if (err2 >= dy) {
            err = (int16_t)(err + dy);
            x0 = (int16_t)(x0 + sx);
        }
        if (err2 <= dx) {
            err = (int16_t)(err + dx);
            y0 = (int16_t)(y0 + sy);
        }
    }
}

void ssd1306_draw_circle(int16_t x0, int16_t y0, int16_t radius, bool on) {
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        ssd1306_draw_pixel((int16_t)(x0 + x), (int16_t)(y0 + y), on);
        ssd1306_draw_pixel((int16_t)(x0 + y), (int16_t)(y0 + x), on);
        ssd1306_draw_pixel((int16_t)(x0 - y), (int16_t)(y0 + x), on);
        ssd1306_draw_pixel((int16_t)(x0 - x), (int16_t)(y0 + y), on);
        ssd1306_draw_pixel((int16_t)(x0 - x), (int16_t)(y0 - y), on);
        ssd1306_draw_pixel((int16_t)(x0 - y), (int16_t)(y0 - x), on);
        ssd1306_draw_pixel((int16_t)(x0 + y), (int16_t)(y0 - x), on);
        ssd1306_draw_pixel((int16_t)(x0 + x), (int16_t)(y0 - y), on);

        if (err <= 0) {
            ++y;
            err = (int16_t)(err + (2 * y) + 1);
        }
        if (err > 0) {
            --x;
            err = (int16_t)(err - (2 * x) - 1);
        }
    }
}

void ssd1306_fill_circle(int16_t x0, int16_t y0, int16_t radius, bool on) {
    for (int16_t y = (int16_t)(-radius); y <= radius; ++y) {
        for (int16_t x = (int16_t)(-radius); x <= radius; ++x) {
            if ((x * x) + (y * y) <= (radius * radius)) {
                ssd1306_draw_pixel((int16_t)(x0 + x), (int16_t)(y0 + y), on);
            }
        }
    }
}
