# HorizonOS v5
<div align="center">
   
   ![MIT License](https://img.shields.io/badge/license-MIT-yellow.svg) 
   ![x86](https://img.shields.io/badge/arch-x86-informational) 
   ![GitHub Contributors](https://img.shields.io/github/contributors/EtienneMaire37/HorizonOS-v5?color=blue)
   ![Monthly Commits](https://img.shields.io/github/commit-activity/m/EtienneMaire37/HorizonOS-v5?color=orange)
   ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/EtienneMaire37/HorizonOS-v5/.github%2Fworkflows%2Fmakefile.yml)
</div>

A 32-bit monolithic kernel for the x86 architecture

## Overview 🌟
HorizonOS is a hobby kernel targeting x86 systems, built with simplicity and readability in mind.

## Features 🛠️

### Implemented ✅
- Single core execution
- Basic memory management supporting up to 4GB RAM
- Preemptive multitasking implementation
- Basic ACPI parsing
- PS/2 drivers (only keyboard for now)
- Custom C standard library (libc and libm) under development

### Planned 📅
- Multi-core support (SMP)
- PCI device enumeration
- USB drivers and mouse support
- Network stack support + ethernet and wifi drivers
- Graphical display support
- Improved filesystem and VFS support
- PATA/SATA drivers
- Full ACPI support (probably with uACPI or LAI)
- Threading API
- Complete (or near complete) libc and libstdc++ implementations
- System idle task and CPU usage management (including process blocking)
- General system optimizations
- Porting GCC, Make, and other third-party software
- GPU drivers (NVIDIA and AMD)

## Building HorizonOS 🛠️

### Prerequisites 📦
- i386-elf cross-compiler
- NASM assembler
- GRUB 2.0+
- QEMU (for emulation)

### Install build dependencies (Debian/Ubuntu) 🐧
```bash
sudo apt install nasm grub-pc-bin qemu-system-i386
```

### Set up cross-compiler (using included script) ⚙️
```bash
sudo sh install-cross-compiler.sh
```

### Compilation 🔨
```bash
make all
```

### Running in QEMU 🚀
```bash
make run
```

The `horizonos.iso` file will be generated in the repository root.

This will:
1. Build the kernel ISO
2. Launch QEMU with:
   - 256MB RAM
   - CD-ROM boot
   - Serial output
   - VGA display
   - Logging to debug/ directory

## Contributing 🧑‍💻
Contributions are welcome ! Feel free to open issues and send pull requests.

## License 📜
Distributed under the MIT License. See `LICENSE` for more information.

---

**Note:** This is experimental software - use at your own risk. Not recommended for production environments or critical systems.
