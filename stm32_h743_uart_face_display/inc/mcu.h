#ifndef MCU_H
#define MCU_H

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))
#define BIT(n) (1UL << (n))

#define GPIOA_BASE 0x58020000UL
#define GPIOB_BASE 0x58020400UL
#define RCC_BASE 0x58024400UL
#define USART1_BASE 0x40011000UL

#define RCC_AHB4ENR REG32(RCC_BASE + 0xE0UL)
#define RCC_APB2ENR REG32(RCC_BASE + 0xF0UL)

#define RCC_AHB4ENR_GPIOAEN BIT(0)
#define RCC_AHB4ENR_GPIOBEN BIT(1)
#define RCC_APB2ENR_USART1EN BIT(4)

#define GPIO_MODER(base) REG32((base) + 0x00UL)
#define GPIO_OTYPER(base) REG32((base) + 0x04UL)
#define GPIO_OSPEEDR(base) REG32((base) + 0x08UL)
#define GPIO_PUPDR(base) REG32((base) + 0x0CUL)
#define GPIO_IDR(base) REG32((base) + 0x10UL)
#define GPIO_ODR(base) REG32((base) + 0x14UL)
#define GPIO_BSRR(base) REG32((base) + 0x18UL)
#define GPIO_AFRL(base) REG32((base) + 0x20UL)
#define GPIO_AFRH(base) REG32((base) + 0x24UL)

#define GPIO_MODE_INPUT 0x0UL
#define GPIO_MODE_OUTPUT 0x1UL
#define GPIO_MODE_ALT 0x2UL

#define GPIO_PULL_NONE 0x0UL
#define GPIO_PULL_UP 0x1UL

#define GPIO_OTYPE_PP 0x0UL
#define GPIO_OTYPE_OD 0x1UL

#define GPIO_SPEED_LOW 0x0UL
#define GPIO_SPEED_MEDIUM 0x1UL
#define GPIO_SPEED_HIGH 0x2UL
#define GPIO_SPEED_VERY_HIGH 0x3UL

#define USART_CR1 REG32(USART1_BASE + 0x00UL)
#define USART_CR2 REG32(USART1_BASE + 0x04UL)
#define USART_CR3 REG32(USART1_BASE + 0x08UL)
#define USART_BRR REG32(USART1_BASE + 0x0CUL)
#define USART_PRESC REG32(USART1_BASE + 0x2CUL)
#define USART_ISR REG32(USART1_BASE + 0x1CUL)
#define USART_ICR REG32(USART1_BASE + 0x20UL)
#define USART_RDR REG32(USART1_BASE + 0x24UL)
#define USART_TDR REG32(USART1_BASE + 0x28UL)

#define USART_CR1_UE BIT(0)
#define USART_CR1_RE BIT(2)
#define USART_CR1_TE BIT(3)

#define USART_ISR_PE BIT(0)
#define USART_ISR_FE BIT(1)
#define USART_ISR_NE BIT(2)
#define USART_ISR_ORE BIT(3)
#define USART_ISR_RXNE_RXFNE BIT(5)
#define USART_ISR_TC BIT(6)
#define USART_ISR_TXE_TXFNF BIT(7)

#define USART_ICR_PECF BIT(0)
#define USART_ICR_FECF BIT(1)
#define USART_ICR_NECF BIT(2)
#define USART_ICR_ORECF BIT(3)

#define SYST_CSR REG32(0xE000E010UL)
#define SYST_RVR REG32(0xE000E014UL)
#define SYST_CVR REG32(0xE000E018UL)

#define SYST_CSR_ENABLE BIT(0)
#define SYST_CSR_TICKINT BIT(1)
#define SYST_CSR_CLKSOURCE BIT(2)

static inline void gpio_set_mode(uint32_t gpio_base, uint8_t pin, uint32_t mode) {
    const uint32_t shift = (uint32_t)pin * 2U;
    uint32_t moder = GPIO_MODER(gpio_base);
    moder &= ~(0x3UL << shift);
    moder |= ((mode & 0x3UL) << shift);
    GPIO_MODER(gpio_base) = moder;
}

static inline void gpio_set_pull(uint32_t gpio_base, uint8_t pin, uint32_t pull) {
    const uint32_t shift = (uint32_t)pin * 2U;
    uint32_t pupdr = GPIO_PUPDR(gpio_base);
    pupdr &= ~(0x3UL << shift);
    pupdr |= ((pull & 0x3UL) << shift);
    GPIO_PUPDR(gpio_base) = pupdr;
}

static inline void gpio_set_speed(uint32_t gpio_base, uint8_t pin, uint32_t speed) {
    const uint32_t shift = (uint32_t)pin * 2U;
    uint32_t ospeedr = GPIO_OSPEEDR(gpio_base);
    ospeedr &= ~(0x3UL << shift);
    ospeedr |= ((speed & 0x3UL) << shift);
    GPIO_OSPEEDR(gpio_base) = ospeedr;
}

static inline void gpio_set_otype(uint32_t gpio_base, uint8_t pin, uint32_t otype) {
    if (otype == GPIO_OTYPE_OD) {
        GPIO_OTYPER(gpio_base) |= BIT(pin);
    } else {
        GPIO_OTYPER(gpio_base) &= ~BIT(pin);
    }
}

static inline void gpio_set_af(uint32_t gpio_base, uint8_t pin, uint8_t af) {
    const uint32_t shift = ((uint32_t)pin & 0x7UL) * 4U;
    if (pin < 8U) {
        uint32_t value = GPIO_AFRL(gpio_base);
        value &= ~(0xFUL << shift);
        value |= ((uint32_t)(af & 0xFUL) << shift);
        GPIO_AFRL(gpio_base) = value;
    } else {
        uint32_t value = GPIO_AFRH(gpio_base);
        value &= ~(0xFUL << shift);
        value |= ((uint32_t)(af & 0xFUL) << shift);
        GPIO_AFRH(gpio_base) = value;
    }
}

static inline void gpio_write_high(uint32_t gpio_base, uint8_t pin) {
    GPIO_BSRR(gpio_base) = BIT(pin);
}

static inline void gpio_write_low(uint32_t gpio_base, uint8_t pin) {
    GPIO_BSRR(gpio_base) = BIT((uint32_t)pin + 16U);
}

static inline uint8_t gpio_read(uint32_t gpio_base, uint8_t pin) {
    return (GPIO_IDR(gpio_base) & BIT(pin)) ? 1U : 0U;
}

#endif
