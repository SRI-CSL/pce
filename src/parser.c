#include <inttypes.h>

#include "memalloc.h"
#include "utils.h"
#include "parser.h"

static void file_stack_resize(file_stack_t *stack) {
  if (stack->fps == NULL) {
    stack->fps = (FILE **) safe_malloc(INIT_FILE_STACK_SIZE * sizeof(FILE *));
    stack->capacity = INIT_FILE_STACK_SIZE;
  }
  int32_t capacity = stack->capacity;
  int32_t size = stack->size;
  if (capacity < size + 1){
    if (MAXSIZE(sizeof(FILE *), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    capacity += capacity/2;
    stack->fps = (FILE **)
      safe_realloc(stack->fps, capacity * sizeof(FILE *));
    stack->capacity = capacity;
  }
}

extern void file_stack_push(FILE *fp, file_stack_t *stack) {
  file_stack_resize(stack);
  stack->fps[stack->size++] = fp;
  parse_input = fp;
}

// Pops the stack, and sets the parse_input
extern void file_stack_pop(file_stack_t *stack) {
  assert(stack->size > 0);
  stack->size--;
  parse_input = stack->size > 0 ? stack->fps[stack->size - 1] : stdin;
}
