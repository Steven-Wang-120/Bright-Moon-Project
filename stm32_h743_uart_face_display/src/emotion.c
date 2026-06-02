#include "emotion.h"

static char to_lower_ascii(char value) {
    if (value >= 'A' && value <= 'Z') {
        return (char)(value - 'A' + 'a');
    }
    return value;
}

static int str_ieq(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (to_lower_ascii(*left) != to_lower_ascii(*right)) {
            return 0;
        }
        ++left;
        ++right;
    }
    return (*left == '\0' && *right == '\0') ? 1 : 0;
}

FaceType emotion_parse_face(const char *name) {
    if (name == 0) {
        return FACE_UNKNOWN;
    }
    if (str_ieq(name, "happy")) {
        return FACE_HAPPY;
    }
    if (str_ieq(name, "sad")) {
        return FACE_SAD;
    }
    if (str_ieq(name, "angry")) {
        return FACE_ANGRY;
    }
    if (str_ieq(name, "surprised")) {
        return FACE_SURPRISED;
    }
    if (str_ieq(name, "thinking")) {
        return FACE_THINKING;
    }
    if (str_ieq(name, "neutral")) {
        return FACE_NEUTRAL;
    }
    return FACE_UNKNOWN;
}

const char *emotion_name(FaceType face) {
    switch (face) {
        case FACE_HAPPY:
            return "happy";
        case FACE_SAD:
            return "sad";
        case FACE_ANGRY:
            return "angry";
        case FACE_SURPRISED:
            return "surprised";
        case FACE_THINKING:
            return "thinking";
        case FACE_NEUTRAL:
            return "neutral";
        default:
            return "unknown";
    }
}

uint8_t emotion_clamp_intensity(int32_t value) {
    if (value < 0) {
        return 0U;
    }
    if (value > 100) {
        return 100U;
    }
    return (uint8_t)value;
}
