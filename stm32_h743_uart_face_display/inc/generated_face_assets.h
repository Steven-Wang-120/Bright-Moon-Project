#ifndef GENERATED_FACE_ASSETS_H
#define GENERATED_FACE_ASSETS_H

#include <stdint.h>

#include "emotion.h"

typedef struct {
    FaceType face;
    uint8_t intensity;
    const uint8_t *bitmap;
} GeneratedFaceAsset;

extern const GeneratedFaceAsset g_generated_face_assets[];
extern const uint16_t g_generated_face_asset_count;

#endif
