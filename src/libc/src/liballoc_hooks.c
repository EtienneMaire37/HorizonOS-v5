atomic_flag malloc_spinlock_state = ATOMIC_FLAG_INIT;

#define MALLOC_BITMAP_SIZE      0x100000
#define MALLOC_BITMAP_SIZE_4    0x40000

#define MAX_PAGE_INDEX          (MALLOC_BITMAP_SIZE * 8)

uint8_t malloc_pages_bitmap[MALLOC_BITMAP_SIZE];
uint64_t malloc_last_allocated_page_index;
uint64_t malloc_allocated_pages;

void malloc_bitmap_init()
{
    malloc_last_allocated_page_index = 0;
    malloc_allocated_pages = 0;
    // for (uint16_t i = 0; i < MALLOC_BITMAP_SIZE_4; i++)
    //     *(uint32_t*)((uint32_t)&malloc_pages_bitmap[0] + 4 * (uint32_t)i) = 0;
    for (uint64_t i = 0; i < MALLOC_BITMAP_SIZE; i++)
        malloc_pages_bitmap[i] = 0;
}

bool malloc_bitmap_get_page(uint64_t page) 
{
    uint64_t byte = page / 8;
    if (byte >= MALLOC_BITMAP_SIZE) return 1;
    if (malloc_allocated_pages == 0 || page > malloc_last_allocated_page_index) return 0;

    uint8_t bit = page & 0b111;
    return (malloc_pages_bitmap[byte] >> bit) & 1;
}

void malloc_bitmap_set_page(uint64_t page, bool state)
{    
    uint64_t byte = page / 8;
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
            uint64_t new_last_allocated_page = malloc_last_allocated_page_index - 1;
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

void* liballoc_alloc(size_t pages)
{
	uint64_t page_number = 0;
    uint64_t contiguous_pages = 0;

    while (true)
    {
        if (page_number >= MAX_PAGE_INDEX) return NULL;
        if (malloc_bitmap_get_page(page_number + contiguous_pages))
        {
            page_number += contiguous_pages + 1;
            contiguous_pages = 0;
        }
        else
            contiguous_pages++;

        if (pages <= contiguous_pages) 
        {
            uint64_t page_address = heap_address + 4096 * page_number;
            if (break_address <= page_address + 4096 * pages)
            {
                if (brk((void*)page_address + 4096 * pages) == -1)
                {
                    // write(STDERR_FILENO, "liballoc_alloc failed\n", 22);
                    return NULL;
                }
            }
            for (uint32_t i = page_number; i < page_number + pages; i++)
                malloc_bitmap_set_page(i, true);
            // write(STDERR_FILENO, "liballoc_alloc successfull\n", 27);
            return (void*)page_address;
        }
    }
}

int liballoc_free(void* ptr, size_t pages)
{    
    if (ptr == NULL) return 1;
    uint64_t page_number = ((uint64_t)ptr - heap_address) / 4096;
	for (uint64_t i = page_number; i < page_number + pages; i++)
        malloc_bitmap_set_page(i, false);
    brk((void*)heap_address + 4096 * (malloc_last_allocated_page_index + 1));
    return 0;
}