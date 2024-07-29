CC = riscv64-unknown-elf-gcc
AR = riscv64-unknown-elf-gcc-ar
LD = riscv64-unknown-elf-ld
OBJDUMP = riscv64-unknown-elf-objdump
QEMU = qemu-system-riscv64

U = user
K = src

CFLAGS = -Wall -fno-builtin -ffreestanding -nostdlib -mcmodel=medany -MD
CFLAGS += -I$K -ggdb -fno-omit-frame-pointer
CFLAGS += -I.
OBJ = $K/init/entry.o $K/init/start.o $K/uart.o $K/kerneltrap.o
OBJ += $K/main.o $K/trap.o $K/stdlib.o $K/swtch.o $K/proc.o
OBJ += $K/kmem.o $K/vm.o $K/spinlock.o $K/syscall.o $K/trampoline.o
OBJ += $K/sleeplock.o $K/virtio_disk.o $K/bio.o $K/plic.o $K/fs.o $K/file.o
OBJ += $K/console.o $K/printf.o
kernel : kernel.ld $(OBJ)
	$(LD) -Tkernel.ld $(OBJ) -o $@
	$(OBJDUMP) -S $@ > $@.asm

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<


# user build
ULIB = $U/ulib.o $U/usys.o
_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

UPROGS=\
	$U/_init\


fs.img : mkfs/mkfs $(UPROGS)
	mkfs/mkfs fs.img $(UPROGS)
	
mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/conf.h
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

QFLAGS = -m 128m -smp 1 -machine virt
QFLAGS += -nographic -bios none 
QFLAGS += -global virtio-mmio.force-legacy=false
QFLAGS += -drive file=fs.img,if=none,format=raw,id=x0
QFLAGS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
qemu : kernel fs.img
	$(QEMU) $(QFLAGS) -kernel kernel 
qemu-gdb : kernel fs.img
	$(QEMU) $(QFLAGS) -s -S -kernel kernel 
.PHONY: clean
clean:
	-rm -rf kernel
	-find . -name *.o |xargs rm -rf
	-find . -name *.img |xargs rm -rf
	-find . -name *.d |xargs rm -rf
	-find . -name *.sym |xargs rm -rf
	-find . -name *.asm |xargs rm -rf
	-rm -rf mkfs/mkfs
