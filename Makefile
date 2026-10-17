# Настройки
CC = i686-linux-gnu-gcc
ASM = nasm
CFLAGS = -ffreestanding -nostdlib -I./include -O0 -g -Wall -Wextra
ASMFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib -lgcc -ffreestanding
QEMU = qemu-system-i386

BUILD_DIR = build
SRC_DIR = src

OBJECTS = $(BUILD_DIR)/boot.o \
          $(BUILD_DIR)/kernel.o \
          $(BUILD_DIR)/screen.o \
          $(BUILD_DIR)/serial.o \
          $(BUILD_DIR)/memory.o \
          $(BUILD_DIR)/string.o \
          $(BUILD_DIR)/shell.o \
          $(BUILD_DIR)/fs.o \
          $(BUILD_DIR)/kheap.o \
          $(BUILD_DIR)/interrupts.o \
          $(BUILD_DIR)/interrupts_asm.o \
          $(BUILD_DIR)/process.o \
         $(BUILD_DIR)/switch.o \
         $(BUILD_DIR)/syscall.o \
        $(BUILD_DIR)/keyboard.o

$(shell mkdir -p $(BUILD_DIR))

all: myos.kernel

$(BUILD_DIR)/switch.o: $(SRC_DIR)/kernel/switch.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: $(SRC_DIR)/kernel/syscall.c include/syscall.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: $(SRC_DIR)/kernel/keyboard.c include/keyboard.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupts.o: $(SRC_DIR)/kernel/interrupts.c include/interrupts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupts_asm.o: $(SRC_DIR)/kernel/interrupts.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/process.o: $(SRC_DIR)/kernel/process.c include/process.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot/boot.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/screen.o: $(SRC_DIR)/kernel/screen.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/serial.o: $(SRC_DIR)/kernel/serial.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/memory.o: $(SRC_DIR)/kernel/memory.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/string.o: $(SRC_DIR)/kernel/string.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell.o: $(SRC_DIR)/kernel/shell.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fs.o: $(SRC_DIR)/kernel/fs.c include/fs.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kheap.o: $(SRC_DIR)/kernel/kheap.c include/kheap.h include/string.h
	$(CC) $(CFLAGS) -c $< -o $@

myos.kernel: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

run: myos.kernel
	$(QEMU) -kernel myos.kernel -serial mon:stdio -vga std

clean:
	rm -rf $(BUILD_DIR)/* myos.kernel myos.iso iso

.PHONY: all run clean
