#include "board.h"
#include "face_display.h"
#include "json_protocol.h"
#include "ssd1306.h"
#include "systick.h"
#include "uart.h"
#include "version.h"

static uint32_t g_revert_deadline_ms = 0U;

void SystemInit(void) {
}

static void write_ack_show(const ParsedCommand *command) {
    uart1_write_cstr("ACK show ");
    uart1_write_cstr(emotion_name(command->face));
    uart1_write_cstr(" ");
    uart1_write_u32(command->intensity);
    if (command->has_duration) {
        uart1_write_cstr(" ");
        uart1_write_u32(command->duration_ms);
    }
    uart1_write_cstr("\r\n");
}

static void write_error(const char *reason) {
    uart1_write_cstr("ERR ");
    uart1_write_cstr(reason);
    uart1_write_cstr("\r\n");
}

static void process_command(const ParsedCommand *command) {
    switch (command->command) {
        case COMMAND_CLEAR:
            face_display_show(FACE_UNKNOWN, 0U);
            g_revert_deadline_ms = 0U;
            uart1_write_line("ACK clear");
            break;
        case COMMAND_PING:
            uart1_write_line("ACK ping");
            break;
        case COMMAND_SHOW:
        default:
            face_display_show(command->face, command->intensity);
            if (command->has_duration && command->duration_ms > 0U) {
                g_revert_deadline_ms = systick_now_ms() + command->duration_ms;
            } else {
                g_revert_deadline_ms = 0U;
            }
            write_ack_show(command);
            break;
    }
}

static void process_line(const char *line) {
    ParsedCommand command;
    char error[48];

    if (!json_protocol_parse(line, &command, error, sizeof(error))) {
        write_error(error);
        return;
    }

    process_command(&command);
}

int main(void) {
    char rx_line[RX_LINE_MAX];
    uint32_t rx_len = 0U;

    systick_init(SYSCLK_HZ);
    uart1_init(UART_BAUDRATE);
    ssd1306_init();
    face_display_init();

    uart1_write_cstr("READY ");
    uart1_write_cstr(FW_NAME);
    uart1_write_cstr(" v");
    uart1_write_cstr(FW_VERSION);
    uart1_write_line(" json-line stm32h743 uart1 pa9-pa10 oled pc8-pc9 flash-assets");

    while (1) {
        uint8_t byte = 0U;
        if (uart1_try_read(&byte)) {
            if (byte == '\r') {
                continue;
            }

            if (byte == '\n') {
                rx_line[rx_len] = '\0';
                if (rx_len > 0U) {
                    process_line(rx_line);
                }
                rx_len = 0U;
                continue;
            }

            if (rx_len + 1U >= RX_LINE_MAX) {
                rx_len = 0U;
                write_error("line too long");
                continue;
            }

            rx_line[rx_len++] = (char)byte;
        }

        if (g_revert_deadline_ms != 0U) {
            const uint32_t now = systick_now_ms();
            if ((int32_t)(now - g_revert_deadline_ms) >= 0) {
                g_revert_deadline_ms = 0U;
                face_display_show(FACE_NEUTRAL, 50U);
                uart1_write_line("ACK revert neutral 50");
            }
        }
    }
}
