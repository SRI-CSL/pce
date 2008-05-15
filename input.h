/* Support for input, i.e., yacc parser */

#ifndef __INPUT_H
#define __INPUT_H 1

#include "utils.h"

#define INIT_INPUT_CLAUSE_BUFFER_SIZE 8
#define INIT_INPUT_LITERAL_BUFFER_SIZE 8
#define INIT_INPUT_ATOM_BUFFER_SIZE 8

typedef struct input_clause_buffer_s {
  uint32_t capacity;
  uint32_t size;
  input_clause_t *clauses;
} input_clause_buffer_t;

typedef struct input_literal_buffer_s {
  uint32_t capacity;
  uint32_t size;
  input_literal_t *literals;
} input_literal_buffer_t;

typedef struct input_atom_buffer_s {
  uint32_t capacity;
  uint32_t size;
  input_atom_t *atoms;
} input_atom_buffer_t;

extern input_clause_buffer_t input_clause_buffer;
extern input_literal_buffer_t input_literal_buffer;
extern input_atom_buffer_t input_atom_buffer;

extern void input_clause_buffer_resize ();
extern void input_literal_buffer_resize ();
extern void input_atom_buffer_resize ();

extern input_clause_t *new_input_clause ();
extern input_literal_t *new_input_literal ();
extern input_atom_t *new_input_atom ();
  
#endif /* __INPUT_H */     
