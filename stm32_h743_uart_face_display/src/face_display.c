#include "face_display.h"

#include "ssd1306.h"

static void draw_face_outline(void) {
    ssd1306_draw_circle(64, 32, 30, true);
    ssd1306_draw_circle(64, 32, 29, true);
}

static void draw_eye_dot(int x, int y, int radius) {
    ssd1306_fill_circle(x, y, radius, true);
}

static void draw_eye_line(int x0, int y0, int x1, int y1) {
    ssd1306_draw_line(x0, y0, x1, y1, true);
    ssd1306_draw_line(x0, y0 + 1, x1, y1 + 1, true);
}

static void draw_happy(uint8_t intensity) {
    int mouth_peak = 47 - ((int)intensity / 10);
    draw_eye_dot(52, 24, 3);
    draw_eye_dot(76, 24, 3);
    ssd1306_draw_line(44, 42, 56, mouth_peak, true);
    ssd1306_draw_line(56, mouth_peak, 72, mouth_peak, true);
    ssd1306_draw_line(72, mouth_peak, 84, 42, true);
}

static void draw_sad(uint8_t intensity) {
    int mouth_low = 45 + ((int)intensity / 12);
    draw_eye_dot(52, 24, 3);
    draw_eye_dot(76, 24, 3);
    ssd1306_draw_line(44, 46, 56, mouth_low, true);
    ssd1306_draw_line(56, mouth_low, 72, mouth_low, true);
    ssd1306_draw_line(72, mouth_low, 84, 46, true);
}

static void draw_angry(uint8_t intensity) {
    int brow_drop = 2 + ((int)intensity / 20);
    draw_eye_line(44, 20 + brow_drop, 58, 16);
    draw_eye_line(70, 16, 84, 20 + brow_drop);
    draw_eye_line(48, 28, 58, 28);
    draw_eye_line(70, 28, 80, 28);
    ssd1306_draw_line(46, 46, 82, 46, true);
    ssd1306_draw_line(48, 48, 80, 48, true);
}

static void draw_surprised(uint8_t intensity) {
    int mouth_radius = 4 + ((int)intensity / 20);
    draw_eye_dot(52, 23, 4);
    draw_eye_dot(76, 23, 4);
    ssd1306_draw_circle(64, 44, mouth_radius, true);
    ssd1306_draw_circle(64, 44, mouth_radius - 1, true);
}

static void draw_thinking(uint8_t intensity) {
    int offset = (int)intensity / 25;
    draw_eye_line(46, 21, 58, 18 - offset);
    draw_eye_line(70, 20, 82, 20);
    draw_eye_line(48, 28, 58, 29);
    draw_eye_line(70, 28, 80, 27);
    ssd1306_draw_line(48, 46, 70, 44, true);
    ssd1306_draw_line(70, 44, 80, 47, true);
    ssd1306_fill_circle(92, 46, 1, true);
    ssd1306_fill_circle(97, 41, 2, true);
}

static void draw_neutral(uint8_t intensity) {
    int half_width = 14 + ((int)intensity / 20);
    draw_eye_dot(52, 24, 3);
    draw_eye_dot(76, 24, 3);
    ssd1306_draw_line(64 - half_width, 44, 64 + half_width, 44, true);
    ssd1306_draw_line(64 - half_width, 45, 64 + half_width, 45, true);
}

void face_display_init(void) {
    face_display_show(FACE_NEUTRAL, 50U);
}

void face_display_show(FaceType face, uint8_t intensity) {
    ssd1306_clear();

    if (face == FACE_UNKNOWN) {
        ssd1306_update();
        return;
    }

    draw_face_outline();

    switch (face) {
        case FACE_HAPPY:
            draw_happy(intensity);
            break;
        case FACE_SAD:
            draw_sad(intensity);
            break;
        case FACE_ANGRY:
            draw_angry(intensity);
            break;
        case FACE_SURPRISED:
            draw_surprised(intensity);
            break;
        case FACE_THINKING:
            draw_thinking(intensity);
            break;
        case FACE_NEUTRAL:
        default:
            draw_neutral(intensity);
            break;
    }

    ssd1306_update();
}
