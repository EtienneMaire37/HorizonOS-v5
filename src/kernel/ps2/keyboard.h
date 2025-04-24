#pragma once

#define PS2_KB_KEY_DETECTION_ERROR_0     0x00
#define PS2_KB_INTERNAL_BUFFER_OVERRUN_0 0x00
#define PS2_KB_SELF_TEST_PASSED          0xaa
#define PS2_KB_ECHO                      0xee
#define PS2_KB_SELF_TEST_FAILED_0        0xfc
#define PS2_KB_SELF_TEST_FAILED_1        0xfd
#define PS2_KB_KEY_DETECTION_ERROR_1     0xff
#define PS2_KB_INTERNAL_BUFFER_OVERRUN_1 0xff

#define PS2_KB_SET_LED_STATE            0xed
#define PS2_KB_ECHO                     0xee
#define PS2_KB_GET_SET_SCANCODE_SET     0xf0
#define PS2_KB_SET_TYPEMATIC_BYTE       0xf3
#define PS2_KB_SET_DEFAULT_PARAMETERS   0xf6

#define PS2_DEVICE_1_KB   (ps2_device_1_connected && ps2_device_1_type == PS2_DEVICE_KEYBOARD)
#define PS2_DEVICE_2_KB   (ps2_device_2_connected && ps2_device_2_type == PS2_DEVICE_KEYBOARD)

bool enable_ps2_kb_input;

uint8_t ps2_kb_1_scancode_set, ps2_kb_2_scancode_set;

void ps2_init_keyboards();
void ps2_handle_keyboard_scancode(uint8_t port, uint8_t scancode);
void ps2_kb_get_scancode_set();