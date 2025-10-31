#pragma onced

void invlpg(uint32_t addr)
{
    asm volatile("invlpg [%0]" :: "r" (addr) : "memory");
}

void load_pd_by_physaddr(physical_address_t addr)
{
    if (addr >> 32)
    {
        LOG(CRITICAL, "Tried to load a page directory above 4GB");
        abort();
    }
    
    asm volatile("mov cr3, eax" :: "a" ((uint32_t)addr) : "memory");
}

void load_pd(void* ptr)
{
    load_pd_by_physaddr((physical_address_t)ptr);
}