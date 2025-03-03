#pragma once

#define PS2_KB_KEY_DETECTION_ERROR_0     0x00
#define PS2_KB_INTERNAL_BUFFER_OVERRUN_0 0x00
#define PS2_KB_SELF_TEST_PASSED          0xaa
#define PS2_KB_ECHO                      0xee
#define PS2_KB_SELF_TEST_FAILED_0        0xfc
#define PS2_KB_SELF_TEST_FAILED_1        0xfd
#define PS2_KB_KEY_DETECTION_ERROR_1     0xff
#define PS2_KB_INTERNAL_BUFFER_OVERRUN_1 0xff

bool enable_ps2_kb_input;

void ps2_handle_keyboard_scancode(uint8_t port, uint8_t scancode);