#include "uart.h"

#include "board.h"
#include "mcu.h"

static void uart_wait_txe(void) {
    while ((USART_ISR & USART_ISR_TXE_TXFNF) == 0U) {
    }
}

void uart1_init(uint32_t baudrate) {
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    gpio_set_mode(UART_GPIO_BASE, UART_TX_PIN, GPIO_MODE_ALT);
    gpio_set_af(UART_GPIO_BASE, UART_TX_PIN, UART_AF);
    gpio_set_otype(UART_GPIO_BASE, UART_TX_PIN, GPIO_OTYPE_PP);
    gpio_set_speed(UART_GPIO_BASE, UART_TX_PIN, GPIO_SPEED_VERY_HIGH);
    gpio_set_pull(UART_GPIO_BASE, UART_TX_PIN, GPIO_PULL_NONE);

    gpio_set_mode(UART_GPIO_BASE, UART_RX_PIN, GPIO_MODE_ALT);
    gpio_set_af(UART_GPIO_BASE, UART_RX_PIN, UART_AF);
    gpio_set_otype(UART_GPIO_BASE, UART_RX_PIN, GPIO_OTYPE_PP);
    gpio_set_speed(UART_GPIO_BASE, UART_RX_PIN, GPIO_SPEED_VERY_HIGH);
    gpio_set_pull(UART_GPIO_BASE, UART_RX_PIN, GPIO_PULL_UP);

    USART_CR1 = 0U;
    USART_CR2 = 0U;
    USART_CR3 = 0U;
    USART_PRESC = 0U;
    USART_BRR = (SYSCLK_HZ + (baudrate / 2UL)) / baudrate;
    USART_ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
    USART_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

bool uart1_try_read(uint8_t *out_byte) {
    if ((USART_ISR & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE | USART_ISR_PE)) != 0U) {
        (void)USART_RDR;
        USART_ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
        return false;
    }

    if ((USART_ISR & USART_ISR_RXNE_RXFNE) == 0U) {
        return false;
    }

    *out_byte = (uint8_t)(USART_RDR & 0xFFU);
    return true;
}

void uart1_write_byte(uint8_t byte) {
    uart_wait_txe();
    USART_TDR = byte;
}

void uart1_write_cstr(const char *text) {
    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        uart1_write_byte((uint8_t)*text);
        ++text;
    }
}

void uart1_write_u32(uint32_t value) {
    char buffer[10];
    uint32_t index = 0U;

    if (value == 0U) {
        uart1_write_byte((uint8_t)'0');
        return;
    }

    while (value > 0U && index < (uint32_t)sizeof(buffer)) {
        buffer[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U) {
        --index;
        uart1_write_byte((uint8_t)buffer[index]);
    }
}

void uart1_write_line(const char *text) {
    uart1_write_cstr(text);
    uart1_write_cstr("\r\n");
}
