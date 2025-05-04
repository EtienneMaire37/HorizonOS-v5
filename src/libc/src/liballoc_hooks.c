bool spinlock_state = false;

int liballoc_lock()
{
	while (spinlock_state);
    spinlock_state = true;
	return 0;
}

int liballoc_unlock()
{
	spinlock_state = false;
	return 0;
}

void* liballoc_alloc(int pages)
{
	uint32_t page_number = 0;
    int continuous_pages = 0;

    while (true)
    {
        if (malloc_bitmap_get_page(page_number + continuous_pages))
        {
            page_number += continuous_pages + 1;
            continuous_pages = 0;
        }
        else
            continuous_pages++;

        if (pages <= continuous_pages)
        {
            uint32_t page_address = heap_address + 4096 * page_number;
            if (break_address < page_address + 4096 * pages)
                if (brk((void*)page_address + 4096 * pages) == -1)
                    return NULL;
            for (uint32_t i = page_number; i < page_number + continuous_pages; i++)
                malloc_bitmap_set_page(i, true);
            return (void*)page_address;
        }
    }

    return NULL;
}

int liballoc_free(void* ptr, int pages)
{
    uint32_t page_number = ((uint32_t)ptr - heap_address) / 4096;
	for (uint32_t i = page_number; i < page_number + pages; i++)
        malloc_bitmap_set_page(i, false);
    brk((void*)heap_address + 4096 * malloc_last_allocated_page_index);     // Shouldn't cause an error
    return 0;
}