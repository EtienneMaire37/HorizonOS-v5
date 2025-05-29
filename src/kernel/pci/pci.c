#pragma once

uint32_t pci_configuration_address_space_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) 
{
    if ((device & 0b11111) != device) return 0xffffffff;
    if ((function & 0b111) != function) return 0xffffffff;
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
    pci_ids = initrd_find_file("./pci.ids");

    for (uint16_t i = 0; i < 256; i++)
    {
        for (uint8_t j = 0; j < 32; j++)
        {
            uint8_t header_type = (pci_configuration_address_space_read_dword(i, j, 0, 12) >> 16) & 0xff;
            uint8_t functions = (header_type & 0x80) ? 8 : 1;
            header_type &= 0x7f;
            for (uint8_t k = 0; k < functions; k++)
            {
                uint32_t reg_0 = pci_configuration_address_space_read_dword(i, j, k, 0);
                uint16_t vendor_id = reg_0 & 0xffff;
                uint16_t device_id = reg_0 >> 16;
                if (vendor_id != 0xffff)
                {
                    LOG(DEBUG, "PCI Device at 0x%x:0x%x:0x%x (Header type : %u) :", i, j, k, header_type);
                    LOG(DEBUG, "    Device ID: 0x%x | Vendor ID: 0x%x", device_id, vendor_id);
                    LOG(DEBUG, "    Vendor : \"");

                    printf("PCI Device at 0x%x:0x%x:0x%x (Header type : %u) :\n", i, j, k, header_type);
                    printf("    Device ID: 0x%x | Vendor ID: 0x%x\n", device_id, vendor_id);
                    printf("    Vendor : ");
                    tty_set_color(FG_LIGHTGREEN, BG_BLACK);
                    putchar('\"');

                    uint32_t line = 0;
                    int32_t line_offset = 0;
                    uint16_t current_vendor_id = 0x0000, current_line_vendor_id = 0x0000, current_line_device_id = 0x0000;
                    bool vendor_id_line = true, device_id_line = true, comment_line = false, found_vendor = false, found_device = false, printed_vendor = false;
                    for (uint32_t file_offset = 0; file_offset < pci_ids->size; file_offset++, line_offset++)
                    {
                        uint8_t byte = pci_ids->data[file_offset];
                        if (byte == '\n')
                        {
                            line++;
                            line_offset = -1;
                            current_line_vendor_id = current_line_device_id = 0;
                            comment_line = false;
                            vendor_id_line = device_id_line = true;
                            if (found_vendor && !printed_vendor)
                            {
                                putchar('\"');
                                tty_set_color(FG_WHITE, BG_BLACK);
                                printf("\n    Device : ");
                                tty_set_color(FG_LIGHTGREEN, BG_BLACK);
                                putchar('\"');
                                fputc('\"', stderr);
                                LOG(DEBUG, "    Device : \"");
                                printed_vendor = true;
                            }
                            if (found_device)
                            {
                                printf("\"\n");
                                fputc('\"', stderr);
                                tty_set_color(FG_WHITE, BG_BLACK);
                                break;
                            }
                            continue;
                        }

                        if (comment_line)
                            continue;

                        if (byte == '#')
                        {
                            comment_line = true;
                            continue;
                        }

                        if (!vendor_id_line && line_offset < 1)
                            continue;

                        if (!device_id_line && line_offset < 2)
                            continue;

                        if (line_offset < 4 && vendor_id_line)
                        {
                            if (byte == '\t')
                            {
                                vendor_id_line = false;
                                continue;
                            }
                            else
                                current_line_vendor_id |= ((uint32_t)hex_char_to_int(byte)) << (4 * (3 - line_offset));
                        }

                        if (line_offset >= 1 && line_offset < 5 && !vendor_id_line)
                        {
                            if (byte == '\t')
                            {
                                device_id_line = false;
                                continue;
                            }
                            else
                                current_line_device_id |= ((uint32_t)hex_char_to_int(byte)) << (4 * (4 - line_offset));
                        }

                        if (line_offset == 4 && vendor_id_line)
                            current_vendor_id = current_line_vendor_id;

                        if (vendor_id_line && current_vendor_id == vendor_id && line_offset >= 6)
                        {
                            putchar(byte);
                            fputc(byte, stderr);
                            found_vendor = true;
                        }

                        if (device_id_line && current_line_device_id == device_id && current_vendor_id == vendor_id && line_offset >= 7)
                        {
                            putchar(byte);
                            fputc(byte, stderr);
                            found_device = true;
                        }
                    }
                    if (!found_device)
                    {
                        tty_set_color(FG_LIGHTGREEN, BG_BLACK);
                        putchar('\"');
                        fputc('\"', stderr);
                        putchar('\n');
                        tty_set_color(FG_WHITE, BG_BLACK);
                    }
                }
            }
        }
    }
}