/* Support for input, i.e., yacc parser */

#ifndef __INPUT_H
#define __INPUT_H 1

#include "tables.h"

#include <stdint.h>
#include <stdbool.h>

extern FILE *mcsat_input;

/* A sort definition in the form of an interval of integers */
typedef struct input_sortdef_s {
	int32_t lower_bound;
	int32_t upper_bound;
} input_sortdef_t;

/* An atom in a input literal */
typedef struct input_atom_s {
	char *pred;
	int32_t builtinop; /* = 0: not a built-in-op, > 0: built-in-op */
	char **args;
} input_atom_t;

/* A literal in an input formula */
typedef struct input_literal_s {
	bool neg; /* true: with negation; false: without negation */
	input_atom_t *atom;
} input_literal_t;

/* An input formula in clause form */
typedef struct input_clause_s {
	int32_t varlen;
	char **variables; /* Input variables */
	int32_t litlen;
	input_literal_t **literals;
} input_clause_t;

/* Composition of two formulas */
typedef struct input_comp_fmla_s {
	int32_t op; /* binary operators such as and, or, implies, iff etc. */
	struct input_fmla_s *arg1;
	struct input_fmla_s *arg2;
} input_comp_fmla_t;

/* A unit is either an atom or a composed formula */
typedef union input_ufmla_s {
	input_atom_t *atom;
	input_comp_fmla_t *cfmla;
} input_ufmla_t;

/* Input FOL formula */
typedef struct input_fmla_s {
	bool atomic;
	input_ufmla_t *ufmla;
} input_fmla_t;

/* 
 * Original formula that is recursively defined, will be converted to CNF later
 */
typedef struct input_formula_s {
	struct var_entry_s **vars; /* NULL terminated list of vars */
	input_fmla_t *fmla;
} input_formula_t;


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
  char **frozen; // TODO: what is this?
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
//   double sa_temperature;
//   double rvar_probability;
//   int32_t max_flips;
//   int32_t max_extra_flips;
// } input_mcsat_decl_t;

typedef struct input_mcsat_params_decl_s {
  int32_t num_params;
  int32_t max_samples;
  double sa_probability;
  double sa_temperature;
  double rvar_probability;
  int32_t max_flips;
  int32_t max_extra_flips;
  int32_t timeout;
  int32_t burn_in_steps;
  int32_t samp_interval;
} input_mcsat_params_decl_t;

typedef struct input_train_params_decl_s {
  int32_t num_params;
  int32_t max_iter;
  double learning_rate;
  double stopping_error;
  int32_t reporting;
} input_train_params_decl_t;

typedef struct input_mwsat_params_decl_s {
  int32_t num_params;
  int32_t num_trials;
  double rvar_probability;
  int32_t max_flips;
  int32_t timeout;
} input_mwsat_params_decl_t;

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

typedef struct input_train_decl_s {
  char *file;
} input_train_decl_t;

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
  input_train_params_decl_t train_params_decl;
  input_mcsat_params_decl_t mcsat_params_decl;
  input_mwsat_params_decl_t mwsat_params_decl;
  input_reset_decl_t reset_decl;
  input_retract_decl_t retract_decl;
  input_load_decl_t load_decl;
  input_train_decl_t train_decl;
  input_verbosity_decl_t verbosity_decl;
  input_dumptable_decl_t dumptable_decl;
  input_help_decl_t help_decl;
} input_decl_t;

typedef struct input_command_s {
  int32_t kind;
  input_decl_t decl;
} input_command_t;

extern input_command_t input_command;

/* Returns true if an 'quit' command is read */
extern bool read_eval(samp_table_t *table);
extern void read_eval_print_loop(char *input, samp_table_t *table);
extern void load_mcsat_file(char *file, samp_table_t *table);

extern void add_sortdef(sort_table_t *sort_table, char *sort, input_sortdef_t *sortdef);

/* Adds a predicate */
extern int32_t add_predicate(char *pred, char **sort, bool directp, samp_table_t *table);

/* Adds an integer to a enumerative integer sort */
extern bool add_int_const(int32_t icnst, sort_entry_t *entry, sort_table_t *sort_table);

/*
 * Adds a constant of a specific sort
 * @cnst: the name of the constant
 * @sort: the name of the sort
 */
extern int32_t add_constant(char *cnst, char *sort, samp_table_t *table);

/*
 * Assert an atom of a witness (direct) predicate. If the predicate is
 * indirect, pop out an error.
 */
extern int32_t assert_atom(samp_table_t *table, input_atom_t *current_atom, char *source);

/* Adds an input atom to the table */
extern int32_t add_atom(samp_table_t *table, input_atom_t *current_atom);

/* Adds an query to the query table and generate instances */
extern int32_t add_query(var_entry_t **vars, rule_literal_t ***lits,
			 samp_table_t *table);

/* handle the command of dumping a table */
extern void dumptable(int32_t tbl, samp_table_t *table);

extern void set_print_exp_p(bool flag);
extern bool get_print_exp_p();

extern void set_pce_rand_seed(uint32_t seed);
extern void rand_reset();

extern int32_t get_max_samples();
extern double get_sa_probability();
extern double get_sa_temperature();
extern double get_rvar_probability();
extern int32_t get_max_flips();
extern int32_t get_max_extra_flips();
extern int32_t get_mcsat_timeout();
extern int32_t get_burn_in_steps();
extern int32_t get_samp_interval();
extern double get_weightlearn_min_error();
extern int32_t get_weightlearn_max_iter();
extern double get_weightlearn_rate();
extern int32_t get_weightlearn_reporting();

extern void set_max_samples(int32_t m);
extern void set_sa_probability(double d);
extern void set_sa_temperature(double d);
extern void set_rvar_probability(double d);
extern void set_max_flips(int32_t m);
extern void set_max_extra_flips(int32_t m);
extern void set_mcsat_timeout(int32_t m);
extern void set_burn_in_steps(int32_t m);
extern void set_samp_interval(int32_t m);
extern void set_weightlearn_min_error(double e);
extern void set_weightlearn_max_iter(int32_t iter);
extern void set_weightlearn_rate(double r);
extern void set_weightlearn_reporting(int32_t iter);

extern bool strict_constants();
extern void set_strict_constants(bool val);
extern bool lazy_mcsat();
extern void set_lazy_mcsat(bool val);

extern char* get_dump_samples_path();
extern void set_dump_samples_path(char *path);
  
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

void set_training_data_file(char *path);

#endif /* __INPUT_H */
