#include "json_protocol.h"

static int is_space(char value) {
    return (value == ' ' || value == '\t' || value == '\r' || value == '\n') ? 1 : 0;
}

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

static void write_error(char *buffer, uint32_t buffer_size, const char *text) {
    if (buffer == 0 || buffer_size == 0U) {
        return;
    }

    uint32_t index = 0U;
    while (text[index] != '\0' && index + 1U < buffer_size) {
        buffer[index] = text[index];
        ++index;
    }
    buffer[index] = '\0';
}

static const char *skip_ws(const char *cursor) {
    while (*cursor != '\0' && is_space(*cursor)) {
        ++cursor;
    }
    return cursor;
}

static const char *find_key(const char *json, const char *key) {
    if (json == 0 || key == 0) {
        return 0;
    }

    while (*json != '\0') {
        if (*json == '"') {
            ++json;
            const char *candidate = json;
            const char *k = key;
            while (*candidate != '\0' && *k != '\0' && *candidate == *k) {
                ++candidate;
                ++k;
            }
            if (*k == '\0' && *candidate == '"') {
                ++candidate;
                candidate = skip_ws(candidate);
                if (*candidate == ':') {
                    return skip_ws(candidate + 1);
                }
            }
        }
        ++json;
    }

    return 0;
}

static int parse_string_value(
    const char *json,
    const char *key,
    char *out_text,
    uint32_t out_text_size
) {
    const char *cursor = find_key(json, key);
    if (cursor == 0 || *cursor != '"' || out_text == 0 || out_text_size == 0U) {
        return 0;
    }

    ++cursor;
    uint32_t index = 0U;
    while (*cursor != '\0' && *cursor != '"') {
        if (index + 1U >= out_text_size) {
            return 0;
        }
        out_text[index++] = *cursor++;
    }
    if (*cursor != '"') {
        return 0;
    }
    out_text[index] = '\0';
    return 1;
}

static int parse_int_value(const char *json, const char *key, int32_t *out_value) {
    const char *cursor = find_key(json, key);
    int32_t sign = 1;
    int32_t value = 0;
    int found_digit = 0;

    if (cursor == 0 || out_value == 0) {
        return 0;
    }

    if (*cursor == '-') {
        sign = -1;
        ++cursor;
    }

    while (*cursor >= '0' && *cursor <= '9') {
        found_digit = 1;
        value = (value * 10) + (int32_t)(*cursor - '0');
        ++cursor;
    }

    if (!found_digit) {
        return 0;
    }

    *out_value = value * sign;
    return 1;
}

bool json_protocol_parse(
    const char *line,
    ParsedCommand *out_command,
    char *error_out,
    uint32_t error_out_size
) {
    char face_text[24];
    char cmd_text[16];
    int32_t raw_value = 0;

    if (line == 0 || out_command == 0) {
        write_error(error_out, error_out_size, "null input");
        return false;
    }

    line = skip_ws(line);
    if (*line != '{') {
        write_error(error_out, error_out_size, "expected json object");
        return false;
    }

    out_command->command = COMMAND_SHOW;
    out_command->face = FACE_NEUTRAL;
    out_command->intensity = 60U;
    out_command->duration_ms = 0U;
    out_command->has_duration = false;

    if (parse_string_value(line, "cmd", cmd_text, sizeof(cmd_text))) {
        if (str_ieq(cmd_text, "show")) {
            out_command->command = COMMAND_SHOW;
        } else if (str_ieq(cmd_text, "clear")) {
            out_command->command = COMMAND_CLEAR;
        } else if (str_ieq(cmd_text, "ping")) {
            out_command->command = COMMAND_PING;
        } else {
            write_error(error_out, error_out_size, "unsupported cmd");
            return false;
        }
    }

    if (parse_int_value(line, "dur", &raw_value)) {
        if (raw_value < 0) {
            raw_value = 0;
        }
        out_command->duration_ms = (uint32_t)raw_value;
        out_command->has_duration = true;
    }

    if (parse_int_value(line, "intensity", &raw_value)) {
        out_command->intensity = emotion_clamp_intensity(raw_value);
    }

    if (out_command->command == COMMAND_SHOW) {
        if (parse_string_value(line, "face", face_text, sizeof(face_text)) ||
            parse_string_value(line, "emo", face_text, sizeof(face_text))) {
            out_command->face = emotion_parse_face(face_text);
            if (out_command->face == FACE_UNKNOWN) {
                write_error(error_out, error_out_size, "unsupported face");
                return false;
            }
        } else {
            write_error(error_out, error_out_size, "missing face/emo");
            return false;
        }
    }

    return true;
}
