#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

void uart1_init(uint32_t baudrate);
bool uart1_try_read(uint8_t *out_byte);
void uart1_write_byte(uint8_t byte);
void uart1_write_cstr(const char *text);
void uart1_write_u32(uint32_t value);
void uart1_write_line(const char *text);

#endif
