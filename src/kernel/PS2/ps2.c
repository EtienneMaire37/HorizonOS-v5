#pragma once

bool ps2_wait_for_output()
{
    uint32_t _start_timer = global_timer + PS2_WAIT_TIME; 
    while(!ps2_output_ready() && _start_timer > global_timer); 
    return _start_timer <= global_timer; // 0 -> Finished in time ; 1 -> timeout
}

bool ps2_wait_for_input()
{
    uint32_t _start_timer = global_timer + PS2_WAIT_TIME;
    while(!ps2_input_ready() && _start_timer > global_timer);
    return _start_timer <= global_timer; // 0 -> Finished in time ; 1 -> timeout
}

void ps2_read_data()
{
    ps2_data_bytes_received = 0;
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

void ps2_detect_devices()
{
    ps2_device_1_connected = ps2_device_2_connected = false;
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
    LOG(DEBUG, "Sending byte 0x%x to PS/2 device 1", command);
    uint8_t return_value;
    uint8_t tries = 0;
    do
    {
        bool timeout = ps2_wait_for_output();
        outb(PS2_COMMAND_REGISTER, command);
        timeout = ps2_wait_for_input();
        return_value = inb(PS2_DATA);
        LOG(DEBUG, "Device sent a resend signal");
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