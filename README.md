# HorizonOS

<div align="center">
   
   ![MIT License](https://img.shields.io/badge/license-MIT-yellow.svg) 
   ![x86](https://img.shields.io/badge/arch-x86-informational) 
   ![GitHub Contributors](https://img.shields.io/github/contributors/EtienneMaire37/HorizonOS-v5?color=blue)
   ![Monthly Commits](https://img.shields.io/github/commit-activity/m/EtienneMaire37/HorizonOS-v5?color=orange)
   ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/EtienneMaire37/HorizonOS-v5/.github%2Fworkflows%2Fmakefile.yml)
</div>

HorizonOS is a hobby 32-bit monolithic kernel for the x86 architecture. This project aims to explore operating system concepts and provide a minimal yet functional environment.

## Features

HorizonOS currently supports the following features:

* **Kernel Features:**
    * Single-core multitasking with basic scheduling.
    * Memory management limited to the 4GB address space of 32-bit architecture.
    * Basic POSIX-like C library implementation.
    * Virtual File System (VFS) infrastructure.
    * Initial RAM disk (initrd) support.
    * Extremely basic ACPI parsing for system information.
    * GRUB bootloader support.
* **Device Drivers:**
    * VGA text mode driver.
    * PS/2 keyboard driver with QWERTY and AZERTY layout support.

## Building HorizonOS

These instructions assume you have a Unix-like environment with basic development tools.

### Prerequisites

* **GNU Make:** For automating the build process.
* **NASM:** Netwide Assembler for assembling assembly files.
* **i486-elf Toolchain:** A cross-compiler toolchain targeting the i486-elf architecture is required to build the kernel and user-mode programs.
* **QEMU:** A machine emulator for testing the operating system.
* **GRUB Utilities:** Specifically `grub-mkrescue` to create the bootable ISO image.

### Setting up the i486-elf Cross-Compiler

The provided `install-cross-compiler.sh` script automates the process of building and installing the necessary cross-compiler toolchain. To use it:

Run the script:
```bash
./install-cross-compiler.sh
```
This script will download, build, and install Binutils and GCC targeting the `i486-elf` architecture in a directory named `i486elfgcc` within the project. It will also update your `PATH` environment variable for the current session.

### Building the Operating System

Once the cross-compiler is set up, you can build HorizonOS using the `Makefile`:

1.  Ensure you are in the root directory of the HorizonOS repository.
2.  Execute the `make` command:
    ```bash
    make all
    ```
    This command will compile the kernel, the basic user-mode task, build the C library, create the initial RAM disk, and finally generate the `horizonos.iso` bootable ISO image in the root directory.

### Running HorizonOS

To run HorizonOS in QEMU:

1.  From the root directory of the HorizonOS repository, execute the `make run` command:
    ```bash
    make run
    ```
    This will launch QEMU, booting from the generated `horizonos.iso` image. You should see the kernel initialize and the basic user-mode task start. Debug output will be written to a file named `debug/YYYY-MM-DD.log`.

## Current Development

The following features are planned:

* ATA/SATA drivers for hard disk access.
* FAT32 filesystem support.
* PS/2 mouse driver.
* USB HID (Human Interface Device) support.
* Physical Address Extension (PAE) support to potentially access more than 4GB of RAM.
* Symmetric Multiprocessing (SMP) for multi-core support.
* Porting to other CPU architectures.
* Ethernet and Wi-Fi drivers along with a basic internet protocol stack.

## License

HorizonOS is licensed under the GNU GPLv3 License. See the `LICENSE` file for more details.
