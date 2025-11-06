CFLAGS := -std=gnu11 -nostdlib -ffreestanding -masm=intel -m64 -mno-ms-bitfields -mno-red-zone -mlong-double-80 -fno-omit-frame-pointer -mstackrealign -march=x86-64
DATE := `date +"%Y-%m-%d"`
CROSSGCC := ./crossgcc/bin/x86_64-elf-gcc
CROSSLD := ./crossgcc/bin/x86_64-elf-ld
CROSSNM := ./crossgcc/bin/x86_64-elf-nm
CROSSAR := ./crossgcc/bin/x86_64-elf-ar
CROSSSTRIP := ./crossgcc/bin/x86_64-elf-strip
DIR2FAT32 := ./dir2fat32/dir2fat32.sh
USERGCC := 
CLOGLEVEL := 
MKBOOTIMG := ./bootboot/mkbootimg/mkbootimg

all: horizonos.iso

run:
	mkdir debug -p
	qemu-system-x86_64                           		\
	-accel kvm                       					\
	-cpu host                                  			\
	-debugcon file:debug/latest.log						\
	-m 64                                        		\
	-drive file=horizonos.iso,index=0,media=disk,format=raw \
	-smp 8 \
	-d cpu

	// src/tasks/bin/start.elf
horizonos.iso: $(CROSSGCC) $(USERGCC) $(MKBOOTIMG) rmbin $(DIR2FAT32) resources/pci.ids
	mkdir bin -p

	nasm -f elf64 -o "bin/gdt.o" "src/kernel/gdt/gdt.asm"
	nasm -f elf64 -o "bin/idt.o" "src/kernel/int/idt.asm"
	nasm -f elf64 -o "bin/context_switch.o" "src/kernel/multitasking/context_switch.asm"
	nasm -f elf64 -o "bin/registers.o" "src/kernel/cpu/registers.asm"
	nasm -f elf64 -o "bin/sse.o" "src/kernel/fpu/sse.asm"

	$(CROSSGCC) -c "src/kernel/main.c" -o "bin/kernel.o" \
	-Wall -Werror -Wno-address-of-packed-member -fpic $(CFLAGS) -I./bootboot/dist/ \
	-Ofast \
	-Wno-stringop-overflow -Wno-unused-variable \
	$(CLOGLEVEL)

	$(CROSSGCC) -nostdlib -n -T src/kernel/link.ld -o bin/kernel.elf -ffreestanding \
	bin/kernel.o \
	"bin/gdt.o" \
	"bin/idt.o"  \
	"bin/context_switch.o" \
	"bin/registers.o"  \
	"bin/sse.o"  \
	-lgcc

# 	$(CROSSSTRIP) -s -K mmio -K fb -K bootboot -K environment -K initstack bin/kernel.elf

	mkdir -p ./bin/initrd

	rm -f src/tasks/bin/*.o
	cp src/tasks/bin/ ./bin/initrd/ -r
	cp resources/* ./bin/initrd/
	$(CROSSNM) -n --defined-only -C bin/kernel.elf > ./bin/initrd/symbols.txt

	mkdir -p ./root
	
	cp ./bin/kernel.elf ./bin/initrd/kernel.elf

	$(DIR2FAT32) bin/horizonos.bin 256 ./root
	
	$(MKBOOTIMG) src/kernel/bootboot.json horizonos.iso

	qemu-img convert -O vdi horizonos.iso horizonos.vdi

src/tasks/bin/start.elf: src/tasks/src/start/* src/tasks/bin/shell src/tasks/bin/echo src/tasks/bin/ls src/tasks/bin/cat src/tasks/bin/clear src/tasks/bin/printenv src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/start/main.c" -o "src/tasks/bin/start.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/start.elf" \
    "src/tasks/bin/start.o" \
    "src/libc/lib/libc.a" \
    "src/libc/lib/libm.a" \
	-ffreestanding -nostdlib \
	-lgcc

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

src/tasks/bin/clear: src/tasks/src/clear/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/clear/main.c" -o "src/tasks/bin/clear.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/clear" \
    "src/tasks/bin/clear.o" \
    "src/libc/lib/libc.a" \
	-ffreestanding -nostdlib \
	-lgcc

src/tasks/bin/printenv: src/tasks/src/printenv/* src/tasks/link.ld src/libc/lib/libc.a src/libc/lib/libm.a
	mkdir -p ./src/tasks/bin
	$(CROSSGCC) -c "src/tasks/src/printenv/main.c" -o "src/tasks/bin/printenv.o" $(CFLAGS) -I"src/libc/include" -Ofast
	$(CROSSGCC) -T src/tasks/link.ld \
    -o "src/tasks/bin/printenv" \
    "src/tasks/bin/printenv.o" \
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
	nasm -f elf64 -o "src/libc/lib/crt0.o" "src/libc/src/crt0.asm"
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

$(MKBOOTIMG):
	git clone https://gitlab.com/bztsrc/bootboot.git bootboot
	cd bootboot/mkbootimg && make

$(DIR2FAT32):
	git clone https://github.com/Othernet-Project/dir2fat32.git dir2fat32
	cd dir2fat32 
	make
	chmod +x dir2fat32.sh
	cd ..

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
	rm -f ./resources/pci.ids
	rm -rf ./bootboot
	rm -rf ./root