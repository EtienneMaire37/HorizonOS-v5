# HorizonOS v5
![MIT License](https://img.shields.io/badge/license-MIT-blue.svg) 
![x86](https://img.shields.io/badge/arch-x86-informational?logo=intel) 
![32-bit](https://img.shields.io/badge/bits-32-ff69b4) 
![Building](https://img.shields.io/badge/building-Yes-brightgreen) 
![QEMU](https://img.shields.io/badge/tested%20with-QEMU-red?logo=qemu) 
![NASM](https://img.shields.io/badge/assembler-NASM-orange) 
![GRUB](https://img.shields.io/badge/bootloader-GRUB-lightgrey)

A 32-bit monolithic kernel for x86 architecture developed by [EtienneMaire37](https://github.com/EtienneMaire37)

## Overview
HorizonOS is a hobby kernel targeting x86 systems, designed as a learning platform for low-level systems programming. Built from scratch and using the GRUB bootloader, it demonstrates core operating system concepts while maintaining simplicity and readability.

## Features

### Implemented
- Monolithic kernel design for single-processor systems
- Basic memory management (paging, frame allocation) supporting up to 4GB RAM
- Preemptive multitasking implementation
- Minimal ACPI parsing capabilities
- PS/2 keyboard driver
- Custom C library (libc) in development
- Basic math library (libm)

### Planned
- Multi-core support
- PCI device enumeration
- USB drivers and mouse support
- Network stack support and ethernet drivers
- Graphical display support
- Improved filesystem support

## Building HorizonOS

### Prerequisites
- i386-elf cross-compiler
- NASM assembler
- GRUB 2.0+
- QEMU (for emulation)

# Install build dependencies (Debian/Ubuntu)
```bash
sudo apt install nasm grub-pc-bin qemu-system-i386
```

# Set up cross-compiler (using included script)
```bash
sudo sh install-cross-compiler.sh
```

### Compilation
```bash
make all
```

## Running in QEMU
```bash
make run
```

The horizonos.iso file will be generated in the repository's root

This will:
1. Build the kernel ISO
2. Launch QEMU with:
   - 256MB RAM
   - CD-ROM boot
   - Serial output
   - VGA display
   - Logging to debug/ directory

## Contributing
Contributions are welcome ! Feel free to open issues and send pull requests

## License
Distributed under the MIT License. See `LICENSE` for more information.

---

**Note:** This is experimental software - use at your own risk. Not recommended for production environments or critical systems.
