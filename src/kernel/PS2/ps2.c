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
        // uint8_t val = inb(PS2_DATA);
        // LOG(INFO, "PS/2 DATABUFFER at %u = 0x%x", ps2_data_bytes_received, val);
        ps2_data_buffer[ps2_data_bytes_received++] = inb(PS2_DATA); // val;
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
    ps2_device_1_connected = ps2_device_2_connected = false;

    // TODO: Init USB controllers

    ps2_controller_connected = true; //false; //(fadt->boot_architecture_flags & 0b10) || acpi_10;   // For ACPI 1.0 the baf field is reserved so we act as if there is a controller

    if (ps2_controller_connected)
    {
        LOG(INFO, "PS/2 controller detected");
    }
    else
    {
        LOG(WARNING, "No PS/2 controller detected");
        return;
    }

    // Disable devices
    ps2_send_command_no_data_no_answer(PS2_DISABLE_DEVICE_1);
    ps2_send_command_no_data_no_answer(PS2_DISABLE_DEVICE_2);

    uint8_t trash = inb(PS2_DATA);  // Flush input buffer
    if (ps2_input_ready())
    {
        // Just so the compiler does'nt "optimize" (remove) the code
        LOG(ERROR, "PS/2 output buffer is full even after being read (0x%x)", trash);
        ps2_controller_connected = false;
        return;
    }

    uint8_t ccb = ps2_send_command_no_data(PS2_GET_CONFIGURATION);

    ccb &= ~0b01010001; // Init first port
    // ccb |=  0b00000100;

    ps2_send_command_with_data_no_answer(PS2_SET_CONFIGURATION, ccb);

    uint8_t return_val = ps2_send_command_no_data(PS2_TEST_CONTROLLER);

    if (return_val != 0x55)
    {
        LOG(ERROR, "PS/2 controller self test failed");
        ps2_controller_connected = false;
        return;
    }

    ps2_send_command_with_data_no_answer(PS2_SET_CONFIGURATION, ccb);

    ps2_send_command_no_data(PS2_ENABLE_DEVICE_2);

    uint8_t ccb2 = ps2_send_command_no_data(PS2_GET_CONFIGURATION);

    ps2_device_1_connected = true;
    ps2_device_2_connected = !(ccb2 & 0b00100000);

    if (ps2_device_2_connected)
    {
        ps2_send_command_no_data_no_answer(PS2_DISABLE_DEVICE_2);
        ccb2 &= ~0b00100010;
        ps2_send_command_with_data_no_answer(PS2_SET_CONFIGURATION, ccb);
    }

    return_val = ps2_send_command_no_data(PS2_TEST_DEVICE_1);
    LOG(INFO, "PS/2 device 1 returned 0x%x after self test", return_val);
    if (return_val != 0)
        ps2_device_1_connected = false;

    if (ps2_device_2_connected)
    {
        return_val = ps2_send_command_no_data(PS2_TEST_DEVICE_2);
        LOG(INFO, "PS/2 device 2 returned 0x%x after self test", return_val);
        if (return_val != 0)
            ps2_device_1_connected = false;
    }

    ps2_send_command_no_data_no_answer(PS2_ENABLE_DEVICE_1);
    // Dont enable device 2 for now
    // ps2_send_command_no_data_no_answer(PS2_ENABLE_DEVICE_2);
}

void ps2_detect_devices()
{
    // if (!ps2_controller_connected)
    //     return;
    // uint8_t return_val = ps2_send_command_no_data(PS2_DISABLE_SCANNING);
    // ps2_read_data();
    // if (return_val != PS2_ACK)
    // {
    //     ps2_device_1_connected = false;
    //     return;
    // }
    // ps2_send_command_no_data_no_answer(PS2_IDENTIFY);
    // ps2_read_data();
    // if (ps2_data_bytes_received != 1 || ps2_data_buffer[0] != PS2_ACK)
    // {
    //     ps2_device_1_connected = false;
    //     LOG(DEBUG, "PS/2 device 1 returned %u bytes : 0x%x 0x%x", ps2_data_bytes_received, ps2_data_buffer[0], ps2_data_buffer[1]);
    //     return;
    // }
    // return_val = ps2_send_command_no_data(PS2_ENABLE_SCANNING);
    // if (return_val != PS2_ACK)
    // {
    //     ps2_device_1_connected = false;
    //     return;
    // }
}

void ps2_send_command_no_data_no_answer(uint8_t command)
{
    if (!ps2_controller_connected)
        return;
    LOG(DEBUG, "Sending byte 0x%x to PS/2 controller", command);
    ps2_wait_for_output();
    outb(PS2_COMMAND_REGISTER, command);
}

uint8_t ps2_send_command_no_data(uint8_t command)
{
    if (!ps2_controller_connected)
        return 0;
    LOG(DEBUG, "Sending byte 0x%x to PS/2 controller", command);
    uint8_t return_value;
    uint8_t tries = 0;
    do
    {
        if (tries >= 1)
            LOG(DEBUG, "Controller sent a resend signal");
        ps2_wait_for_output();
        outb(PS2_COMMAND_REGISTER, command);
        ps2_wait_for_input();
        return_value = inb(PS2_DATA);
        tries++;
    } while (return_value == PS2_RESEND && tries < PS2_MAX_RESEND);
    if (tries >= PS2_MAX_RESEND)
    {
        LOG(DEBUG, "Controller did'nt respond correctly in %u tries", PS2_MAX_RESEND);
        return 0;
    }
    LOG(DEBUG, "Controller returned 0x%x", return_value);
    return return_value;
}

void ps2_send_command_with_data_no_answer(uint8_t command, uint8_t data)
{
    if (!ps2_controller_connected)
        return;
    LOG(DEBUG, "Sending byte 0x%x to PS/2 controller", command);
    ps2_wait_for_output();
    outb(PS2_COMMAND_REGISTER, command);
    ps2_wait_for_output();
    outb(PS2_DATA, data);
}

uint8_t ps2_send_command_with_data(uint8_t command, uint8_t data)
{
    if (!ps2_controller_connected)
        return 0;
    LOG(DEBUG, "Sending byte 0x%x to PS/2 controller", command);
    uint8_t return_value;
    uint8_t tries = 0;
    do
    {
        if (tries >= 1)
            LOG(DEBUG, "Controller sent a resend signal");
        ps2_wait_for_output();
        outb(PS2_COMMAND_REGISTER, command);
        ps2_wait_for_output();
        outb(PS2_DATA, data);
        ps2_wait_for_input();
        return_value = inb(PS2_DATA);
        tries++;
    } while (return_value == PS2_RESEND && tries < PS2_MAX_RESEND);
    if (tries >= PS2_MAX_RESEND)
    {
        LOG(DEBUG, "Controller did'nt respond correctly in %u tries", PS2_MAX_RESEND);
        return 0;
    }
    LOG(DEBUG, "Controller returned 0x%x", return_value);
    return return_value;
}