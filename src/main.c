#include <stdio.h>
#include <string.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"

// ===== Waveshare Pico-OLED-1.3 (SH1107) pins =====
#define OLED_SPI spi1
#define PIN_SCK 10
#define PIN_MOSI 11
#define PIN_CS 9
#define PIN_DC 8
#define PIN_RST 12

#define OLED_WIDTH 64
#define OLED_HEIGHT 128
#define OLED_PAGES (OLED_HEIGHT / 8)
#define OLED_X_OFFSET 32

static uint8_t framebuffer[OLED_WIDTH * OLED_PAGES];

static inline void cs_select(void) { gpio_put(PIN_CS, 0); }
static inline void cs_deselect(void) { gpio_put(PIN_CS, 1); }

static void oled_write_cmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0);
    cs_select();
    spi_write_blocking(OLED_SPI, &cmd, 1);
    cs_deselect();
}

static void oled_write_data(const uint8_t* data, size_t len) {
    gpio_put(PIN_DC, 1);
    cs_select();
    spi_write_blocking(OLED_SPI, data, len);
    cs_deselect();
}

static void oled_reset(void) {
    gpio_put(PIN_RST, 0);
    sleep_ms(20);
    gpio_put(PIN_RST, 1);
    sleep_ms(50);
}

static void sh1107_init(void) {
    oled_reset();

    // SH1107 init sequence for 64x128 mapped panels (Waveshare Pico-OLED-1.3)
    oled_write_cmd(0xAE);          // Display OFF
    oled_write_cmd(0xDC); oled_write_cmd(0x00); // Display start line
    oled_write_cmd(0x81); oled_write_cmd(0x4F); // Contrast
    oled_write_cmd(0xA1);          // Segment remap
    oled_write_cmd(0xC8);          // COM scan direction
    oled_write_cmd(0xA8); oled_write_cmd(0x7F); // Multiplex ratio (128)
    oled_write_cmd(0xD3); oled_write_cmd(0x00); // Display offset for 64x128 mapping
    oled_write_cmd(0xD5); oled_write_cmd(0x51); // Display clock
    oled_write_cmd(0xD9); oled_write_cmd(0x22); // Pre-charge
    oled_write_cmd(0xDB); oled_write_cmd(0x35); // VCOM
    oled_write_cmd(0xAD); oled_write_cmd(0x80); // DC-DC
    oled_write_cmd(0xA4);          // Display follows RAM
    oled_write_cmd(0xA6);          // Normal display
    oled_write_cmd(0xAF);          // Display ON
}

static void oled_set_column(int x) {
    if (x < 0) {
        x = 0;
    }
    if (x > 127) {
        x = 127;
    }
    oled_write_cmd((uint8_t)(0x00 | (x & 0x0F)));        // low nibble
    oled_write_cmd((uint8_t)(0x10 | ((x >> 4) & 0x0F))); // high nibble
}

static void oled_flush(void) {
    for (int page = 0; page < OLED_PAGES; ++page) {
        oled_write_cmd(0xB0 | page); // page address
        oled_set_column(OLED_X_OFFSET);
        oled_write_data(&framebuffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}

static inline void fb_clear(void) {
    memset(framebuffer, 0x00, sizeof(framebuffer));
}

static inline void fb_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }
    uint8_t *cell = &framebuffer[(y / 8) * OLED_WIDTH + x];
    const uint8_t mask = (uint8_t)(1u << (y & 7));
    if (on) {
        *cell |= mask;
    } else {
        *cell &= (uint8_t)(~mask);
    }
}

static void fb_fill_rect(int x, int y, int w, int h, bool on) {
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            fb_set_pixel(xx, yy, on);
        }
    }
}

static void fb_draw_rect(int x, int y, int w, int h, bool on) {
    for (int xx = x; xx < x + w; ++xx) {
        fb_set_pixel(xx, y, on);
        fb_set_pixel(xx, y + h - 1, on);
    }
    for (int yy = y; yy < y + h; ++yy) {
        fb_set_pixel(x, yy, on);
        fb_set_pixel(x + w - 1, yy, on);
    }
}

typedef struct {
    char c;
    uint8_t row[7];
} Glyph;

// 5x7 bitmap font rows; bit 4 is leftmost pixel, bit 0 is rightmost.
static const Glyph kFont[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
    {'/', {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x10, 0x13, 0x11, 0x0E}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
};

static const Glyph* find_glyph(char c) {
    for (size_t i = 0; i < sizeof(kFont) / sizeof(kFont[0]); ++i) {
        if (kFont[i].c == c) {
            return &kFont[i];
        }
    }
    return &kFont[0];
}

static void fb_draw_char(int x, int y, char c, bool on, bool invert) {
    const Glyph* glyph = find_glyph(c);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            const bool bit = ((glyph->row[row] >> (4 - col)) & 0x01u) != 0;
            const bool pixel = invert ? !bit : bit;
            if (pixel) {
                fb_set_pixel(x + col, y + row, on);
            } else if (invert) {
                fb_set_pixel(x + col, y + row, !on);
            }
        }
    }
}

static void fb_draw_text(int x, int y, const char* text, bool on, bool invert) {
    for (size_t i = 0; text[i] != '\0'; ++i) {
        fb_draw_char(x + (int)i * 6, y, text[i], on, invert);
    }
}

static void draw_theme_screen(uint32_t tick) {
    fb_clear();

    // Outer frame
    fb_draw_rect(0, 0, OLED_WIDTH, OLED_HEIGHT, true);

    // Top status bar
    fb_fill_rect(1, 1, OLED_WIDTH - 20, 0, false);
    fb_draw_text(3, 6, " AXFER-IO", true, false);

    // Main card
    fb_draw_rect(3, 19, OLED_WIDTH - 6, 52, true);
    fb_draw_text(10, 26, "PICO2", true, false);
    fb_draw_text(10, 38, "OLED", true, false);
    fb_draw_text(10, 50, "READY", true, false);

    // Soft pulse separator for visual depth
    const int pulse = (int)((tick / 120) % 8);
    const int sep_y = 78;
    for (int x = 6; x < OLED_WIDTH - 6; ++x) {
        const bool on = ((x + pulse) % 8) < 3;
        if (on) {
            fb_set_pixel(x, sep_y, true);
        }
    }

    // Bottom progress rail
    fb_draw_rect(4, OLED_HEIGHT - 11, OLED_WIDTH - 8, 8, true);
    const int progress_max = OLED_WIDTH - 12;
    const int progress = (int)((tick / 40) % progress_max);
    fb_fill_rect(6, OLED_HEIGHT - 9, progress, 4, true);

    char status[22];
    snprintf(status, sizeof(status), "%02lu:%02lu:%02lu", (tick / 1000) / 3600,
             ((tick / 1000) / 60) % 60, (tick / 1000) % 60);
    fb_draw_text(8, OLED_HEIGHT - 24, status, true, false);

    oled_flush();
}

int main() {
    stdio_init_all();
    sleep_ms(1200);
    printf("axfer-io: SH1107 dark theme UI\n");

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 0);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);

    spi_init(OLED_SPI, 8 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    spi_set_format(OLED_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    sh1107_init();

    while (true) {
        const uint32_t tick = to_ms_since_boot(get_absolute_time());
        draw_theme_screen(tick);
        sleep_ms(40);
    }
}
