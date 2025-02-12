#include "threadinfo.h"

#include <stdio.h>

thread_local size_t tl_id;
thread_local void** native_stack_base;
thread_local void** native_stack_contents;
thread_local struct RegisterContents native_register_contents;

/* Need to ensure comparisons only happen under void* */
#define PTR_IN_RANGE(V) ((MIN_ALLOCATED_ADDRESS <= (void*)V) && ((void*)V <= MAX_ALLOCATED_ADDRESS))
#define PTR_NOT_IN_STACK(BASE, CURR, V) (((void*)V < (void*)CURR) | ((void*)BASE < (void*)V))

/* Was originally ..._contents.##R but preprocessor was not happy */
#define PROCESS_REGISTER(BASE, CURR, R)                                       \
    register void* R asm(#R);                                                 \
    native_register_contents.R = NULL;                                        \
    if(PTR_IN_RANGE(R) && PTR_NOT_IN_STACK(BASE, CURR, R)) { native_register_contents.R = R; }

void initializeStartup()
{
    mtx_init(&g_lock, mtx_plain);
}

void initializeThreadLocalInfo()
{
    int lckok = mtx_lock(&g_lock);
    assert(lckok == thrd_success);

    tl_id = tl_id_counter++;
    xallocInitializePageManager(tl_id);

    int unlckok = mtx_unlock(&g_lock);
    assert(unlckok == thrd_success);
    
    register void* rbp asm("rbp");   
    native_stack_base = rbp;
}

/* Need to discuss specifics of this walking, not totally sure about taking the potential ptr to be cur_frame + 1 */
void loadNativeRootSet()
{
    native_stack_contents = (void**)xallocAllocatePage();
    xmem_pageclear(native_stack_contents);

    //this code should load from the asm stack pointers and copy the native stack into the roots memory
    #ifdef __x86_64__
        /* originally current_frame used rsp */
        register void* rbp asm("rbp");
        register void* rsp asm("rsp");
        void** current_frame = rsp;
        void** current_base = rbp;
        int i = 0;

        debug_print("native stack base %p base pointer %p top of stack %p\n", native_stack_base, rbp, rsp);

        /* Walk the stack */
        while (current_frame < native_stack_base) {
            
            /* Walk entire frame looking for valid pointers */
            size_t frame_size = (current_base - current_frame);
            debug_print("current frame size %li\n", frame_size);
            
            for(size_t j = 0; j < frame_size; j++) {
                void* potential_ptr = *(current_frame + j);
                debug_print("potential_ptr %p, current_frame %p\n", potential_ptr, current_frame);
                if (PTR_IN_RANGE(potential_ptr) && PTR_NOT_IN_STACK(native_stack_base, current_frame, potential_ptr)) {
                    native_stack_contents[i++] = potential_ptr;
                    debug_print("current found %i potential pointers\n", i);
                }
            }
            /**
            * rbp stores pointer to next rbp. we can set our current stack pointer
            * to the old rsp to keep iterating up the stack
            **/
            current_frame = current_base; 
            current_base = *(void**)current_base;
        }

        /* Check contents of registers */
        PROCESS_REGISTER(native_stack_base, current_frame, rax)
        PROCESS_REGISTER(native_stack_base, current_frame, rbx)
        PROCESS_REGISTER(native_stack_base, current_frame, rcx)
        PROCESS_REGISTER(native_stack_base, current_frame, rdx)
        PROCESS_REGISTER(native_stack_base, current_frame, rsi)
        PROCESS_REGISTER(native_stack_base, current_frame, rdi)
        PROCESS_REGISTER(native_stack_base, current_frame, r8)
        PROCESS_REGISTER(native_stack_base, current_frame, r9)
        PROCESS_REGISTER(native_stack_base, current_frame, r10)
        PROCESS_REGISTER(native_stack_base, current_frame, r11)
        PROCESS_REGISTER(native_stack_base, current_frame, r12)
        PROCESS_REGISTER(native_stack_base, current_frame, r13)
        PROCESS_REGISTER(native_stack_base, current_frame, r14)
        PROCESS_REGISTER(native_stack_base, current_frame, r15)
    #else
        #error "Architecture not supported"
    #endif
}

void unloadNativeRootSet()
{
    xallocFreePage(native_stack_contents);
}