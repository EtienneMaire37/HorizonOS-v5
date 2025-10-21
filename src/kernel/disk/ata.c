#pragma once

void pci_connect_ide_controller(uint8_t bus, uint8_t device, uint8_t function)
{
    pci_ide_controller[connected_pci_ide_controllers].bus = bus;
    pci_ide_controller[connected_pci_ide_controllers].device = device;
    pci_ide_controller[connected_pci_ide_controllers].function = function;

    pci_ide_controller[connected_pci_ide_controllers].prog_if = (pci_configuration_address_space_read_byte(bus, device, function, 8) >> 8) & 0xff;

    pci_ide_controller[connected_pci_ide_controllers].bar0 = pci_configuration_address_space_read_dword(bus, device, function, 16);
    pci_ide_controller[connected_pci_ide_controllers].bar1 = pci_configuration_address_space_read_dword(bus, device, function, 20);
    pci_ide_controller[connected_pci_ide_controllers].bar2 = pci_configuration_address_space_read_dword(bus, device, function, 24);
    pci_ide_controller[connected_pci_ide_controllers].bar3 = pci_configuration_address_space_read_dword(bus, device, function, 28); 
    pci_ide_controller[connected_pci_ide_controllers].bar4 = pci_configuration_address_space_read_dword(bus, device, function, 32);

    pci_ide_controller[connected_pci_ide_controllers].channels[0].compatibility_mode = (pci_ide_controller[connected_pci_ide_controllers].prog_if & 0b1) == 0;
    pci_ide_controller[connected_pci_ide_controllers].channels[1].compatibility_mode = (pci_ide_controller[connected_pci_ide_controllers].prog_if & 0b100) == 0;
    
    if (pci_ide_controller[connected_pci_ide_controllers].channels[0].compatibility_mode)
    {
        pci_ide_controller[connected_pci_ide_controllers].channels[0].base_address = 0x1F0;
        pci_ide_controller[connected_pci_ide_controllers].channels[0].ctrl_base_address = 0x3F6;
        pci_ide_controller[connected_pci_ide_controllers].channels[0].irq = 14;
    }
    else
    {
        pci_ide_controller[connected_pci_ide_controllers].channels[0].base_address = pci_ide_controller[connected_pci_ide_controllers].bar0 & 0xfffffffc;
        pci_ide_controller[connected_pci_ide_controllers].channels[0].ctrl_base_address = pci_ide_controller[connected_pci_ide_controllers].bar1 & 0xfffffffc;
        pci_ide_controller[connected_pci_ide_controllers].channels[0].irq = pci_configuration_address_space_read_dword(bus, device, function, 0x3C) & 0xff;
    }

    if (pci_ide_controller[connected_pci_ide_controllers].channels[1].compatibility_mode)
    {
        pci_ide_controller[connected_pci_ide_controllers].channels[1].base_address = 0x170;
        pci_ide_controller[connected_pci_ide_controllers].channels[1].ctrl_base_address = 0x376;
        pci_ide_controller[connected_pci_ide_controllers].channels[1].irq = 15;
    }
    else
    {
        pci_ide_controller[connected_pci_ide_controllers].channels[1].base_address = pci_ide_controller[connected_pci_ide_controllers].bar2 & 0xfffffffc;
        pci_ide_controller[connected_pci_ide_controllers].channels[1].ctrl_base_address = pci_ide_controller[connected_pci_ide_controllers].bar3 & 0xfffffffc;
        pci_ide_controller[connected_pci_ide_controllers].channels[1].irq = pci_configuration_address_space_read_dword(bus, device, function, 0x3C) & 0xff;
    }

    connected_pci_ide_controllers++;

    LOG(DEBUG, "Connected PCI IDE controller at %u:%u:%u", bus, device, function);

    LOG(DEBUG, "    %s mode on primary channel", pci_ide_controller[connected_pci_ide_controllers - 1].channels[0].compatibility_mode ? "Compatibility" : "Native");
    LOG(DEBUG, "    %s mode on secondary channel", pci_ide_controller[connected_pci_ide_controllers - 1].channels[1].compatibility_mode ? "Compatibility" : "Native");

    LOG(DEBUG, "    Primary channel base address : 0x%x | Control base address : 0x%x | IRQ : %u",
        pci_ide_controller[connected_pci_ide_controllers - 1].channels[0].base_address,
        pci_ide_controller[connected_pci_ide_controllers - 1].channels[0].ctrl_base_address,
        pci_ide_controller[connected_pci_ide_controllers - 1].channels[0].irq);
    LOG(DEBUG, "    Secondary channel base address : 0x%x | Control base address : 0x%x | IRQ : %u",
        pci_ide_controller[connected_pci_ide_controllers - 1].channels[1].base_address,
        pci_ide_controller[connected_pci_ide_controllers - 1].channels[1].ctrl_base_address,
        pci_ide_controller[connected_pci_ide_controllers - 1].channels[1].irq);
}