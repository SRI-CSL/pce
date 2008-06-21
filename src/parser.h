
#ifndef __PARSER_H
#define __PARSER_H

#include "utils.h"

#define INIT_FILE_STACK_SIZE 4

typedef struct file_stack_s {
  uint32_t size;
  uint32_t capacity;
  FILE **fps;
} file_stack_t;

extern FILE *parse_input;

extern void file_stack_push(FILE *fp, file_stack_t *stack);
extern void file_stack_pop(file_stack_t *stack);

#endif
