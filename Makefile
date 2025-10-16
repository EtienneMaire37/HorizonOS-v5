CFLAGS := -std=gnu99 -nostdlib -ffreestanding -masm=intel -m32 -mno-ms-bitfields -mno-red-zone -mlong-double-80 -fno-omit-frame-pointer -march=i686 # -fbuiltin -fno-builtin-fork
DATE := `date +"%Y-%m-%d"`
CROSSGCC := ./i686elfgcc/bin/i686-elf-gcc
CROSSLD := ./i686elfgcc/bin/i686-elf-ld
CROSSNM := ./i686elfgcc/bin/i686-elf-nm
CROSSAR := ./i686elfgcc/bin/i686-elf-ar
USERGCC := 
CLOGLEVEL := 

all: $(CROSSGCC) $(USERGCC) horizonos.iso

run:
	mkdir debug -p
	qemu-system-x86_64                           		\
	-accel kvm                       					\
	-cpu host                                  			\
	-debugcon file:debug/latest.log						\
	-m 128                                        		\
	-drive file=horizonos.iso,index=0,media=disk,format=raw \
	-smp 8 \
	-d cpu

horizonos.iso: rmbin src/tasks/bin/kernel32.elf resources/pci.ids
	mkdir bin -p

	nasm -f elf32 -o "bin/kernelentry.o" "src/kernel/kernelentry.asm"
	nasm -f elf32 -o "bin/gdt.o" "src/kernel/gdt/gdt.asm"
	nasm -f elf32 -o "bin/idt.o" "src/kernel/int/idt.asm"
	nasm -f elf32 -o "bin/paging.o" "src/kernel/paging/paging.asm"
	nasm -f elf32 -o "bin/context_switch.o" "src/kernel/multitasking/context_switch.asm"
	nasm -f elf32 -o "bin/registers.o" "src/kernel/cpu/registers.asm"
	nasm -f elf32 -o "bin/sse.o" "src/kernel/fpu/sse.asm"
	 
	$(CROSSGCC) -c "src/kernel/kmain.c" -o "bin/kernel.o" $(CFLAGS) \
	-Ofast -Wall -Werror \
	-Wno-stringop-overflow -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Wno-unused-value -Wno-comment -Wno-nonnull-compare -Wno-format \
	$(CLOGLEVEL)
	
	$(CROSSGCC) -T src/kernel/link.ld \
	-ffreestanding -nostdlib \
	"bin/kernelentry.o" \
 	"bin/kernel.o" \
	"bin/gdt.o" \
	"bin/idt.o"  \
	"bin/paging.o" \
    "bin/context_switch.o" \
    "bin/registers.o"  \
	"bin/sse.o"  \
	-lgcc
	
	mkdir -p ./root/boot/grub
	mkdir -p ./bin/initrd

	rm src/tasks/bin/*.o
	cp src/tasks/bin/ ./bin/initrd/ -r
	cp resources/pci.ids ./bin/initrd/pci.ids
	$(CROSSNM) -n --defined-only -C bin/kernel.elf > ./bin/initrd/symbols.txt

	tar --transform 's|^\./||' -cvf ./root/boot/initrd.tar -C ./bin/initrd .
	
	cp ./bin/kernel.elf ./root/boot/kernel.elf
	cp ./src/kernel/grub.cfg ./root/boot/grub/grub.cfg
	 
	grub-mkrescue -o ./horizonos.iso ./root

src/tasks/bin/kernel32.elf: src/tasks/src/kernel32/* src/tasks/bin/shell src/tasks/bin/echo src/tasks/bin/ls src/tasks/bin/cat src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/kernel32/main.c" -o "src/tasks/bin/kernel32.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/kernel32.elf" \
    "src/tasks/bin/kernel32.o" \
    "src/libc/lib/libc.a" \
    "src/libc/lib/libm.a" \
	-ffreestanding -nostdlib \
	-lgcc
	mkdir -p ./bin/initrd

src/tasks/bin/echo: src/tasks/src/echo/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/echo/main.c" -o "src/tasks/bin/echo.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/echo" \
    "src/tasks/bin/echo.o" \
    "src/libc/lib/libc.a" \
	-ffreestanding -nostdlib \
	-lgcc

src/tasks/bin/ls: src/tasks/src/ls/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/ls/main.c" -o "src/tasks/bin/ls.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/ls" \
    "src/tasks/bin/ls.o" \
    "src/libc/lib/libc.a" \
	-ffreestanding -nostdlib \
	-lgcc

src/tasks/bin/cat: src/tasks/src/cat/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/cat/main.c" -o "src/tasks/bin/cat.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/cat" \
    "src/tasks/bin/cat.o" \
    "src/libc/lib/libc.a" \
	-ffreestanding -nostdlib \
	-lgcc

src/tasks/bin/shell: src/tasks/src/shell/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/shell/main.c" -o "src/tasks/bin/shell.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/shell" \
    "src/tasks/bin/shell.o" \
    "src/libc/lib/libc.a" \
	-ffreestanding -nostdlib \
	-lgcc

src/libc/lib/libc.a: src/libc/src/* src/libc/include/*
	mkdir -p ./src/libc/lib
	nasm -f elf32 -o "src/libc/lib/crt0.o" "src/libc/src/crt0.asm"
	$(CROSSGCC) -c "src/libc/src/libc.c" -o "src/libc/lib/clibc.o" -O3 $(CFLAGS)
	$(CROSSGCC) "src/libc/lib/crt0.o" "src/libc/lib/clibc.o" -o "src/libc/lib/libc.o" -r
	$(CROSSAR) rcs "src/libc/lib/libc.a" "src/libc/lib/libc.o"
src/libc/lib/libm.a: src/libc/src/* src/libc/include/*
	mkdir -p ./src/libc/lib
	$(CROSSGCC) -c "src/libc/src/math.c" -o "src/libc/lib/libm.o" -O3 $(CFLAGS) -malign-double
	$(CROSSAR) rcs "src/libc/lib/libm.a" "src/libc/lib/libm.o"

$(CROSSGCC):
	sh install-cross-compiler.sh

$(USERGCC):
	rm -rf build-area horizonos-toolchain
	sh install-custom-toolchain.sh

resources/pci.ids:
	mkdir -p resources
	wget https://raw.githubusercontent.com/pciutils/pciids/refs/heads/master/pci.ids -O ./resources/pci.ids

rmbin:
	rm -rf ./bin/*
	rm -rf ./src/tasks/bin/*
	rm -rf ./src/libc/lib/*
	rm -rf ./initrd.tar

clean: rmbin
	rm -f ./horizonos.iso
