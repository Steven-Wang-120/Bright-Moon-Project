#include "face_display.h"

#include "face_assets.h"
#include "ssd1306.h"

void face_display_init(void) {
    face_display_show(FACE_NEUTRAL, 50U);
}

void face_display_show(FaceType face, uint8_t intensity) {
    const GeneratedFaceAsset *asset = 0;

    if (face == FACE_UNKNOWN) {
        ssd1306_clear();
        ssd1306_update();
        return;
    }

    asset = face_assets_find_best_match(face, intensity);
    if (asset == 0 || asset->bitmap == 0) {
        ssd1306_clear();
        ssd1306_update();
        return;
    }

    ssd1306_load_bitmap(asset->bitmap);
    ssd1306_update();
}
