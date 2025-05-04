uint32_t break_address, heap_address, alloc_break_address;
uint32_t heap_size;

void* sbrk(intptr_t incr)
{
    void* old_break = (void*)break_address;
    if (brk((void*)((uint32_t)(break_address + incr))) != 0)
        return (void*)-1;
    return old_break;
}