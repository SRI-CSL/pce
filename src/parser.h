
#ifndef __PARSER_H
#define __PARSER_H

#include "utils.h"

enum input_kind_t {INFILE, INSTRING};

#define INIT_INPUT_STACK_SIZE 4

typedef struct input_file_s {
  char *file;
  FILE *fps;
} input_file_t;

typedef struct input_string_s {
  char *string;
  int32_t index;
} input_string_t;

typedef union input_s {
  input_file_t in_file;
  input_string_t in_string;
} input_t;

typedef struct parse_input_s {
  int32_t kind;
  input_t input;
} parse_input_t;

typedef struct input_stack_s {
  uint32_t size;
  uint32_t capacity;
  parse_input_t **input;
} input_stack_t;

//extern char *parse_file;
extern parse_input_t *parse_input;

//extern bool input_stack_push_stream(FILE *input, char *source);
extern bool input_stack_push_file(char *name);
extern bool input_stack_push_string(char *str);
extern void input_stack_pop();
extern bool input_is_stdin(parse_input_t *input);

#endif
