#ifndef FACE_DISPLAY_H
#define FACE_DISPLAY_H

#include <stdint.h>

#include "emotion.h"

void face_display_init(void);
void face_display_show(FaceType face, uint8_t intensity);

#endif
