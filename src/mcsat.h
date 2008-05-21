/*
 * DATA STRUCTURES FOR THE MCSAT PARSER.
 */

#ifndef __MCSAT_H
#define __MCSAT_H 1

#include <stdint.h>
#include <stdbool.h>

#include "samplesat.h"

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

#endif /* __MCSAT_H */     
