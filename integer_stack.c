#include <inttypes.h>

#include "memalloc.h"
#include "integer_stack.h"

void init_integer_stack(integer_stack_t *istack,
			int32_t size){
  if (size <= 0){
    size = INIT_INTEGER_STACK_SIZE;
  }
  if (MAX_SIZE(sizeof(int32_t), 0) < size){
    out_of_memory();
  }
  istack->elems = (int32_t *) safe_malloc(size * sizeof(int32_t));
  istack->size = size;
  istack->top = 0;
}

void resize_integer_stack(integer_stack_t *istack){
  int32_t size = istack->size;
  if (MAX_SIZE(sizeof(int32_t), 0) - size < size/2){
    out_of_memory();
  }
  istack->size += istack->size/2;
  istack->elems = (int32_t *) safe_realloc(istack->elems, istack->size * sizeof(int32_t)); 
}

void push_integer_stack(int32_t val,
			integer_stack_t *istack){
  if (istack->top >= istack->size){
    resize_integer_stack(istack);
  }
  istack->elems[istack->top++] = val;
}

void pop_integer_stack(integer_stack_t *istack){
  if (istack->top > 0){
    istack->elems[--istack->top] = 0;
  }
}

int32_t top_integer_stack(integer_stack_t *istack){
  if (istack->top > 0){
    return istack->elems[istack->top - 1];
  } else {
    return -1;
  }
}

void clear_integer_stack(integer_stack_t *istack){
  istack->top = 0;
}

int32_t nth_integer_stack(uint32_t n, integer_stack_t *istack){
  if (n <= istack->top)
    return istack->elems[n];
  return -1;
}

bool empty_integer_stack(integer_stack_t *istack){
  return (istack->top == 0);
}

int32_t length_integer_stack(integer_stack_t *istack){
  return (istack->top); 
}

void print_integer_stack(integer_stack_t *istack){
  int32_t i;
  printf("[size: %"PRId32", top: %"PRId32", (", istack->size, istack->top);
  for (i = 0; i < istack->top; i++){
    if (i != 0) printf(", ");
    printf("%"PRId32, istack->elems[i]);
  }
  printf(")]\n");
}
