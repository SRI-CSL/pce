#ifndef __INTEGER_STACK_H
#define __INTEGER_STACK_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define MAX_SIZE(size, offset) ((UINT32_MAX - offset)/size)

typedef struct integer_stack_s {
  int32_t size;
  int32_t top;
  int32_t *elems;
} integer_stack_t;

#define INIT_INTEGER_STACK_SIZE 16

void init_integer_stack(integer_stack_t *istack,
			int32_t size);

void resize_integer_stack(integer_stack_t *istack);

void push_integer_stack(int32_t val,
			integer_stack_t *istack);

void pop_integer_stack(integer_stack_t *istack);

int32_t top_integer_stack(integer_stack_t *istack);

bool empty_integer_stack(integer_stack_t *istack);

int32_t length_integer_stack(integer_stack_t *istack);

int32_t nth_integer_stack(uint32_t n, integer_stack_t *istack);

void print_integer_stack(integer_stack_t *istack);
  
#endif /* __INTEGER_STACK_H */     
