#include <inttypes.h>
#include <unistd.h>
#include <string.h>

#include "memalloc.h"
#include "utils.h"
#include "parser.h"

static input_stack_t parse_input_stack = {0, 0, NULL};

static void input_stack_resize() {
  int32_t capacity, newcap, size;
  if (parse_input_stack.input == NULL) {
    parse_input_stack.input = (parse_input_t **)
      safe_malloc(INIT_INPUT_STACK_SIZE * sizeof(parse_input_t *));
    parse_input_stack.capacity = INIT_INPUT_STACK_SIZE;
  }
  capacity = parse_input_stack.capacity;
  size = parse_input_stack.size;
  if (capacity < size + 1) {
    newcap = capacity + capacity/2;
    if (MAXSIZE(sizeof(parse_input_t *), 0) - capacity <= capacity/2) {
      out_of_memory();
    }
    parse_input_stack.input = (parse_input_t **)
      safe_realloc(parse_input_stack.input, newcap * sizeof(parse_input_t *));
    parse_input_stack.capacity = newcap;
  }
}

void input_stack_push(parse_input_t *input) {
  input_stack_resize();
  parse_input_stack.input[parse_input_stack.size++] = input;
  parse_input = input;
}

extern bool input_stack_push_file(char *file) {
  FILE *input;
  parse_input_t *pinput;

  if (strlen(file) == 0) {
    input = stdin;
  } else if ((input = fopen(file, "r")) != NULL) {
    printf("Input from file %s\n", file);
  } else {
    printf("File %s could not be opened\n", file);
    return false;
  }
  pinput = safe_malloc(sizeof(parse_input_t));
  pinput->kind = INFILE;
  pinput->input.in_file.file = file;
  pinput->input.in_file.fps = input;
  input_stack_push(pinput);
  return true;
}

extern bool input_stack_push_string(char *str) {
  parse_input_t *pinput;

  pinput = safe_malloc(sizeof(parse_input_t));
  pinput->kind = INSTRING;
  pinput->input.in_string.string = str;
  pinput->input.in_string.index = 0;
  input_stack_push(pinput);
  return true;
}

// Pops the stack, and sets the parse_input
extern void input_stack_pop() {
  assert(parse_input_stack.size > 0);
  if (parse_input->kind == INFILE) {
    fclose(parse_input->input.in_file.fps);
  }
  parse_input_stack.size--;
  if (parse_input_stack.size > 0) {
    parse_input = parse_input_stack.input[parse_input_stack.size - 1];
  }
}

extern bool input_is_stdin(parse_input_t *input) {
  return (input->kind == INFILE && input->input.in_file.fps == stdin);
}
