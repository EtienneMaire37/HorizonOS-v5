#pragma once

bool ps2_wait_for_output()
{
    if (!ps2_controller_connected)
        return 1;
    uint32_t _start_timer = global_timer + PS2_WAIT_TIME; 
    while(!ps2_output_ready() && _start_timer > global_timer); 
    return _start_timer <= global_timer; // 0 -> Finished in time ; 1 -> timeout
}

bool ps2_wait_for_input()
{
    if (!ps2_controller_connected)
        return 1;
    uint32_t _start_timer = global_timer + PS2_WAIT_TIME;
    while(!ps2_input_ready() && _start_timer > global_timer);
    return _start_timer <= global_timer; // 0 -> Finished in time ; 1 -> timeout
}

void ps2_read_data()
{
    ps2_data_bytes_received = 0;
    if (!ps2_controller_connected)
        return;
    while (ps2_wait_for_input() == 0)
    {
        ps2_data_buffer[ps2_data_bytes_received++] = inb(PS2_DATA);
        if (ps2_data_bytes_received == PS2_READ_BUFFER_SIZE)
            return;
        if (ps2_data_bytes_received > PS2_READ_BUFFER_SIZE)
        {
            LOG(CRITICAL, "PS/2 read buffer overrun");
            kabort();
        }
    }
}

void ps2_controller_init()
{
    // TODO: Init USB controllers

    ps2_controller_connected = false; //(fadt->boot_architecture_flags & 0b10) || acpi_10;   // For ACPI 1.0 the baf field is reserved so we act as if there is a controller

    if (ps2_controller_connected)
    {
        LOG(INFO, "PS/2 controller detected");
    }
    else
    {
        LOG(INFO, "No PS/2 controller detected");
    }

}

void ps2_detect_devices()
{
    ps2_device_1_connected = ps2_device_2_connected = false;
    if (!ps2_controller_connected)
        return;
    bool val = ps2_send_command_no_data(PS2_DISABLE_SCANNING);
    if (val != PS2_ACK)
    {
        ps2_device_1_connected = false;
        return;
    }
    val = ps2_send_command_no_data(PS2_IDENTIFY);
    if (val != PS2_ACK)
    {
        ps2_device_1_connected = false;
        return;
    }
    ps2_read_data();
    ps2_send_command_no_data(PS2_ENABLE_SCANNING);
}

uint8_t ps2_send_command_no_data(uint8_t command)
{
    if (!ps2_controller_connected)
        return 0;
    LOG(DEBUG, "Sending byte 0x%x to PS/2 device 1", command);
    uint8_t return_value;
    uint8_t tries = 0;
    do
    {
        if (tries >= 1)
            LOG(DEBUG, "Device sent a resend signal");
        ps2_wait_for_output();
        outb(PS2_COMMAND_REGISTER, command);
        ps2_wait_for_input();
        return_value = inb(PS2_DATA);
        tries++;
    } while (return_value == PS2_RESEND && tries < PS2_MAX_RESEND);
    if (tries >= PS2_MAX_RESEND)
    {
        LOG(DEBUG, "Device did'nt respond correctly in %u tries", PS2_MAX_RESEND);
        return 0;
    }
    LOG(DEBUG, "Device returned 0x%x", return_value);
    return return_value;
}