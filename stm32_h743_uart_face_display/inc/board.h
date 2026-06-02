#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#include "mcu.h"

#define SYSCLK_HZ 64000000UL
#define UART_BAUDRATE 115200UL

#define UART_GPIO_BASE GPIOA_BASE
#define UART_TX_PIN 9U
#define UART_RX_PIN 10U
#define UART_AF 7U

#define OLED_GPIO_BASE GPIOB_BASE
#define OLED_SCL_PIN 8U
#define OLED_SDA_PIN 9U
#define OLED_I2C_ADDRESS 0x3CU

#define RX_LINE_MAX 256U

#endif
