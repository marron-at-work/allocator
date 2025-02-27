#pragma once

#include "xalloc.h"

struct StackSegment {
    void** data;
    struct StackSegment* next;
};

struct Stack {
    void** top;
    void** min;
    void** max;
    struct StackSegment* current;
};

extern thread_local struct Stack marking_stack;

void stack_push_slow(struct Stack* s, void* obj);
void* stack_pop_slow(struct Stack* s);

/**
 * Stack design -- top points to the current (valid) top of the stack and is null if the stack is empty
 * -- min is the first valid slot (data[0]) and max is the last valid slot (data[len -1]) 
 */

//ALWAYS CALL AT TOP LEVEL WITH SIMPLE ARGUMENTS
#define stack_empty(S) ((S).top == NULL)

#define stack_push(T, S, O) if((S).top < (S).max) { *(++(S).top) = O; } else { stack_push_slow(&(S), O); }
#define stack_pop(T, S) ((T*)((S).top != (S).min ? *((S).top--) : stack_pop_slow(&(S))))
