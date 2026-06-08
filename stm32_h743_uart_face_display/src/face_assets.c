#include "face_assets.h"

const GeneratedFaceAsset *face_assets_find_best_match(FaceType face, uint8_t intensity) {
    const GeneratedFaceAsset *best = 0;
    uint8_t best_delta = 0xFFU;

    for (uint16_t i = 0U; i < g_generated_face_asset_count; ++i) {
        const GeneratedFaceAsset *entry = &g_generated_face_assets[i];
        uint8_t delta = 0U;

        if (entry->face != face) {
            continue;
        }

        delta = (entry->intensity > intensity)
                    ? (uint8_t)(entry->intensity - intensity)
                    : (uint8_t)(intensity - entry->intensity);

        if (best == 0 || delta < best_delta) {
            best = entry;
            best_delta = delta;
            if (delta == 0U) {
                break;
            }
        }
    }

    return best;
}
