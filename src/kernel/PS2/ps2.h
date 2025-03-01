#pragma once

#define PS2_DATA                0x60
#define PS2_STATUS_REGISTER     0x64
#define PS2_COMMAND_REGISTER    0x64

#define PS2_WAIT_TIME           5000  // In milliseconds
#define PS2_MAX_RESEND          5
#define PS2_READ_BUFFER_SIZE    8     // 4 should work though

#define ps2_output_ready()      ((inb(PS2_STATUS_REGISTER) & 0b10) == 0)    // Output buffer full   (controller -> device)
#define ps2_input_ready()       ((inb(PS2_STATUS_REGISTER) & 0b01) == 1)    // Input buffer full    (device -> controller)

bool ps2_wait_for_output();
bool ps2_wait_for_intput();

#define PS2_GET_CONFIGURATION   0x20
#define PS2_SET_CONFIGURATION   0x60
#define PS2_DISABLE_DEVICE_2    0xa7
#define PS2_ENABLE_DEVICE_2     0xa8
#define PS2_TEST_DEVICE_2       0xa9
#define PS2_TEST_CONTROLLER     0xaa
#define PS2_TEST_DEVICE_1       0xab
#define PS2_DISABLE_DEVICE_1    0xad
#define PS2_ENABLE_DEVICE_1     0xae
#define PS2_IDENTIFY            0xf2
#define PS2_ENABLE_SCANNING     0xf4
#define PS2_DISABLE_SCANNING    0xf5
#define PS2_ACK                 0xfa
#define PS2_RESEND              0xfe
#define PS2_RESET               0xff

bool ps2_controller_connected;
bool ps2_device_1_connected, ps2_device_2_connected;

uint8_t ps2_data_buffer[PS2_READ_BUFFER_SIZE];
uint8_t ps2_data_bytes_received;

void ps2_controller_init();
void ps2_detect_devices();
uint8_t ps2_send_command_no_data(uint8_t command);
uint8_t ps2_send_command_with_data(uint8_t command, uint8_t data);
void ps2_send_command_no_data_no_answer(uint8_t command);
void ps2_send_command_with_data_no_answer(uint8_t command, uint8_t data);