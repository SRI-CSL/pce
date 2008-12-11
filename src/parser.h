
#ifndef __PARSER_H
#define __PARSER_H

#include "utils.h"

#define INIT_INPUT_STACK_SIZE 4

typedef struct input_stack_s {
  uint32_t size;
  uint32_t capacity;
  char **file;
  FILE **fps;
} input_stack_t;

extern char *parse_file;
extern FILE *parse_input;

extern bool input_stack_push(char *file);
extern void input_stack_pop();

#endif
