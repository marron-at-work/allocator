#include "../src/runtime/memory/gc.h"

struct TypeInfoBase Empty = {
    .type_id = 0,
    .type_size = 8,
    .slot_size = 1,
    .ptr_mask = "0",  
    .typekey = "Empty"
};

struct TypeInfoBase ListNode = {
    .type_id = 1,
    .type_size = 16,
    .slot_size = 2,
    .ptr_mask = "01",  
    .typekey = "ListNode"
};

struct TypeInfoBase TreeNode = {
    .type_id = 1,
    .type_size = 16,
    .slot_size = 2,
    .ptr_mask = "11",  
    .typekey = "TreeNode"
};

/**
* This whole test is pretty scuffed. I am not totally confident with exactly what is expected
* to happen in this unique case. To my knowledge we should have one root that points to two
* gc local variables eligible for evacuation. Our root variable cannot be moved because 
* he is pointed to externally.
**/
int run() {
    AllocatorBin* bin16 = getBinForSize(16);
    AllocatorBin* bin8 = getBinForSize(8);

    void** root = (void**)allocate(bin16, &TreeNode);

    root[0] = allocate(bin8, &Empty);
    root[1] = allocate(bin8, &Empty);

    collect();

    debug_print("%p %p %p\n", root, root[0], root[1]);

    /* Proper evacuation and promotion */
    assert(bin16->evac_page == NULL);
    assert(bin8->evac_page->freecount == bin8->evac_page->entrycount - 2);

    /* These pages should have been moved to their page managers (and totally empty) */
    assert(bin16->alloc_page->freecount == bin16->roots_count - 1);
    assert(bin8->alloc_page == NULL);

    /* Root still points to leafs */
    assert(root[0] != NULL);
    assert(root[1] != NULL);

    /* Type preservation */
    assert(GC_TYPE(root) == &TreeNode);
    assert(GC_TYPE(root[0]) == &Empty);
    assert(GC_TYPE(root[1]) == &Empty);

    /** 
    * Looks like our pointers do not properly update after being moved, although this may not be 
    * easily detectable since we arent modifying these variables themselves? idk...
    **/
    assert(GC_IS_ALLOCATED(root) == true);
    assert(GC_IS_ALLOCATED(root[0]) == true);
    assert(GC_IS_ALLOCATED(root[1]) == true);

    assert(GC_IS_YOUNG(root) == false);
    assert(GC_IS_YOUNG(root[0]) == false);
    assert(GC_IS_YOUNG(root[1]) == false);

    return 0;
}

/**
* If we do not initialize startup and thread stuff from main THEN do our allocations
* ceratin objects do not get found on the stack. 
**/
int main(int argc, char** argv) {
    initializeStartup();
    initializeThreadLocalInfo();

    run();

    return 0;
}