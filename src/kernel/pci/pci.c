#pragma once

uint32_t pci_configuration_address_space_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) 
{
    if ((device & 0b11111) != device) return 0xffffffff;
    if ((function & 0b11) != function) return 0xffffffff;
    if (offset & 0b11) return 0xffffffff;

    outd(PCI_CONFIG_ADDRESS,    (1 << 31) | // ~ Enable bit
                                ((uint32_t)bus << 16) |
                                ((uint32_t)device << 11) |
                                ((uint32_t)function << 8) | 
                                offset);

    return ind(PCI_CONFIG_DATA);
}

uint8_t pci_configuration_address_space_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    static uint8_t _bus = 0xff, _device = 0xff, _function = 0xff, _offset = 0xff;
    static uint32_t _dword = 0xffffffff;
    if (!(_bus == bus && _device == device && _function == function && _offset == offset))
    {
        _dword = pci_configuration_address_space_read_dword(bus, device, function, offset & 0b11111100);
        _bus = bus; _device = device; _function = function; _offset = offset;
    }
    return (_dword >> (8 * (_offset & 0b11))) & 0xff; // ^ "PCI devices are inherently little-endian"
}

void pci_scan_buses()
{
    for (uint16_t i = 0; i < 256; i++)
    {
        for (uint8_t j = 0; j < 32; j++)
        {
            uint8_t header_type = (pci_configuration_address_space_read_dword(i, j, 0, 12) >> 16) & 0xff;
            uint16_t functions = (header_type & 0x80) ? 1 : 256;
            header_type &= 0x7f;
            for (uint16_t k = 0; k < functions; k++)
            {
                uint32_t reg_0 = pci_configuration_address_space_read_dword(i, j, k, 0);
                uint16_t vendor_id = reg_0 & 0xffff;
                uint16_t device_id = reg_0 >> 16;
                if (vendor_id != 0xffff)
                {
                    LOG(DEBUG, "PCI Device at 0x%x:0x%x:0x%x (Header type : %u) :", i, j, k, header_type);
                    LOG(DEBUG, "    Device ID: 0x%x | Vendor ID: 0x%x", device_id, vendor_id);

                    printf("PCI Device at 0x%x:0x%x:0x%x (Header type : %u) :\n", i, j, k, header_type);
                    printf("    Device ID: 0x%x | Vendor ID: 0x%x\n", device_id, vendor_id);
                }
            }
        }
    }
}