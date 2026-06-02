#include "systick.h"

#include "mcu.h"

static volatile uint32_t g_tick_ms = 0;

void SysTick_Handler(void) {
    ++g_tick_ms;
}

void systick_init(uint32_t cpu_hz) {
    SYST_RVR = (cpu_hz / 1000UL) - 1UL;
    SYST_CVR = 0UL;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

uint32_t systick_now_ms(void) {
    return g_tick_ms;
}

void systick_delay_ms(uint32_t ms) {
    const uint32_t start = systick_now_ms();
    while ((systick_now_ms() - start) < ms) {
    }
}
