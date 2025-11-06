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

void dump_xsave_header(uint8_t* s, bool print) 
{
    uint64_t xstate_bv = *(uint64_t*)(s + 512);       // XSTATE_BV
    uint64_t xcomp_bv  = *(uint64_t*)(s + 520);       // XCOMP_BV
    if (print)
    {
        printf("XSTATE_BV = 0x%x\n", (uint64_t)xstate_bv);
        printf("XCOMP_BV  = 0x%x\n", (uint64_t)xcomp_bv);
    }
    LOG(DEBUG, "XSTATE_BV = 0x%x", (uint64_t)xstate_bv);
    LOG(DEBUG, "XCOMP_BV  = 0x%x", (uint64_t)xcomp_bv);
    for (int i = 16; i < 64; i++) 
    {
        if (s[i] != 0)
            printf("Header byte %d != 0 (value 0x%02x)\n", i, s[i]);
    }
}

extern void enable_avx();
extern uint32_t get_xsave_area_size();