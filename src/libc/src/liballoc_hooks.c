atomic_flag malloc_spinlock_state = ATOMIC_FLAG_INIT;

#define MALLOC_BITMAP_SIZE      98304
#define MALLOC_BITMAP_SIZE_4    24576

uint8_t malloc_pages_bitmap[MALLOC_BITMAP_SIZE]; // 3GB / 4KB / 8B
uint32_t malloc_last_allocated_page_index;
uint32_t malloc_allocated_pages;

void malloc_bitmap_init()
{
    malloc_last_allocated_page_index = 0;
    malloc_allocated_pages = 0;
    // for (uint16_t i = 0; i < MALLOC_BITMAP_SIZE_4; i++)
    //     *(uint32_t*)((uint32_t)&malloc_pages_bitmap[0] + 4 * (uint32_t)i) = 0;
    for (uint32_t i = 0; i < MALLOC_BITMAP_SIZE; i++)
        malloc_pages_bitmap[i] = 0;
}

bool malloc_bitmap_get_page(uint32_t page) 
{
    uint32_t byte = page / 8;
    if (byte >= MALLOC_BITMAP_SIZE) return 1;
    if (malloc_allocated_pages == 0 || page >= (uint32_t)malloc_last_allocated_page_index) return 0;

    uint8_t bit = page & 0b111;
    return (malloc_pages_bitmap[byte] >> bit) & 1;
}

void malloc_bitmap_set_page(uint32_t page, bool state)
{    
    uint32_t byte = page / 8;
    if (byte >= MALLOC_BITMAP_SIZE) return;
    uint8_t bit = page & 0b111;
    if (state)  
    {
        if (!(malloc_pages_bitmap[byte] & (1 << bit)))
            malloc_allocated_pages++;
        malloc_pages_bitmap[byte] |= (1 << bit);
        if (malloc_last_allocated_page_index < page) 
            malloc_last_allocated_page_index = page;
    }   
    else
    {
        if (malloc_pages_bitmap[byte] & (1 << bit))
            malloc_allocated_pages--;
        malloc_pages_bitmap[byte] &= ~(1 << bit);

        if (malloc_last_allocated_page_index == page)
        {
            if (malloc_allocated_pages == 0)
            {
                malloc_last_allocated_page_index = 0;
                return;
            }
            uint32_t new_last_allocated_page = malloc_last_allocated_page_index - 1;
            while (malloc_bitmap_get_page(new_last_allocated_page) == 0)
                new_last_allocated_page--;
            malloc_last_allocated_page_index = new_last_allocated_page;
        }
    }
}

int liballoc_lock()
{
	acquire_spinlock(&malloc_spinlock_state);
	return 0;
}

int liballoc_unlock()
{
	release_spinlock(&malloc_spinlock_state);
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
            if (break_address <= page_address + 4096 * pages)
                if (brk((void*)page_address + 4096 * pages) == -1)
                    return NULL;
            for (uint32_t i = page_number; i < page_number + pages; i++)
                malloc_bitmap_set_page(i, true);
            // printf("Successfully allocated pages 0x%x-0x%x\n", page_number, page_number + pages);
            return (void*)page_address;
        }
    }
}

int liballoc_free(void* ptr, int pages)
{
    // fprintf(stderr, "Freeing pages!!!\n");
    if (ptr == NULL) return 1;
    uint32_t page_number = ((uint32_t)ptr - heap_address) / 4096;
	for (uint32_t i = page_number; i < page_number + pages; i++)
        malloc_bitmap_set_page(i, false);
    brk((void*)heap_address + 4096 * malloc_last_allocated_page_index);     // * Shouldn't cause an error
    return 0;
}