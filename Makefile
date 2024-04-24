CC = riscv64-unknown-elf-gcc
AR = riscv64-unknown-elf-gcc-ar
LD = riscv64-unknown-elf-ld
QEMU = qemu-system-riscv64

CFLAGS = -Wall -fno-builtin -ffreestanding -nostdlib -mcmodel=medany -MD
CFLAGS += -Isrc -ggdb -fno-omit-frame-pointer
OBJ = src/init/entry.o src/init/start.o src/uart.o # this should be firt
kernel : kernel.ld $(OBJ)
	$(LD) -Tkernel.ld $(OBJ) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o : %.s
	$(CC) $(CFLAGS) -c -o $@ $<

QFLAGS = \
	-m 128m -smp 1 -machine virt \
	-nographic -bios none 
qemu : kernel
	$(QEMU) $(QFLAGS) -kernel kernel 
qemu-gdb : kernel
	$(QEMU) $(QFLAGS) -s -S -kernel kernel 
.PHONY: clean
clean:
	-rm -rf kernel
	-find . -name *.o |xargs rm -rf
	-find . -name *.d |xargs rm -rf
