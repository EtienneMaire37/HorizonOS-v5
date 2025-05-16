CFLAGS := -std=gnu99 -nostdlib -ffreestanding -masm=intel -m32 -mno-ms-bitfields -mno-red-zone -mlong-double-80 -fno-omit-frame-pointer
DATE := `date +"%Y-%m-%d"`
CC := ./i486elfgcc/bin/i486-elf-gcc
LD := ./i486elfgcc/bin/i486-elf-ld
AR := ./i486elfgcc/bin/i486-elf-ar

all: horizonos.iso

run:
	mkdir debug -p
	qemu-system-x86_64                           		\
	-accel kvm                       					\
	-cpu host                                  			\
	-debugcon file:debug/${DATE}.log					\
	-m 4096                                        		\
	-hda horizonos.iso    								\
	-smp 8

horizonos.iso: rmbin src/tasks/bin/kernel32.elf resources/pci.ids
	mkdir bin -p

	nasm -f elf32 -o "bin/kernelentry.o" "src/kernel/kernelentry.asm"
	nasm -f elf32 -o "bin/gdt.o" "src/kernel/gdt/gdt.asm"
	nasm -f elf32 -o "bin/idt.o" "src/kernel/idt/idt.asm"
	nasm -f elf32 -o "bin/paging.o" "src/kernel/paging/paging.asm"
	 
	$(CC) -c "src/kernel/kmain.c" -o "bin/kmain.o" $(CFLAGS) -O3
	$(LD) -T src/kernel/link.ld "src/libc/lib/libm.a"
	
	mkdir -p ./root/boot/grub
	mkdir -p ./bin/initrd

	cp src/tasks/bin/kernel32.elf ./bin/initrd/kernel32.elf
	cp resources/pci.ids ./bin/initrd/pci.ids
	nm -n --defined-only -C bin/kernel.elf > ./bin/initrd/symbols.txt

	tar -cvf ./root/boot/initrd.tar -C ./bin/initrd/ .
	
	cp ./bin/kernel.elf ./root/boot/kernel.elf
	cp ./src/kernel/grub.cfg ./root/boot/grub/grub.cfg
	 
	grub-mkrescue -o ./horizonos.iso ./root

src/tasks/bin/kernel32.elf: src/tasks/src/kernel32/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CC) -c "src/tasks/src/kernel32/main.c" -o "src/tasks/bin/kernel32.o" $(CFLAGS) -I"src/libc/include" -O3
	$(LD) -T src/tasks/link.ld -m elf_i386 \
    -o "src/tasks/bin/kernel32.elf" \
    "src/tasks/bin/kernel32.o" \
    "src/libc/lib/libc.a" \
    "src/libc/lib/libm.a"
	mkdir -p ./bin/initrd
	nm -n --defined-only -C src/tasks/bin/kernel32.elf > ./bin/initrd/kernel32_symbols.txt

src/libc/lib/libc.a: src/libc/src/* src/libc/include/*
	mkdir -p ./src/libc/lib
	nasm -f elf32 -o "src/libc/lib/crt0.o" "src/libc/src/crt0.asm"
	$(CC) -c "src/libc/src/libc.c" -o "src/libc/lib/clibc.o" -O0 $(CFLAGS)
	$(LD) "src/libc/lib/crt0.o" "src/libc/lib/clibc.o" -m elf_i386 -o "src/libc/lib/libc.o" -r
	$(AR) rcs "src/libc/lib/libc.a" "src/libc/lib/libc.o"
src/libc/lib/libm.a: src/libc/src/* src/libc/include/*
	mkdir -p ./src/libc/lib
	$(CC) -c "src/libc/src/math.c" -o "src/libc/lib/libm.o" -O3 $(CFLAGS) -malign-double
	$(AR) rcs "src/libc/lib/libm.a" "src/libc/lib/libm.o"

resources/pci.ids:
	wget https://raw.githubusercontent.com/pciutils/pciids/refs/heads/master/pci.ids -O resources/pci.ids

rmbin:
	rm -rf ./bin/*
	rm -rf ./src/tasks/bin/*
	rm -rf ./src/libc/lib/*
	rm -rf ./initrd.tar

clean: rmbin
	rm -f ./horizonos.iso
