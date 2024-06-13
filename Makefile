CC = riscv64-unknown-elf-gcc
AR = riscv64-unknown-elf-gcc-ar
LD = riscv64-unknown-elf-ld
OBJDUMP = riscv64-unknown-elf-objdump
QEMU = qemu-system-riscv64

CFLAGS = -Wall -fno-builtin -ffreestanding -nostdlib -mcmodel=medany -MD
CFLAGS += -Isrc -ggdb -fno-omit-frame-pointer
OBJ = src/init/entry.o src/init/start.o src/uart.o src/kerneltrap.o
OBJ += src/main.o src/trap.o src/stdlib.o src/swtch.o src/proc.o src/utils.o
OBJ += src/kmem.o src/vm.o src/spinlock.o src/syscall.o src/trampoline.o
OBJ += src/sleeplock.o src/virtio_disk.o src/bio.o src/plic.o
kernel : kernel.ld $(OBJ)
	$(LD) -Tkernel.ld $(OBJ) -o $@
	$(OBJDUMP) -S $@ > $@.asm

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

QFLAGS = -m 128m -smp 1 -machine virt
QFLAGS += -nographic -bios none 
QFLAGS += -global virtio-mmio.force-legacy=false
QFLAGS += -drive file=fs.img,if=none,format=raw,id=x0
QFLAGS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
qemu : kernel fs.img
	$(QEMU) $(QFLAGS) -kernel kernel 
qemu-gdb : kernel
	$(QEMU) $(QFLAGS) -s -S -kernel kernel 
.PHONY: clean
clean:
	-rm -rf kernel
	-find . -name *.o |xargs rm -rf
	-find . -name *.d |xargs rm -rf
