#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_init(uint32_t cpu_hz);
uint32_t systick_now_ms(void);
void systick_delay_ms(uint32_t ms);

#endif
