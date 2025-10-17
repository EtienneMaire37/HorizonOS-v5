#pragma once

void enable_sse()
{
    uint32_t cr0 = get_cr0();
    cr0 &= ~(1 << 3);   // * Clear TS
    cr0 &= ~(1 << 2);   // * Clear EM
    cr0 |= (1 << 1);    // * Set MP
    load_cr0(cr0);

    uint32_t cr4 = get_cr4();
    cr4 |= (1 << 9);    // * OSFXSR
    cr4 |= (1 << 10);   // * OSXMMEXCPT
    cr4 &= ~(1 << 11);  // * !UMIP
    load_cr4(cr4);
}

extern void enable_avx();