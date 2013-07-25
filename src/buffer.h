#ifndef __BUFFER_H
#define __BUFFER_H 1

#include "tables.h"

#define INIT_INPUT_CLAUSE_BUFFER_SIZE 8
#define INIT_INPUT_LITERAL_BUFFER_SIZE 8
#define INIT_INPUT_ATOM_BUFFER_SIZE 8

#define INIT_STRING_BUFFER_SIZE 32

typedef struct atom_buffer_s {
	int32_t size;
	int32_t *data;
} atom_buffer_t;

typedef struct clause_buffer_s {
  int32_t size;
  int32_t *data;
} clause_buffer_t;

typedef struct substit_buffer_s {
  int32_t size;
  substit_entry_t *entries;
} substit_buffer_t;

typedef struct input_clause_buffer_s {
  uint32_t capacity;
  uint32_t size;
  input_clause_t **clauses;
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

typedef struct string_buffer_s {
  uint32_t capacity;
  uint32_t size;
  char *string;
} string_buffer_t;

extern void substit_buffer_resize(int32_t length);
extern void clause_buffer_resize(int32_t length);
extern void atom_buffer_resize(atom_buffer_t *atom_buffer, int32_t arity);

extern input_clause_t *new_input_clause();
extern input_literal_t *new_input_literal();
extern input_atom_t *new_input_atom();

extern char *get_string_from_buffer(string_buffer_t *strbuf);

#endif /* __BUFFER_H */

