#pragma once

bool ps2_wait_for_output() 
{
    uint32_t timeout = global_timer + PS2_WAIT_TIME;
    while ((inb(PS2_STATUS_REGISTER) & PS2_STATUS_INPUT_FULL) && timeout > global_timer);
    return (global_timer >= timeout);
}

bool ps2_wait_for_input() 
{
    uint32_t timeout = global_timer + PS2_WAIT_TIME;
    while (!(inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL) && timeout > global_timer);
    return (global_timer >= timeout);
}

void ps2_flush_buffer() 
{
    while (inb(PS2_STATUS_REGISTER) & PS2_STATUS_OUTPUT_FULL)
        inb(PS2_DATA);
}

uint8_t ps2_send_command(uint8_t command) 
{
    if (ps2_wait_for_output()) return 0xff;
    outb(PS2_COMMAND_REGISTER, command);
    if (ps2_wait_for_input()) return 0xff;
    return inb(PS2_DATA);
}

uint8_t ps2_send_command_with_data(uint8_t command, uint8_t data) 
{
    if (ps2_wait_for_output()) return 0xff;
    outb(PS2_COMMAND_REGISTER, command);
    if (ps2_wait_for_output()) return 0xff;
    outb(PS2_DATA, data);
    if (ps2_wait_for_input()) return 0xff;
    return inb(PS2_DATA);
}

uint8_t ps2_send_device_command(uint8_t device, uint8_t command) 
{
    uint8_t response;
    for (int tries = 0; tries < PS2_MAX_RESEND; tries++) 
    {
        if (device == 2) 
        {
            response = ps2_send_command_with_data(PS2_WRITE_DEVICE_2, command);
        } 
        else 
        {
            if (ps2_wait_for_output()) continue;
            outb(PS2_DATA, command);
            if (ps2_wait_for_input()) continue;
            response = inb(PS2_DATA);
        }
        
        if (response == PS2_ACK) return response;
        if (response != PS2_RESEND) break;
    }
    return 0xff;
}

void ps2_read_data() 
{
    ps2_data_bytes_received = 0;
    while (!ps2_wait_for_input() && ps2_data_bytes_received < PS2_READ_BUFFER_SIZE) 
        ps2_data_buffer[ps2_data_bytes_received++] = inb(PS2_DATA);
}

void ps2_controller_init() 
{
    ps2_controller_connected = ps2_device_1_connected = ps2_device_2_connected = 0;

    // Disable devices and flush
    ps2_send_command(PS2_DISABLE_DEVICE_1);
    ps2_send_command(PS2_DISABLE_DEVICE_2);
    ps2_flush_buffer();

    // Controller self-test
    if (ps2_send_command(PS2_TEST_CONTROLLER) != 0x55) 
    {
        LOG(ERROR, "PS/2 Controller failed self-test");
        return;
    }

    // Configure controller
    uint8_t config = ps2_send_command(PS2_GET_CONFIGURATION);
    config &= ~0x03;    // Disable interrupts
    config &= ~0x30;    // Enable both ports
    ps2_send_command_with_data(PS2_SET_CONFIGURATION, config);

    ps2_send_command(PS2_ENABLE_DEVICE_1);
    if (ps2_send_command(PS2_TEST_DEVICE_1) != 0x00) 
    {
        LOG(WARNING, "PS/2 Device 1 not responding");
        return;
    } 
    else 
    {
        ps2_device_1_connected = true;
    }

    if (ps2_send_command(PS2_ENABLE_DEVICE_2) == 0x00) 
    {
        if (ps2_send_command(PS2_TEST_DEVICE_2) == 0x00) 
        {
            ps2_device_2_connected = true;
        }
    }

    config |= 0x01;     // Enable device1 interrupt
    config &= ~0x40;    // Disable translation
    if (ps2_device_2_connected) config |= 0x02; // Enable device 2 interrupt
    ps2_send_command_with_data(PS2_SET_CONFIGURATION, config);

    ps2_controller_connected = true;
    LOG(INFO, "PS/2 Controller initialized");
}

void ps2_detect_devices() 
{
    ps2_device_1_type = ps2_device_2_type = PS2_DEVICE_UNKNOWN;
    if (ps2_device_1_connected) 
    {
        if (ps2_send_device_command(1, PS2_IDENTIFY) == PS2_ACK) 
        {
            ps2_read_data();
            LOG(INFO, "Device1 ID: 0x%x 0x%x", 
                ps2_data_buffer[0], ps2_data_buffer[1]);
        }
        else
            ps2_device_1_connected = false;
    }
    
    if (ps2_device_2_connected) 
    {
        if (ps2_send_device_command(2, PS2_IDENTIFY) == PS2_ACK) 
        {
            ps2_read_data();
            LOG(INFO, "Device2 ID: 0x%x 0x%x",
                ps2_data_buffer[0], ps2_data_buffer[1]);
        }
        else
            ps2_device_2_connected = false;
    }
}

void ps2_detect_keyboards() 
{
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
    if (ps2_device_1_type == PS2_DEVICE_KEYBOARD) 
    {
        uint8_t scancode = inb(PS2_DATA);
        
        handle_keyboard_input(1, scancode);
    }
}

void handle_irq_12() 
{
    if (ps2_device_1_type == PS2_DEVICE_KEYBOARD) 
    {
        uint8_t scancode = inb(PS2_DATA);
        
        handle_keyboard_input(2, scancode);
    }
}