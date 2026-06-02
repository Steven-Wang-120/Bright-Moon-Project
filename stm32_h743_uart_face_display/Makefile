TARGET := stm32_h743_uart_face_display
BUILD_DIR := build

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size

CFLAGS := -mcpu=cortex-m7 -mthumb -mfloat-abi=soft -ffreestanding -fdata-sections -ffunction-sections -Os -Wall -Wextra -Iinc
ASFLAGS := -mcpu=cortex-m7 -mthumb
LDFLAGS := -mcpu=cortex-m7 -mthumb -mfloat-abi=soft -nostdlib -Wl,--gc-sections -Tstm32h743zi.ld -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))
OBJS += $(BUILD_DIR)/startup_stm32h743xx.o

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup_stm32h743xx.o: startup_stm32h743xx.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) stm32h743zi.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f openocd.cfg -c "program $< verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all flash clean
