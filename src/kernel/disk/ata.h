#pragma once

#define IDE_MAX     8

typedef struct pci_ide_controller_data
{
    uint8_t bus, device, function;
} pci_ide_controller_data_t;

pci_ide_controller_data_t pci_ide_controller[IDE_MAX];
uint16_t connected_pci_ide_controllers = 0;

void pci_connect_ide_controller(uint8_t bus, uint8_t device, uint8_t function)
{
    pci_ide_controller[connected_pci_ide_controllers].bus = bus;
    pci_ide_controller[connected_pci_ide_controllers].device = device;
    pci_ide_controller[connected_pci_ide_controllers].function = function;
    
    connected_pci_ide_controllers++;
}