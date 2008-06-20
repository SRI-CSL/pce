#ifndef __UTILS_H
#define __UTILS_H 1

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "array_hash_map.h"
#include "tables.h"
#include "symbol_tables.h"
#include "integer_stack.h"


/*
 * Missing comment: what does this do?
 * FIX: MAX_SIZE is used already. We need a different name
 */
#define MAXSIZE(size, offset) ((UINT32_MAX - (offset))/(size))


/*
 * min and max of two integers
 */
extern int32_t imax(int32_t i, int32_t j);
extern int32_t imin(int32_t i, int32_t j);


/*
 * Return a freshly allocated copy of string s.
 * - the result must be deleted by calling safe_free later
 */
extern char *str_copy(char *s);

static inline bool db_tval(samp_truth_value_t v){
  return (v == v_db_true || v == v_db_false);
}

static inline bool fixed_tval(samp_truth_value_t v){
  return (v == v_fixed_true || v == v_fixed_false);
}

static inline bool unfixed_tval(samp_truth_value_t v){
  return (v == v_true || v == v_false);
}


/*
 * This function is too large to be inlined
 */
static inline samp_truth_value_t negate_tval(samp_truth_value_t v){
  switch (v){
  case v_true:
    return v_false;
  case v_false:
    return v_true;
  case v_db_true:
    return v_db_false;
  case v_db_false:
    return v_db_true;
  case v_fixed_true:
    return v_fixed_false;
  case v_fixed_false:
    return v_fixed_true;
  default:
    return v_undef;
  }
}
  

typedef struct clause_buffer_s {
  int32_t size;
  int32_t *data;
} clause_buffer_t;

typedef struct substit_entry_s {
  int32_t const_index;
  bool fixed;
} substit_entry_t;

typedef struct substit_buffer_s {
  int32_t size;
  substit_entry_t *entries;
} substit_buffer_t;

extern void free_atom(input_atom_t *atom);
extern void free_clause(input_clause_t *clause);
extern void free_strings (char **string);

extern void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table);

extern void print_atoms(samp_table_t *samp_table);

extern void print_atom(samp_atom_t *atom, samp_table_t *table);

extern void print_clauses(samp_table_t *samp_table);

extern void print_clause_table(samp_table_t *table, int32_t num_vars);

extern void print_rules(rule_table_t *rule_table);

extern void print_state(samp_table_t *table, uint32_t round);

/*
 * Same question?
 */
extern substit_buffer_t substit_buffer;

extern void substit_buffer_resize(int32_t length);


extern bool assigned_undef(samp_truth_value_t value);
extern bool assigned_true(samp_truth_value_t value);
extern bool assigned_false(samp_truth_value_t value);
extern bool assigned_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_fixed_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_fixed_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);

extern int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause);
extern int32_t eval_neg_clause(samp_truth_value_t *assignment, samp_clause_t *clause);


/*
 * Is there a need to make this extern?
 */
extern clause_buffer_t clause_buffer;

extern void clause_buffer_resize(int32_t length);


#endif /* __UTILS_H */     
