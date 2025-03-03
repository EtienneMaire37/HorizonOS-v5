#pragma once

#define PS2_DATA                0x60
#define PS2_STATUS_REGISTER     0x64
#define PS2_COMMAND_REGISTER    0x64

#define PS2_WAIT_TIME           3000  // In milliseconds
#define PS2_MAX_RESEND          5
#define PS2_READ_BUFFER_SIZE    8     // 4 should work though

// Status Register Flags
#define PS2_STATUS_OUTPUT_FULL  0x01  // Bit 0: Output buffer full (data available)
#define PS2_STATUS_INPUT_FULL   0x02  // Bit 1: Input buffer full  (controller busy)

// Controller Commands
#define PS2_GET_CONFIGURATION   0x20
#define PS2_SET_CONFIGURATION   0x60
#define PS2_DISABLE_DEVICE_2    0xA7
#define PS2_ENABLE_DEVICE_2     0xA8
#define PS2_TEST_DEVICE_2       0xA9
#define PS2_TEST_CONTROLLER     0xAA
#define PS2_TEST_DEVICE_1       0xAB
#define PS2_DISABLE_DEVICE_1    0xAD
#define PS2_ENABLE_DEVICE_1     0xAE
#define PS2_WRITE_DEVICE_2      0xD4

#define PS2_DEVICE_TEST_PASS    0x00
#define PS2_DEVICE_RESET        0xFF
#define PS2_SELF_TEST_OK        0x55
#define PS2_DEVICE_BAT_OK       0xAA
#define PS2_DEVICE_ID_KEYBOARD  0xAB
#define PS2_DEVICE_ID_MOUSE     0x00

// Device Commands
#define PS2_IDENTIFY            0xF2
#define PS2_ENABLE_SCANNING     0xF4
#define PS2_DISABLE_SCANNING    0xF5
#define PS2_ACK                 0xFA
#define PS2_RESEND              0xFE
#define PS2_RESET               0xFF

// Device Types
#define PS2_DEVICE_UNKNOWN      0
#define PS2_DEVICE_KEYBOARD     1
#define PS2_DEVICE_MOUSE        2

uint8_t ps2_device_1_type;
uint8_t ps2_device_2_type;

bool ps2_device_1_interrupt, ps2_device_2_interrupt;

bool ps2_controller_connected;
bool ps2_device_1_connected, ps2_device_2_connected;
uint8_t ps2_data_buffer[PS2_READ_BUFFER_SIZE];
uint8_t ps2_data_bytes_received;

bool ps2_wait_for_output();
bool ps2_wait_for_input();
void ps2_read_data();
void ps2_controller_init();
// void ps2_detect_devices();
void ps2_detect_keyboards();
void ps2_enable_interrupts();
uint8_t ps2_send_command(uint8_t command);
uint8_t ps2_send_command_with_data(uint8_t command, uint8_t data);
bool ps2_send_device_command(uint8_t device, uint8_t command);
void ps2_flush_buffer();

void handle_irq_1();
void handle_irq_12();