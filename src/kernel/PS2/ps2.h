#pragma once

#define PS2_DATA                0x60
#define PS2_STATUS_REGISTER     0x64
#define PS2_COMMAND_REGISTER    0x64

#define PS2_WAIT_TIME           5000  // In milliseconds
#define PS2_MAX_RESEND          5
#define PS2_READ_BUFFER_SIZE    2

#define ps2_output_ready()      ((inb(PS2_STATUS_REGISTER) & 0b10) == 0)    // Wait to be able to output to the PS/2 device
#define ps2_input_ready()       ((inb(PS2_STATUS_REGISTER) & 0b01) == 1)    // Wait to be able to receive data from the PS/2 device

bool ps2_wait_for_output();
bool ps2_wait_for_intput();

#define PS2_ACK                 0xfa
#define PS2_RESEND              0xfe
#define PS2_ENABLE_SCANNING     0xf4
#define PS2_DISABLE_SCANNING    0xf5
#define PS2_IDENTIFY            0xf2

bool ps2_device_1_connected, ps2_device_2_connected;

uint8_t ps2_data_buffer[PS2_READ_BUFFER_SIZE];
uint8_t ps2_data_bytes_received;

void ps2_detect_devices();
uint8_t ps2_send_command_no_data(uint8_t command);