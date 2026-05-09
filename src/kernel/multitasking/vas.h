#pragma once

#include "../paging/paging.h"
#include "task.h"
#include "../memalloc/page_frame_allocator.h"

physical_address_t task_create_empty_vas(uint8_t privilege);
void task_free_vas(physical_address_t pml4_address);
