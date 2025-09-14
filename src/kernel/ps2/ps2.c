#pragma once

bool ps2_wait_for_output() 
{
    if (!ps2_controller_connected)
        return true;
    uint64_t start = precise_time_to_milliseconds(global_timer);
    while (precise_time_to_milliseconds(global_timer) - start < PS2_WAIT_TIME)
    { 
        uint8_t reg = inb(PS2_STATUS_REGISTER);
        // LOG(DEBUG, "Status reg : 0x%x", reg); 
        if ((reg & PS2_STATUS_INPUT_FULL) == 0)
            return false;
    }
    LOG(WARNING, "PS/2 wait to output timeout");
    return true;
}

bool ps2_wait_for_input()
{
    if (!ps2_controller_connected)
        return true;
    uint64_t start = precise_time_to_milliseconds(global_timer);
    while (precise_time_to_milliseconds(global_timer) - start < PS2_WAIT_TIME)
    { 
        uint8_t reg = inb(PS2_STATUS_REGISTER);
        // LOG(DEBUG, "Status reg : 0x%x", reg); 
        if ((reg & PS2_STATUS_OUTPUT_FULL) == PS2_STATUS_OUTPUT_FULL)
            return false;
    }
    // LOG(TRACE, "PS/2 wait to input timeout");
    return true;
}

void ps2_flush_buffer() 
{
    if (!ps2_controller_connected)
        return;
    while (inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL)
        inb(PS2_DATA);
}

uint8_t ps2_send_command(uint8_t command) 
{
    if (!ps2_controller_connected)
        return 0xff;

    // LOG(TRACE, "Sending command 0x%x to PS/2 controller", command);

    uint8_t tries = 0;
    uint8_t return_val;
    do 
    {
        if (tries > 0)
            LOG(WARNING, "PS/2 controller sent resend signal");
        if (ps2_wait_for_output())
            return 0xff;
        outb(PS2_COMMAND_REGISTER, command);
        if (ps2_wait_for_input()) 
            return 0xff;
        return_val = inb(PS2_DATA);
    }
    while (return_val == PS2_RESEND && tries < PS2_MAX_RESEND);
    return return_val;
}

void ps2_send_command_no_response(uint8_t command)
{
    if (!ps2_controller_connected)
        return;
    // LOG(TRACE, "Sending command 0x%x to PS/2 controller", command);
    if (ps2_wait_for_output())
        return;
    outb(PS2_COMMAND_REGISTER, command);
}

uint8_t ps2_send_command_with_data(uint8_t command, uint8_t data) 
{
    if (!ps2_controller_connected)
        return 0xff;

    // LOG(TRACE, "Sending command 0x%x, 0x%x to PS/2 controller", command, data);

    uint8_t tries = 0;
    uint8_t return_val;
    do 
    {
        if (tries > 0)
            LOG(WARNING, "PS/2 controller sent resend signal");
        if (ps2_wait_for_output()) 
            return 0xff;
        outb(PS2_COMMAND_REGISTER, command);
        if (ps2_wait_for_output()) 
            return 0xff;
        outb(PS2_DATA, data);
        if (ps2_wait_for_input()) 
            return 0xff;
        return_val = inb(PS2_DATA);
    }
    while (return_val == PS2_RESEND && tries < PS2_MAX_RESEND);
    return return_val;
}

void ps2_send_command_with_data_no_response(uint8_t command, uint8_t data) 
{
    if (!ps2_controller_connected)
        return;

    // LOG(TRACE, "Sending command 0x%x, 0x%x to PS/2 controller", command, data);

    if (ps2_wait_for_output()) 
        return;
    outb(PS2_COMMAND_REGISTER, command);
    if (ps2_wait_for_output()) 
        return;
    outb(PS2_DATA, data);
}

bool ps2_send_device_command(uint8_t device, uint8_t command) 
{
    if (!ps2_controller_connected)
        return false;

    // LOG(TRACE, "Sending command 0x%x to PS/2 device %u", command, device);

    for (int tries = 0; tries < PS2_MAX_RESEND; tries++) 
    {
        if (device == 2) 
            if(ps2_send_command_with_data(PS2_WRITE_DEVICE_2, command) != PS2_ACK)
                continue;

        if (ps2_wait_for_output()) 
        {
            LOG(DEBUG, "Timeout waiting to send to device %d", device);
            continue;
        }

        outb(PS2_DATA, command);

        // ksleep(100 * PRECISE_MILLISECONDS);

        return true;
    }
    return false;
}

bool ps2_send_device_full_command(uint8_t device, uint8_t command, uint8_t extected_bytes)
{
    uint8_t tries = 0;
    do
    {
        tries++;
        if(!ps2_send_device_command(device, command))
            return false;
        ps2_read_data(extected_bytes);
    } while ((ps2_data_bytes_received == 0 || ps2_data_buffer[0] == PS2_RESEND) && tries < PS2_MAX_RESEND);
    if (tries >= PS2_MAX_RESEND)
        return false;
    return true;
}

bool ps2_send_device_full_command_with_data(uint8_t device, uint8_t command, uint8_t data, uint8_t extected_bytes)
{
    bool sending_command = true;
    uint8_t tries = 0;
    do
    {
        tries++;
        if (sending_command)
        {
            if(!ps2_send_device_command(device, command))
                return false;
            // ps2_read_data(extected_bytes);
            ps2_read_data(1);
        }
        else
        {
            if(!ps2_send_device_command(device, data))
                return false;
            ps2_read_data(extected_bytes);
            return true;
        }
        if (!((ps2_data_bytes_received == 0 || ps2_data_buffer[0] == PS2_RESEND || !sending_command)))
        {
            sending_command = false;
            tries--;
        }
    } while (tries < PS2_MAX_RESEND); //((ps2_data_bytes_received == 0 || ps2_data_buffer[0] == PS2_RESEND || !sending_command) && tries < PS2_MAX_RESEND);
    // if (tries >= PS2_MAX_RESEND)
    //     return false;
    // return true;
    return false;
}

void ps2_read_data(uint8_t extected_bytes) 
{
    ps2_data_bytes_received = 0;
    if (!ps2_controller_connected || extected_bytes == 0)
        return;
    for (uint8_t i = 0; !ps2_wait_for_input() && ps2_data_bytes_received < PS2_READ_BUFFER_SIZE; i++)   // && (inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL)
    {
        ps2_data_buffer[ps2_data_bytes_received++] = inb(PS2_DATA);
        if (i >= extected_bytes - 1)
            return;
    }
}

void ps2_read_additional_data() 
{
    // ps2_data_bytes_received = 0;
    if (!ps2_controller_connected)
        return;
    while (!ps2_wait_for_input() && ps2_data_bytes_received < PS2_READ_BUFFER_SIZE)
        ps2_data_buffer[ps2_data_bytes_received++] = inb(PS2_DATA);
}

void ps2_controller_init() 
{
    LOG(DEBUG, "PS/2 controller initialization sequence");

    ps2_device_1_connected = false;
    ps2_device_2_connected = false;
    enable_ps2_kb_input = false;
    ps2_device_1_interrupt = ps2_device_2_interrupt = false;

    if (!ps2_controller_connected)
    {
        LOG(INFO, "No PS/2 controller connected");
        return;
    }

    LOG(DEBUG, "Disabling device 1");
    ps2_send_command_no_response(PS2_DISABLE_DEVICE_1);
    LOG(DEBUG, "Disabling device 2");
    ps2_send_command_no_response(PS2_DISABLE_DEVICE_2);
    
    ps2_flush_buffer();

    LOG(DEBUG, "Setting up the Controller Configuration Byte");
    // printf("Setting up the Controller Configuration Byte\n");

    uint8_t config = ps2_send_command(PS2_GET_CONFIGURATION);
    config &= ~0b11001011; // (disable IRQs and translation)
    ps2_send_command_with_data_no_response(PS2_SET_CONFIGURATION, config);

    LOG(DEBUG, "Testing the controller");
    // printf("Testing the controller\n");

    uint8_t self_test_code = ps2_send_command(PS2_TEST_CONTROLLER);

    if (self_test_code != PS2_SELF_TEST_OK) 
    {
        LOG(ERROR, "Controller self-test failed (code 0x%x)", self_test_code);
        ps2_controller_connected = false;
        return;
    }

    LOG(DEBUG, "Testing the ports");

    bool dual_channel = false;
    if (ps2_send_command(PS2_ENABLE_DEVICE_2) == PS2_DEVICE_TEST_PASS) 
    {
        uint8_t new_config = ps2_send_command(PS2_GET_CONFIGURATION);
        dual_channel = !(new_config & 0x20);
        ps2_send_command_no_response(PS2_DISABLE_DEVICE_2);
    }

    // ps2_flush_buffer();

    ps2_device_1_connected = (ps2_send_command(PS2_TEST_DEVICE_1) == PS2_DEVICE_TEST_PASS);
    ps2_device_2_connected = dual_channel && 
                           (ps2_send_command(PS2_TEST_DEVICE_2) == PS2_DEVICE_TEST_PASS);

    ps2_send_device_full_command(1, PS2_DISABLE_SCANNING, 1);
    ps2_send_device_full_command(2, PS2_DISABLE_SCANNING, 1);

    LOG(DEBUG, "Resetting the devices");

    if (ps2_device_1_connected) 
    {
        ps2_send_command_no_response(PS2_ENABLE_DEVICE_1);
        // if (ps2_send_device_command(1, PS2_RESET)) 
        if (ps2_send_device_full_command(1, PS2_RESET, 2))
        {
            if (ps2_data_bytes_received >= 2 && 
                ps2_data_buffer[1] == PS2_DEVICE_BAT_OK) 
            {
                LOG(INFO, "Device 1 basic assurance test passed");
            }
            else
                ps2_device_1_connected = false;
        }
        else
            ps2_device_1_connected = false;
    }

    if (ps2_device_2_connected) 
    {
        ps2_send_command_no_response(PS2_ENABLE_DEVICE_2);
        // if (ps2_send_device_command(1, PS2_RESET)) 
        if (ps2_send_device_full_command(2, PS2_RESET, 2))
        {
            if (ps2_data_bytes_received >= 2 && 
                ps2_data_buffer[1] == PS2_DEVICE_BAT_OK) 
            {
                LOG(INFO, "Device 2 basic assurance test passed");
            }
            else
                ps2_device_2_connected = false;
        }
        else
            ps2_device_2_connected = false;
    }

    LOG(DEBUG, "Setting up the ccb without irqs");

    config = ps2_send_command(PS2_GET_CONFIGURATION);
    config &= ~0b01000000;   // Disable translation
    // if (ps2_device_1_connected) config &= ~0b00000001; // Disable interrupts
    // if (ps2_device_2_connected) config &= ~0b00000010;
    ps2_send_command_with_data_no_response(PS2_SET_CONFIGURATION, config);

    LOG(INFO, "PS/2 Controller initialized. Devices: %u/%u", 
        ps2_device_1_connected, ps2_device_2_connected);
}

void ps2_detect_keyboards() 
{
    if (!ps2_controller_connected)
        return;

    LOG(INFO, "Detecting PS/2 keyboards");
    
    if (ps2_device_1_connected) 
    {
        if (ps2_send_device_full_command(1, PS2_DISABLE_SCANNING, 1))
        {
            if (!(ps2_data_bytes_received >= 1 && ps2_data_buffer[0] == PS2_ACK))
                goto invalid_port_1;
            if (ps2_send_device_full_command(1, PS2_IDENTIFY, 3))
            {
                if (!(ps2_data_bytes_received >= 1 && ps2_data_buffer[0] == PS2_ACK))
                    goto invalid_port_1;

                LOG(INFO, "PS/2 port 1 id : 0x%x 0x%x (%u bytes)", ps2_data_buffer[1], 
                    ps2_data_buffer[2], ps2_data_bytes_received);
                
                if (ps2_data_bytes_received >= 3 && ps2_data_buffer[1] == 0xab) // Any PS/2 keyboard
                {
                    ps2_device_1_type = PS2_DEVICE_KEYBOARD;
                    LOG(INFO, "Keyboard detected on port 1");
                }
            }
            ps2_send_device_full_command(1, PS2_ENABLE_SCANNING, 1);
        }
    }

detect_port_2:
    if (ps2_device_2_connected) 
    {
        if (ps2_send_device_full_command(2, PS2_DISABLE_SCANNING, 1))
        {
            if (!(ps2_data_bytes_received >= 1 && ps2_data_buffer[0] == PS2_ACK))
                goto invalid_port_2;
            if (ps2_send_device_full_command(2, PS2_IDENTIFY, 3))
            {
                if (!(ps2_data_bytes_received >= 1 && ps2_data_buffer[0] == PS2_ACK))
                    goto invalid_port_2;

                LOG(INFO, "PS/2 port 2 id : 0x%x 0x%x (%u bytes)", ps2_data_buffer[1], 
                    ps2_data_buffer[2], ps2_data_bytes_received);
                
                if (ps2_data_bytes_received >= 3 && ps2_data_buffer[1] == 0xab) // Any PS/2 keyboard
                {
                    ps2_device_1_type = PS2_DEVICE_KEYBOARD;
                    LOG(INFO, "Keyboard detected on port 2");
                }
            }
            ps2_send_device_full_command(2, PS2_ENABLE_SCANNING, 1);
        }
    }

    return;

invalid_port_1:
    ps2_device_1_connected = false;
    goto detect_port_2;
invalid_port_2:
    ps2_device_2_connected = false;
}

void ps2_enable_interrupts()
{
    LOG(INFO, "Enabling PS/2 IRQs");
    
    ps2_device_1_interrupt = ps2_device_1_connected;
    ps2_device_2_interrupt = ps2_device_2_connected;

    // config = ps2_send_command(PS2_GET_CONFIGURATION);
    // config &= ~0b01000000;   // Disable translation
    uint8_t config = 0b00000100;
    if (ps2_device_1_interrupt) config |= 0b00000001; // Enable interrupts
    if (ps2_device_2_interrupt) config |= 0b00000010;
    ps2_send_command_with_data_no_response(PS2_SET_CONFIGURATION, config);

    ksleep(10 * PRECISE_MILLISECONDS);

    enable_ps2_kb_input = true;
}

void handle_irq_1() 
{
    if (!ps2_controller_connected)
        return;
    
    if (!(inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL))
        return;

    uint8_t data = inb(PS2_DATA);

    // if (!ps2_device_1_interrupt)
    // {
    //     LOG(CRITICAL, "Kernel failed to poll PS/2 return value");
    //     abort();
    // }

    if (!ps2_device_1_connected) 
        return;
    
    if (ps2_device_1_type == PS2_DEVICE_KEYBOARD && enable_ps2_kb_input) 
        ps2_handle_keyboard_scancode(1, data);
}

void handle_irq_12() 
{
    if (!ps2_controller_connected)
        return;

    if (!(inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL))
        return;

    uint8_t data = inb(PS2_DATA);

    // if (!ps2_device_2_interrupt)
    // {
    //     LOG(CRITICAL, "Kernel failed to poll PS/2 return value");
    //     abort();
    // }

    if (!ps2_device_2_connected) 
        return;
    
    if (ps2_device_2_type == PS2_DEVICE_KEYBOARD && enable_ps2_kb_input) 
        ps2_handle_keyboard_scancode(2, data);
}