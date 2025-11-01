# HorizonOS

<div align="center">
   
   ![GPL License](https://img.shields.io/badge/license-GPL-yellow.svg) 
   ![x86-64](https://img.shields.io/badge/arch-x86_64-informational) 
   ![GitHub Contributors](https://img.shields.io/github/contributors/EtienneMaire37/HorizonOS-v5?color=blue)
   ![Monthly Commits](https://img.shields.io/github/commit-activity/m/EtienneMaire37/HorizonOS-v5?color=orange)
   ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/EtienneMaire37/HorizonOS-v5/.github%2Fworkflows%2Fmakefile.yml)
</div>

HorizonOS is a hobby 64-bit monolithic kernel for the x86-64 architecture. It aims at simplicity and readability.

## Building HorizonOS

These instructions assume a Debian-like environment. Feel free to adapt those instructions to other platforms.

### Prerequisites

Install dependencies:
```bash
sudo apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
sudo apt install -y nasm xorriso mtools mkbootimg util-linux dosfstools mtools
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
A `horizonos.iso` disk image file will be created in the root of the repository.

### Running HorizonOS

To run HorizonOS in QEMU:
```bash
make run
```

## Memory map (when multitasking)

| Range     | Mapping         |
| --------- | --------------- |
| 0-1TB     | Identity mapped |
| 1TB-128TB | Process segments, heap and stack    |
| 128TB-[-128TB] | Noncanonical addresses |
| [-133MB]-[-132MB] | Process kernel stack |
| [-132MB]-[-128MB] | Process stack |
| [-128MB]-0 | mmio, framebuffer, bootboot data and kernel code segment |


## Contributing

You can submit issues [here](https://github.com/EtienneMaire37/HorizonOS-v5/issues).
Feel free to contribute and submit pull requests !

## License

HorizonOS is licensed under the GNU GPLv3 License. See the `LICENSE` file for more details.
