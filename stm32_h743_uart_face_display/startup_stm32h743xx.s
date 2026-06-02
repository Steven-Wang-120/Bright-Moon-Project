.syntax unified
.cpu cortex-m7
.thumb

.global __StackTop
.global Reset_Handler

.extern main
.extern SystemInit
.extern SysTick_Handler

.extern _sidata
.extern _sdata
.extern _edata
.extern _sbss
.extern _ebss

.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
g_pfnVectors:
  .word __StackTop
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler
  .rept 166
  .word Default_Handler
  .endr
.size g_pfnVectors, . - g_pfnVectors

.section .text.Reset_Handler, "ax", %progbits
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
1:
  cmp r0, r1
  bcs 2f
  ldr r3, [r2], #4
  str r3, [r0], #4
  b 1b
2:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
3:
  cmp r0, r1
  bcs 4f
  str r2, [r0], #4
  b 3b
4:
  bl SystemInit
  bl main
5:
  b 5b
.size Reset_Handler, . - Reset_Handler

.section .text.Default_Handler, "ax", %progbits
.type Default_Handler, %function
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
.size Default_Handler, . - Default_Handler

.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
