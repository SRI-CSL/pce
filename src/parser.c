#include <inttypes.h>
#include <unistd.h>
#include <string.h>

#include "memalloc.h"
#include "utils.h"
#include "parser.h"

static input_stack_t parse_input_stack = {0, 0, NULL, NULL};

static void input_stack_resize() {
  int32_t capacity, newcap, size;
  if (parse_input_stack.file == NULL) {
    parse_input_stack.file = (char **) safe_malloc(INIT_INPUT_STACK_SIZE * sizeof(char *));
    parse_input_stack.fps = (FILE **) safe_malloc(INIT_INPUT_STACK_SIZE * sizeof(FILE *));
    parse_input_stack.capacity = INIT_INPUT_STACK_SIZE;
  }
  capacity = parse_input_stack.capacity;
  size = parse_input_stack.size;
  if (capacity < size + 1) {
    newcap = capacity + capacity/2;
    if (MAXSIZE(sizeof(char *), 0) - capacity <= capacity/2) {
      out_of_memory();
    }
    parse_input_stack.file = (char **)
      safe_realloc(parse_input_stack.file, newcap * sizeof(char *));
    
    if (MAXSIZE(sizeof(FILE *), 0) - capacity <= capacity/2) {
      out_of_memory();
    }
    parse_input_stack.fps = (FILE **)
      safe_realloc(parse_input_stack.fps, newcap * sizeof(FILE *));
    
    parse_input_stack.capacity = newcap;
  }
}

extern bool input_stack_push(char *file) {
  FILE *input;
    
  if (strlen(file) == 0) {
    input = stdin;
  } else if ((input = fopen(file, "r")) != NULL) {
    printf("Input from file %s\n", file);
  } else {
    printf("File %s could not be opened\n", file);
    return false;
  }
  input_stack_resize();
  parse_input_stack.file[parse_input_stack.size] = file;
  parse_input_stack.fps[parse_input_stack.size] = input;
  parse_input_stack.size++;
  parse_file = file;
  parse_input = input;
  return true;
}

// Pops the stack, and sets the parse_input
extern void input_stack_pop() {
  assert(parse_input_stack.size > 0);
  fclose(parse_input);
  parse_input_stack.size--;
  if (parse_input_stack.size > 0) {
    parse_file = parse_input_stack.file[parse_input_stack.size - 1];
    parse_input = parse_input_stack.fps[parse_input_stack.size - 1];
  }
}
