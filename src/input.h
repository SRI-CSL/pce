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

typedef struct input_pred_decl_s {
  input_atom_t *atom;
  bool witness;
} input_pred_decl_t;

typedef struct input_sort_decl_s {
  char *name;
  input_sortdef_t *sortdef; // Holds intervals
} input_sort_decl_t;

typedef struct input_subsort_decl_s {
  char *subsort;
  char *supersort;
} input_subsort_decl_t;

typedef struct input_const_decl_s {
  int32_t num_names;
  char **name;
  char *sort;
} input_const_decl_t;

typedef struct input_var_decl_s {
  int32_t num_names;
  char **name;
  char *sort;
} input_var_decl_t;

typedef struct input_atom_decl_s {
  input_atom_t *atom;
} input_atom_decl_t;

typedef struct input_assert_decl_s {
  input_atom_t *atom;
  char *source;
} input_assert_decl_t;

typedef struct input_add_fdecl_s {
  input_formula_t *formula;
  double weight;
  char *source;
} input_add_fdecl_t;

typedef struct input_ask_fdecl_s {
  input_formula_t *formula;
  double threshold;
  int32_t numresults;
} input_ask_fdecl_t;

typedef struct input_add_decl_s {
  input_clause_t *clause;
  double weight;
  char *source;
} input_add_decl_t;

typedef struct input_ask_decl_s {
  input_clause_t *clause;
  double threshold;
  bool all;
  int32_t num_samples;
} input_ask_decl_t;

// typedef struct input_mcsat_decl_s {
//   int32_t max_samples;
//   double sa_probability;
//   double samp_temperature;
//   double rvar_probability;
//   int32_t max_flips;
//   int32_t max_extra_flips;
// } input_mcsat_decl_t;

typedef struct input_mcsat_params_decl_s {
  int32_t num_params;
  int32_t max_samples;
  double sa_probability;
  double samp_temperature;
  double rvar_probability;
  int32_t max_flips;
  int32_t max_extra_flips;
} input_mcsat_params_decl_t;

typedef struct input_reset_decl_s {
  int32_t kind;
} input_reset_decl_t;

typedef struct input_retract_decl_s {
  char *source;
} input_retract_decl_t;

typedef struct input_load_decl_s {
  char *file;
} input_load_decl_t;

typedef struct input_dumptable_decl_s {
  int32_t table;
} input_dumptable_decl_t;

typedef struct input_help_decl_s {
  int32_t command;
} input_help_decl_t;

typedef struct input_verbosity_decl_s {
  int32_t level;
} input_verbosity_decl_t;

typedef union input_decl_s {
  input_sort_decl_t sort_decl;
  input_subsort_decl_t subsort_decl;
  input_pred_decl_t pred_decl;
  input_const_decl_t const_decl;
  input_var_decl_t var_decl;
  input_atom_decl_t atom_decl;
  input_assert_decl_t assert_decl;
  input_add_fdecl_t add_fdecl;
  input_ask_fdecl_t ask_fdecl;
  input_add_decl_t add_decl;
  input_ask_decl_t ask_decl;
  //input_mcsat_decl_t mcsat_decl;
  input_mcsat_params_decl_t mcsat_params_decl;
  input_reset_decl_t reset_decl;
  input_retract_decl_t retract_decl;
  input_load_decl_t load_decl;
  input_verbosity_decl_t verbosity_decl;
  input_dumptable_decl_t dumptable_decl;
  input_help_decl_t help_decl;
} input_decl_t;

typedef struct input_command_s {
  int32_t kind;
  input_decl_t decl;
} input_command_t;

extern input_command_t input_command;

extern bool read_eval(samp_table_t *table);
extern void read_eval_print_loop(char *input, samp_table_t *table);
extern void load_mcsat_file(char *file, samp_table_t *table);

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

extern int32_t add_predicate(char *pred, char **sort, bool directp, samp_table_t *table);
extern int32_t add_constant(char *cnst, char *sort, samp_table_t *table);
extern void dumptable(int32_t tbl, samp_table_t *table);

extern int32_t get_max_samples();
extern double get_sa_probability();
extern double get_samp_temperature();
extern double get_rvar_probability();
extern int32_t get_max_flips();
extern int32_t get_max_extra_flips();

extern void set_max_samples(int32_t m);
extern void set_sa_probability(double d);
extern void set_samp_temperature(double d);
extern void set_rvar_probability(double d);
extern void set_max_flips(int32_t m);
extern void set_max_extra_flips(int32_t m);

extern bool strict_constants();
extern void set_strict_constants(bool val);
extern bool lazy_mcsat();
extern void set_lazy_mcsat(bool val);
  
extern void input_clause_buffer_resize ();
extern void input_literal_buffer_resize ();
extern void input_atom_buffer_resize ();

extern input_clause_t *new_input_clause ();
extern input_literal_t *new_input_literal ();
extern input_atom_t *new_input_atom ();

extern void free_atom(input_atom_t *atom);
extern void free_clause(input_clause_t *clause);

extern void free_var_entries(var_entry_t **vars);

extern void free_fmla(input_fmla_t *fmla);
extern void free_formula(input_formula_t *formula);
extern void free_samp_atom(samp_atom_t *atom);
extern void free_rule_literal(rule_literal_t *lit);
extern void free_rule_literals(rule_literal_t **lit);
extern void free_samp_query(samp_query_t *query);
extern void free_samp_query_instance(samp_query_instance_t *qinst);
  
extern input_fmla_t *yy_fmla(int32_t op, input_fmla_t *arg1, input_fmla_t *arg2);
extern input_fmla_t *yy_atom_to_fmla (input_atom_t *atom);
extern input_formula_t *yy_formula (char **vars, input_fmla_t *fmla);

extern int32_t str2int(char *cnst);
extern void add_int_const(int32_t icnst, sort_entry_t *entry, sort_table_t *sort_table);

#endif /* __INPUT_H */     
