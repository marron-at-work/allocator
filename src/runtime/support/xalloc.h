
/**
 * This is the memory allocator that we will use for MANUALLY manged (NON-GC) memory that we need when implementing the GC itself and other runtime support.
 * At this point it supports a pool of pages and allocation from this pool.
 * 
 * This is a thread local (static) allocator.
 */
#pragma once

#include "../common.h"

void xallocInitializePageManager();

/**
 * Get a page from the system
 */
void* xallocAllocatePage();

/**
 * Free a page back to the system
 */
void xallocFreePage(void* page);
