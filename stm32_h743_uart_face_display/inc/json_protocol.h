#ifndef JSON_PROTOCOL_H
#define JSON_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "emotion.h"

typedef enum {
    COMMAND_SHOW = 0,
    COMMAND_CLEAR = 1,
    COMMAND_PING = 2,
} CommandType;

typedef struct {
    CommandType command;
    FaceType face;
    uint8_t intensity;
    uint32_t duration_ms;
    bool has_duration;
} ParsedCommand;

bool json_protocol_parse(
    const char *line,
    ParsedCommand *out_command,
    char *error_out,
    uint32_t error_out_size
);

#endif
