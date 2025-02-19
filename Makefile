CC := i386-elf-gcc
ASM := nasm
CFLAGS := -std=gnu99 -nostdlib -ffreestanding -Wall -masm=intel -m32 -mno-ms-bitfields -mno-red-zone
DATE := `date +"%Y-%m-%d"`

all: horizonos.iso

run: all
	mkdir debug -p
	qemu-system-i386                               		\
	-accel tcg,thread=single                       		\
	-cpu core2duo                                  		\
	-debugcon file:debug/${DATE}.log					\
	-m 256                                        		\
	-drive format=raw,media=cdrom,file=horizonos.iso    \
	-serial stdio                                  		\
	-smp 1                                         		\
	-usb                                           		\
	-vga std 

horizonos.iso: rmbin src/tasks/bin/taskA.elf src/tasks/bin/taskB.elf
	mkdir bin -p

	$(ASM) -f elf32 -o "bin/kernelentry.o" "src/kernel/kernelentry.asm"
	$(ASM) -f elf32 -o "bin/gdt.o" "src/kernel/GDT/gdt.asm"
	$(ASM) -f elf32 -o "bin/idt.o" "src/kernel/IDT/idt.asm"
	$(ASM) -f elf32 -o "bin/paging.o" "src/kernel/paging/paging.asm"
	 
	$(CC) -c "src/kernel/kmain.c" -o "bin/kmain.o" $(CFLAGS)
	 
	ld -T src/link.ld -m elf_i386 
	
	mkdir -p ./root/boot/grub
	mkdir -p ./bin/initrd

	cp src/tasks/bin/taskA.elf ./bin/initrd/taskA.elf
	cp src/tasks/bin/taskB.elf ./bin/initrd/taskB.elf
	cp resources/pci.ids ./bin/initrd/pci.ids

	tar -cvf ./root/boot/initrd.tar ./bin/initrd/*
	
	cp ./bin/kernel.elf ./root/boot/kernel.elf
	cp ./src/grub.cfg ./root/boot/grub/grub.cfg
	 
	grub-mkrescue -o ./horizonos.iso ./root

src/tasks/bin/taskA.elf: src/tasks/src/taskA.asm src/tasks/link.ld
	$(ASM) -f elf32 -o "src/tasks/bin/taskA.o" "src/tasks/src/taskA.asm"
	ld -T src/tasks/link.ld -m elf_i386 -o "src/tasks/bin/taskA.elf" "src/tasks/bin/taskA.o"
src/tasks/bin/taskB.elf: src/tasks/src/taskB.asm src/tasks/link.ld
	$(ASM) -f elf32 -o "src/tasks/bin/taskB.o" "src/tasks/src/taskB.asm"
	ld -T src/tasks/link.ld -m elf_i386 -o "src/tasks/bin/taskB.elf" "src/tasks/bin/taskB.o"

rmbin:
	rm -rf ./bin/*
	rm -rf ./src/libc/lib/*
	rm -rf ./initrd.tar

clean: rmbin
	rm -f ./horizonos.iso