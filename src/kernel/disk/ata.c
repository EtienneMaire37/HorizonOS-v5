#pragma once

void pci_connect_ide_controller(uint8_t bus, uint8_t device, uint8_t function)
{
    if (connected_pci_ide_controllers >= IDE_MAX)
        return;

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

    // * Disable IRQs
    ata_write_control_block_register(&pci_ide_controller[connected_pci_ide_controllers], ATA_PRIMARY_CHANNEL, ATA_REG_CONTROL, 0b10); // ! Default value is 0
    ata_write_control_block_register(&pci_ide_controller[connected_pci_ide_controllers], ATA_SECONDARY_CHANNEL, ATA_REG_CONTROL, 0b10);

    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t j = 0; j < 2; j++) 
        {
            pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].connected = false;

            // * Select Drive
            ata_write_command_block_register(&pci_ide_controller[connected_pci_ide_controllers], i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); 
            ksleep(1);

            ata_write_command_block_register(&pci_ide_controller[connected_pci_ide_controllers], i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            ksleep(1); 

            if (ata_read_command_block_register(&pci_ide_controller[connected_pci_ide_controllers], i, ATA_REG_STATUS) == 0) 
                continue;

            uint8_t status;
            while (true) 
            {
                status = ata_read_command_block_register(&pci_ide_controller[connected_pci_ide_controllers], i, ATA_REG_STATUS);
                if (status & ATA_SR_ERR)
                    break;
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) 
                    break;
            }

            if (status & ATA_SR_ERR)
            {
                // * Don't support ATAPI drives for now
                continue;
            }

            uint8_t ide_buf[512];
            for (uint16_t k = 0; k < 256; k++)
            {
                ((uint16_t*)ide_buf)[k] = inw(pci_ide_controller[connected_pci_ide_controllers].channels[i].base_address + ATA_REG_DATA);
                // LOG(TRACE, "IDE IDENTIFY data word %u : 0x%x", k, ((uint16_t*)ide_buf)[k]);
            }

            pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].connected = 1;
            pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].signature = *((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE));
            pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].capabilities = *((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES));
            pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].command_sets = *((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS));

            if (pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].command_sets & (1 << 26))    
                // * 48-Bit LBA
                pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].size = (*((uint64_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT))) & 0xffffffffffff;
            else                 
                // * 28-Bit LBA or CHS
                pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].size = (*((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA))) & 0x0fffffff;

            uint8_t last_char_index = 0;
            for(uint8_t k = 0; k < 40; k += 2) 
            {
                pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                if (ide_buf[ATA_IDENT_MODEL + k] != ' ')
                    last_char_index = k;
                pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
                if (ide_buf[ATA_IDENT_MODEL + k + 1] != ' ')
                    last_char_index = k + 1;
            }
            if (last_char_index <= 40)
                pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].model[last_char_index] = 0;
            else
                pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].model[40] = 0;
        }
    }

    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t j = 0; j < 2; j++)
        {
            if (pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].connected) 
            {
                uint64_t bytes = (uint64_t)pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].size * 512;
                uint8_t magnitude = 0;
                uint64_t magnitude_value = bytes;
                const char* magnitude_text[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
                while (magnitude_value >= 1024 * 1024)
                {
                    magnitude_value /= 1024;
                    magnitude++;
                }
                if (magnitude_value >= 1024)
                    magnitude++;
                LOG(INFO, "Found drive \"%s\" (%lu bytes) [%lu.%lu%lu %s]", 
                    pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].model, 
                   bytes, magnitude_value / 1024, (magnitude_value * 10 / 1024) % 10, (magnitude_value * 100 / 1024) % 10, magnitude_text[magnitude]);
                printf("Found drive ");
                tty_set_color(FG_LIGHTGREEN, BG_BLACK);
                printf("\"%s\" ", pci_ide_controller[connected_pci_ide_controllers].channels[i].devices[j].model);
                tty_set_color(FG_WHITE, BG_BLACK);
                printf("(%lu bytes) [", bytes);
                tty_set_color(FG_LIGHTCYAN, BG_BLACK);
                printf("%lu.%lu%lu ", magnitude_value / 1024, (magnitude_value * 10 / 1024) % 10, (magnitude_value * 100 / 1024) % 10);
                tty_set_color(FG_WHITE, BG_BLACK);
                printf("%s]\n", magnitude_text[magnitude]);
            }
        }
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

void ata_write_command_block_register(pci_ide_controller_data_t* controller, uint8_t channel, uint8_t reg, uint8_t data)
{
    outb(controller->channels[channel].base_address + reg, data);
}
uint8_t ata_read_command_block_register(pci_ide_controller_data_t* controller, uint8_t channel, uint8_t reg)
{
    return inb(controller->channels[channel].base_address + reg);
}

void ata_write_control_block_register(pci_ide_controller_data_t* controller, uint8_t channel, uint8_t reg, uint8_t data)
{
    outb(controller->channels[channel].ctrl_base_address + reg, data);
}
uint8_t ata_read_control_block_register(pci_ide_controller_data_t* controller, uint8_t channel, uint8_t reg)
{
    return inb(controller->channels[channel].ctrl_base_address + reg);
}