#pragma once

bool ps2_wait_for_output() 
{
    if (!ps2_controller_connected)
        return true;
    uint32_t start = global_timer;
    while (global_timer - start < PS2_WAIT_TIME)
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
    uint32_t start = global_timer;
    while (global_timer - start < PS2_WAIT_TIME)
    { 
        uint8_t reg = inb(PS2_STATUS_REGISTER);
        // LOG(DEBUG, "Status reg : 0x%x", reg); 
        if ((reg & PS2_STATUS_OUTPUT_FULL) == PS2_STATUS_OUTPUT_FULL)
            return false;
    }
    LOG(WARNING, "PS/2 wait to input timeout");
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

uint8_t ps2_send_command_with_data(uint8_t command, uint8_t data) 
{
    if (!ps2_controller_connected)
        return 0xff;

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

uint8_t ps2_send_device_command(uint8_t device, uint8_t command) 
{
    if (!ps2_controller_connected)
        return 0xff;

    uint8_t response;
    for (int tries = 0; tries < PS2_MAX_RESEND; tries++) 
    {
        if (device == 2) 
        {
            response = ps2_send_command_with_data(PS2_WRITE_DEVICE_2, command);
        } 
        else 
        {
            if (ps2_wait_for_output()) {
                LOG(DEBUG, "Timeout waiting to send to device %d", device);
                continue;
            }
            outb(PS2_DATA, command);
            
            // Give device time to process (minimum 1ms delay)
            uint32_t delay_start = global_timer;
            while (global_timer - delay_start < 2); // ~2ms
            
            if (ps2_wait_for_input()) {
                LOG(DEBUG, "Timeout waiting for response from device %d", device);
                continue;
            }
            response = inb(PS2_DATA);
        }

        switch (response) 
        {
            case PS2_ACK:
                return PS2_ACK;
            case PS2_RESEND:
                LOG(DEBUG, "Resend request from device %d", device);
                // Add 5ms delay before retry
                uint32_t delay_start = global_timer;
                while (global_timer - delay_start < 5);
                // for (uint16_t j = 0; j < 100; j++)
                //     io_wait();
                break;
            default:
                LOG(DEBUG, "Unexpected response 0x%x from device %d", response, device);
                return response;
        }
    }
    return 0xff;
}

void ps2_read_data() 
{
    ps2_data_bytes_received = 0;
    if (!ps2_controller_connected)
        return;
    while (!ps2_wait_for_input() && ps2_data_bytes_received < PS2_READ_BUFFER_SIZE) 
        ps2_data_buffer[ps2_data_bytes_received++] = inb(PS2_DATA);
}

void ps2_controller_init() 
{
    LOG(DEBUG, "PS/2 controller initialization sequence");

    ps2_controller_connected = true;   // TODO: FADT parsing
    ps2_device_1_connected = false;
    ps2_device_2_connected = false;

    if (!ps2_controller_connected)
        return;

    LOG(DEBUG, "Disabling devices");
    LOG(DEBUG, "Disabling device 1");
    ps2_send_command(PS2_DISABLE_DEVICE_1);
    LOG(DEBUG, "Disabling device 2");
    ps2_send_command(PS2_DISABLE_DEVICE_2);

    LOG(DEBUG, "Setting up the Controller Configuration Byte");

    uint8_t config = ps2_send_command(PS2_GET_CONFIGURATION);
    config &= 0b01000011; // (disable IRQs and translation)
    ps2_send_command_with_data(PS2_SET_CONFIGURATION, config);

    LOG(DEBUG, "Testing the controller");

    uint8_t self_test_code = ps2_send_command(PS2_TEST_CONTROLLER);

    if (self_test_code != 0x55) 
    {
        LOG(ERROR, "Controller self-test failed (code 0x%x)", self_test_code);
        ps2_controller_connected = false;
        return;
    }

    LOG(DEBUG, "Testing the ports");

    bool dual_channel = false;
    if (ps2_send_command(PS2_ENABLE_DEVICE_2) == 0x00) 
    {
        uint8_t new_config = ps2_send_command(PS2_GET_CONFIGURATION);
        dual_channel = !(new_config & 0x20);
        ps2_send_command(PS2_DISABLE_DEVICE_2);
    }

    ps2_device_1_connected = (ps2_send_command(PS2_TEST_DEVICE_1) == 0x00);
    ps2_device_2_connected = dual_channel && 
                           (ps2_send_command(PS2_TEST_DEVICE_2) == 0x00);

    ps2_send_device_command(1, PS2_DISABLE_SCANNING);
    ps2_send_device_command(2, PS2_DISABLE_SCANNING);

    LOG(DEBUG, "Resetting the devices");

    if (ps2_device_1_connected) 
    {
        ps2_send_command(PS2_ENABLE_DEVICE_1);
        if (ps2_send_device_command(1, PS2_RESET) == PS2_ACK) 
        {
            ps2_read_data();
            if (ps2_data_bytes_received >= 1 && 
                ps2_data_buffer[0] == PS2_DEVICE_BAT_OK) 
            {
                LOG(INFO, "Device 1 basic assurance test passed");
            }
        }
        else
            ps2_device_1_connected = false;
    }

    if (ps2_device_2_connected) 
    {
        ps2_send_command(PS2_ENABLE_DEVICE_2);
        if (ps2_send_device_command(2, PS2_RESET) == PS2_ACK) 
        {
            ps2_read_data();
            if (ps2_data_bytes_received >= 1 && 
                ps2_data_buffer[0] == PS2_DEVICE_BAT_OK) 
            {
                LOG(INFO, "Device 2 basic assurance test passed");
            }
        }
        else
            ps2_device_2_connected = false;
    }

    LOG(DEBUG, "Setting up the right ccb");

    config = ps2_send_command(PS2_GET_CONFIGURATION);
    if (ps2_device_1_connected) config |= 0b00000001; // Enable interrupts
    if (ps2_device_2_connected) config |= 0b00000010;
    config &= 0b01000000;   // Disable translation
    ps2_send_command_with_data(PS2_SET_CONFIGURATION, config);

    LOG(INFO, "PS/2 Controller initialized. Devices: %u/%u", 
        ps2_device_1_connected, ps2_device_2_connected);
}

void ps2_detect_devices() 
{
    if (!ps2_controller_connected)
        return;

    ps2_device_1_type = ps2_device_2_type = PS2_DEVICE_UNKNOWN;
    
    if (ps2_device_1_connected) 
    {
        if (ps2_send_device_command(1, PS2_DISABLE_SCANNING) == PS2_ACK) 
        {
            if (ps2_send_device_command(1, PS2_IDENTIFY) == PS2_ACK) 
            {
                ps2_read_data();
                LOG(INFO, "Device1 ID: 0x%x 0x%x", 
                    ps2_data_buffer[0], ps2_data_buffer[1]);
            }
            ps2_send_device_command(1, PS2_ENABLE_SCANNING);
        }
        else 
            ps2_device_1_connected = false;
    }
    
    if (ps2_device_2_connected) 
    {
        if (ps2_send_device_command(2, PS2_DISABLE_SCANNING) == PS2_ACK) 
        {
            if (ps2_send_device_command(2, PS2_IDENTIFY) == PS2_ACK) 
            {
                ps2_read_data();
                LOG(INFO, "Device2 ID: 0x%x 0x%x",
                    ps2_data_buffer[0], ps2_data_buffer[1]);
            }
            ps2_send_device_command(2, PS2_ENABLE_SCANNING);
        }
        else 
            ps2_device_2_connected = false;
    }
}

void ps2_detect_keyboards() 
{
    if (!ps2_controller_connected)
        return;
    
    if (ps2_device_1_connected) 
    {
        if (ps2_send_device_command(1, PS2_DISABLE_SCANNING) == PS2_ACK) 
        {
            if (ps2_send_device_command(1, PS2_IDENTIFY) == PS2_ACK) 
            {
                ps2_read_data();
                
                // Standard PS/2 keyboard responds with 0xAB 0x83
                // AT keyboard responds with single 0xAB
                if ((ps2_data_bytes_received >= 1 && ps2_data_buffer[0] == 0xAB) ||
                    (ps2_data_bytes_received >= 2 && ps2_data_buffer[1] == 0x83)) 
                {
                    ps2_device_1_type = PS2_DEVICE_KEYBOARD;
                    LOG(INFO, "Keyboard detected on port 1");
                }
            }
            ps2_send_device_command(1, PS2_ENABLE_SCANNING);
        }
    }

    if (ps2_device_2_connected) 
    {
        if (ps2_send_device_command(2, PS2_DISABLE_SCANNING) == PS2_ACK) 
        {
            if (ps2_send_device_command(2, PS2_IDENTIFY) == PS2_ACK) 
            {
                ps2_read_data();
                
                if ((ps2_data_bytes_received >= 1 && ps2_data_buffer[0] == 0xAB) ||
                    (ps2_data_bytes_received >= 2 && ps2_data_buffer[1] == 0x83)) 
                {
                    ps2_device_2_type = PS2_DEVICE_KEYBOARD;
                    LOG(INFO, "Keyboard detected on port 2");
                }
            }
            ps2_send_device_command(2, PS2_ENABLE_SCANNING);
        }
    }
}

void handle_irq_1() 
{
    if (!ps2_controller_connected)
        return;
    
    if (!(inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL))
        return;

    uint8_t data = inb(PS2_DATA);

    if (!ps2_device_1_interrupt)
        return;
    if (!ps2_device_1_connected) 
        return;
    
    if (ps2_device_1_type == PS2_DEVICE_KEYBOARD) 
        ps2_handle_keyboard_scancode(1, data);
}

void handle_irq_12() 
{
    if (!ps2_controller_connected)
        return;

    if (!(inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL))
        return;

    uint8_t data = inb(PS2_DATA);

    if (!ps2_device_2_interrupt)
        return;
    if (!ps2_device_2_connected) 
        return;
    
    if (ps2_device_2_type == PS2_DEVICE_KEYBOARD) 
        ps2_handle_keyboard_scancode(2, data);
}