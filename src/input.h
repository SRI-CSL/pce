/* Support for input, i.e., yacc parser */

#ifndef __INPUT_H
#define __INPUT_H 1

#include "utils.h"

#include <stdint.h>
#include <stdbool.h>

#include "samplesat.h"

extern FILE *mcsat_input;

/*
 * Abstract syntax tree
 */

typedef struct input_preddecl_s {
  input_atom_t *atom;
  bool witness;
} input_preddecl_t;

typedef struct input_sortdecl_s {
  char *name;
} input_sortdecl_t;

typedef struct input_constdecl_s {
  int32_t num_names;
  char **name;
  char *sort;
} input_constdecl_t;

typedef struct input_vardecl_s {
  int32_t num_names;
  char **name;
  char *sort;
} input_vardecl_t;

typedef struct input_atomdecl_s {
  input_atom_t *atom;
} input_atomdecl_t;

typedef struct input_assertdecl_s {
  input_atom_t *atom;
} input_assertdecl_t;

typedef struct input_adddecl_s {
  input_clause_t *clause;
  double weight;
} input_adddecl_t;

typedef struct input_askdecl_s {
  input_clause_t *clause;
  double threshold;
  bool all;
} input_askdecl_t;

typedef struct input_mcsatdecl_s {
  double sa_probability;
  double samp_temperature;
  double rvar_probability;
  int32_t max_flips;
  int32_t max_extra_flips;
  int32_t max_samples;
} input_mcsatdecl_t;

typedef struct input_resetdecl_s {
  enum {ALL, PROBABILITIES} kind;
} input_resetdecl_t;

typedef struct input_loaddecl_s {
  char *file;
} input_loaddecl_t;

typedef struct input_verbositydecl_s {
  int32_t level;
} input_verbositydecl_t;

typedef union input_decl_s {
  input_sortdecl_t sortdecl;
  input_preddecl_t preddecl;
  input_constdecl_t constdecl;
  input_vardecl_t vardecl;
  input_atomdecl_t atomdecl;
  input_assertdecl_t assertdecl;
  input_adddecl_t adddecl;
  input_askdecl_t askdecl;
  input_mcsatdecl_t mcsatdecl;
  input_resetdecl_t resetdecl;
  input_loaddecl_t loaddecl;
  input_verbositydecl_t verbositydecl;
} input_decl_t;

typedef struct input_command_s {
  int32_t kind;
  input_decl_t decl;
} input_command_t;

extern input_command_t input_command;

extern void read_eval_print_loop(FILE *input, samp_table_t *table);
extern bool load_mcsat_file(char *file, samp_table_t *table);

#define INIT_INPUT_CLAUSE_BUFFER_SIZE 8
#define INIT_INPUT_LITERAL_BUFFER_SIZE 8
#define INIT_INPUT_ATOM_BUFFER_SIZE 8

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

extern input_clause_buffer_t input_clause_buffer;
extern input_literal_buffer_t input_literal_buffer;
extern input_atom_buffer_t input_atom_buffer;

extern void input_clause_buffer_resize ();
extern void input_literal_buffer_resize ();
extern void input_atom_buffer_resize ();

extern input_clause_t *new_input_clause ();
extern input_literal_t *new_input_literal ();
extern input_atom_t *new_input_atom ();

extern void test (samp_table_t *table);

#endif /* __INPUT_H */     
