# HorizonOS

<div align="center">
   
   ![GPL License](https://img.shields.io/badge/license-GPL-yellow.svg) 
   ![x86](https://img.shields.io/badge/arch-x86-informational) 
   ![GitHub Contributors](https://img.shields.io/github/contributors/EtienneMaire37/HorizonOS-v5?color=blue)
   ![Monthly Commits](https://img.shields.io/github/commit-activity/m/EtienneMaire37/HorizonOS-v5?color=orange)
   ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/EtienneMaire37/HorizonOS-v5/.github%2Fworkflows%2Fmakefile.yml)
</div>

HorizonOS is a hobby 32-bit monolithic kernel for the x86 architecture. It aims at simplicity and readability.

## Features

HorizonOS currently supports the following features:

* **Features:**
    * Single-core multitasking with basic round robin scheduling
    * Memory management limited to 4GB
    * Simple (non standard compliant) C library implementation
    * Extremely basic ACPI parsing 
    * GRUB bootloader integration
    * PS/2 keyboard driver (8042)
    * Legacy (8259) PIC support
* **Planned:**
    * More than 4GB memory with PAE
    * Full APIC and ACPI support 
    * Multicore support with SMP
    * ATA and USB drivers
    * Network stack support and Ethernet drivers

## Building HorizonOS

These instructions assume a Debian-like environment. Feel free to adapt those instructions to other platforms.

### Prerequisites

Install dependencies:
```bash
sudo apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
sudo apt install nasm xorriso mtools 
```

### Building

Simply run: 
```bash
make all
```
Or:
```bash
make all CLOGLEVEL=-DLOGLEVEL=TRACE
```
To build with E9 port logs.
A `horizonos.img` disk image file will be created in the root of the repository.

### Running HorizonOS

To run HorizonOS in QEMU:
```bash
make run
```

## Contributing

You can submit issues [here](https://github.com/EtienneMaire37/HorizonOS-v5/issues).
Feel free to contribute and submit pull requests !

## License

HorizonOS is licensed under the GNU GPLv3 License. See the `LICENSE` file for more details.
