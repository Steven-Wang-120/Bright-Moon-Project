#ifndef EMOTION_H
#define EMOTION_H

#include <stdint.h>

typedef enum {
    FACE_UNKNOWN = -1,
    FACE_HAPPY = 0,
    FACE_SAD = 1,
    FACE_ANGRY = 2,
    FACE_SURPRISED = 3,
    FACE_THINKING = 4,
    FACE_NEUTRAL = 5,
} FaceType;

FaceType emotion_parse_face(const char *name);
const char *emotion_name(FaceType face);
uint8_t emotion_clamp_intensity(int32_t value);

#endif
