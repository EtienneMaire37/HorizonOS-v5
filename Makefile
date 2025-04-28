CFLAGS := -std=gnu99 -nostdlib -ffreestanding -Wall -masm=intel -m32 -mno-ms-bitfields -mno-red-zone -mlong-double-80
DATE := `date +"%Y-%m-%d"`
CC := ./i486elfgcc/bin/i486-elf-gcc
LD := ./i486elfgcc/bin/i486-elf-ld

all: horizonos.iso

run: all
	mkdir debug -p
	qemu-system-x86_64                           		\
	-accel kvm                       					\
	-cpu host                                  			\
	-debugcon file:debug/${DATE}.log					\
	-m 4096                                        		\
	-hda horizonos.iso    								\
	-smp 8

horizonos.iso: rmbin src/tasks/bin/kernel32.elf
	mkdir bin -p

	nasm -f elf32 -o "bin/kernelentry.o" "src/kernel/kernelentry.asm"
	nasm -f elf32 -o "bin/gdt.o" "src/kernel/gdt/gdt.asm"
	nasm -f elf32 -o "bin/idt.o" "src/kernel/idt/idt.asm"
	nasm -f elf32 -o "bin/paging.o" "src/kernel/paging/paging.asm"
	 
	$(CC) -c "src/kernel/kmain.c" -o "bin/kmain.o" $(CFLAGS) -O3
	$(LD) -T src/kernel/link.ld 
	
	mkdir -p ./root/boot/grub
	mkdir -p ./bin/initrd

	cp src/tasks/bin/kernel32.elf ./bin/initrd/kernel32.elf
	cp resources/pci.ids ./bin/initrd/pci.ids

	tar -cvf ./root/boot/initrd.tar -C ./bin/initrd/ .
	
	cp ./bin/kernel.elf ./root/boot/kernel.elf
	cp ./src/kernel/grub.cfg ./root/boot/grub/grub.cfg
	 
	grub-mkrescue -o ./horizonos.iso ./root

src/tasks/bin/kernel32.elf: src/tasks/src/kernel32/* src/tasks/link.ld libc libm
	mkdir -p ./src/tasks/bin
	$(CC) -c "src/tasks/src/kernel32/main.c" -o "src/tasks/bin/kernel32.o" $(CFLAGS) -I"src/libc/include" -O3
	$(LD) -T src/tasks/link.ld -nostdlib -m elf_i386 \
    -o "src/tasks/bin/kernel32.elf" \
    "src/tasks/bin/kernel32.o" \
    "src/libc/lib/libc.o" \
    "src/libc/lib/libm.o"

src/libc/lib/libc.o: libc
src/libc/lib/libm.o: libm

libc: src/libc/src/* src/libc/include/*
	mkdir -p ./src/libc/lib
	$(CC) -c "src/libc/src/libc.c" -o "src/libc/lib/libc.o" -O0 $(CFLAGS)
libm: src/libc/src/* src/libc/include/*
	mkdir -p ./src/libc/lib
	$(CC) -c "src/libc/src/math.c" -o "src/libc/lib/libm.o" -O3 $(CFLAGS) -malign-double

rmbin:
	rm -rf ./bin/*
	rm -rf ./src/tasks/bin/*
	rm -rf ./src/libc/lib/*
	rm -rf ./initrd.tar

clean: rmbin
	rm -f ./horizonos.iso