#ifndef __UTILS_H
#define __UTILS_H 1

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "array_hash_map.h"
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
 * - A verbosity_level controls the amount of output produced
 *   it's set to 1 by default. Can be changed via set_verbosity_level
 * - cprintf is a replacement for printf:
 *   cprintf(level, <format>, ...) does nothing if level > verbosity_level
 *   otherwise, it's the same as printf(<format>, ...)
 */
extern void set_verbosity_level(int32_t level);
extern void cprintf(int32_t level, const char *fmt, ...);


/*
 * Return a freshly allocated copy of string s.
 * - the result must be deleted by calling safe_free later
 */
extern char *str_copy(char *s);
  




/*
 * MCSAT MAIN DATA STRUCTURES
 */

/*
 * TODO: Everything from here to the end of this file should be moved
 * to a different file. This stuff is specific to mcsat, putting it
 * into a file called utils.h is not a great idea.
 */

/*
 * Boolean variables are repressented by 32bit integers.
 * For a variable x, the positive literal is 2x, the negative
 * literal is 2x + 1.
 *
 * -1 is a special marker for both variables and literals
 */
typedef int32_t samp_bvar_t;
typedef int32_t samp_literal_t;

enum {
  null_samp_bvar = -1,
  null_literal = -1,
};


/*
 * Maximal number of boolean variables
 */
#define MAX_VARIABLES (INT32_MAX >> 1)

/*
 * Conversions from variables to literals
 */
static inline samp_literal_t pos_lit(samp_bvar_t x) {
  return x<<1;
}

static inline samp_literal_t neg_lit(samp_bvar_t x) {
  return (x<<1) | 1;
}

/*
 * mk_lit(x, 0) = pos_lit(x)
 * mk_lit(x, 1) = neg_lit(x)
 */
static inline samp_literal_t mk_lit(samp_bvar_t x, uint32_t sign) {
  assert((sign & ~1) == 0);
  return (x<<1)|sign;
}

static inline samp_bvar_t var_of(samp_literal_t l) {
  return l>>1;
}

static inline uint32_t sign_of_lit(samp_literal_t l) {
  return ((uint32_t) l) & 1;
}

// negation of literal l
static inline samp_literal_t not(samp_literal_t l) {
  return l ^ 1;
}

// check whether l1 and l2 are opposite
static inline bool opposite(samp_literal_t l1, samp_literal_t l2) {
  return (l1 ^ l2) == 1;
}

// true if l has positive polarity (i.e., l = pos_lit(x))
static inline bool is_pos(samp_literal_t l) {
  return !(l & 1);
}

static inline bool is_neg(samp_literal_t l) {
  return (l & 1);
}


/*
 * Truth values: explain??
 */
typedef enum {
  v_undef = -1,
  v_false = 0,
  v_true = 1,
  v_fixed_false = 2,
  v_fixed_true = 3,
  v_db_false = 4,
  v_db_true = 5,
} samp_truth_value_t;


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
  

/*
 * 
 */
#define SAMP_INIT_CLAUSE_SIZE 8

//A clause has weight/status and a -1-terminated array of literals
//disjunct[0] is watched and should have assignment true, when
//such a literal exists.  The low-bit for link is one if the clause is dead.
typedef struct samp_clause_s {
  double weight;//weight of the clause: 0 for hard clause
  struct samp_clause_s *link; //link to next clause for a given watched literal
  int32_t numlits;
  samp_literal_t disjunct[0];//array of literals 
} samp_clause_t;


//An atom has a predicate symbol and an array of the corresponding arity. 
typedef struct samp_atom_s {
  int32_t pred;
  int32_t args[0];
} samp_atom_t;

typedef struct input_atom_s {
  char *pred;
  char *args[0];
} input_atom_t;

typedef struct input_literal_s {
  bool neg;
  input_atom_t *atom;
} input_literal_t;

// This is used for both clauses (no vars) and rules.
typedef struct input_clause_s {
  int varlen;
  char **variables; //Input variables
  int litlen;
  input_literal_t **literals;
} input_clause_t;



//Each sort has a name (string), and a cardinality.  Each element is also assigned
//a number; the elements are prefixed with the sort.
typedef struct sort_entry_s {
  int32_t size;
  int32_t cardinality; //number of elements in sort i
  char *name;//print name of the sort
  int32_t *constants; //array of constants in the given sort
} sort_entry_t;
  
typedef  struct sort_table_s {
  int32_t size;
  int32_t num_sorts; //number of sorts
  stbl_t sort_name_index;//table giving index for sort name
  sort_entry_t *entries;//maps sort index to cardinality and name
} sort_table_t;

#define INIT_SORT_TABLE_SIZE 64
#define INIT_CONST_TABLE_SIZE 64
#define INIT_VAR_TABLE_SIZE 64
#define INIT_PRED_TABLE_SIZE 64
#define INIT_ATOM_PRED_SIZE 16
#define INIT_RULE_PRED_SIZE 16
#define INIT_ATOM_TABLE_SIZE 64
#define INIT_CLAUSE_TABLE_SIZE 64
#define INIT_RULE_TABLE_SIZE 64
#define INIT_SORT_CONST_SIZE 16
#define INIT_CLAUSE_SIZE 16
#define INIT_ATOM_SIZE 16
#define INIT_MODEL_TABLE_SIZE 64
#define INIT_SUBSTIT_TABLE_SIZE 8 // number of vars in rules - likely to be small

typedef  struct pred_entry_s {
  int32_t arity;//number of arguments; negative for evidence predicate
  int32_t *signature;//pointer to an array of sort indices
  char *name;//print name
  int32_t size_atoms;
  int32_t num_atoms;
  int32_t *atoms;
  // Keep track of the rules that mention this predicate
  int32_t size_rules;
  int32_t num_rules;
  int32_t *rules;
} pred_entry_t;

typedef struct pred_tbl_s {
  int32_t size;
  int32_t num_preds;
  pred_entry_t *entries;
} pred_tbl_t;

typedef struct pred_table_s  {
  pred_tbl_t evpred_tbl; //signature map for evidence predicates
  pred_tbl_t pred_tbl;//signature map for normal predicates
  stbl_t pred_name_index;//symbol table for all predicates
} pred_table_t;

typedef  struct const_entry_s {
  char *name;
  uint32_t sort_index; 
} const_entry_t;

typedef struct const_table_s {
  int32_t size;
  int32_t num_consts;
  const_entry_t *entries; 
  stbl_t const_name_index; //table mapping const name to index
} const_table_t;

typedef struct var_entry_s {
  char *name;
  uint32_t sort_index; 
} var_entry_t;

typedef struct var_table_s {
  int32_t size;
  int32_t num_vars;
  var_entry_t *entries; 
  stbl_t var_name_index; //table mapping var name to index
} var_table_t;

typedef struct atom_entry_s {
  samp_atom_t *atom;
  samp_truth_value_t assignment; 
} atom_entry_t;

typedef struct atom_table_s {
  int32_t size; //size of the var_atom array (double for watched)
  int32_t num_vars;  //number of bvars
  int32_t num_unfixed_vars;
  samp_atom_t **atom; // atom_entry_t *entries;
  samp_truth_value_t *assignment;
  int32_t num_samples;
  int32_t *pmodel;
  array_hmap_t atom_var_hash; //maps atoms to variables
} atom_table_t;

typedef struct clause_table_s {
  int32_t size;   //size of the samp_clauses array
  int32_t num_clauses; //number of clause entries
  samp_clause_t **samp_clauses; //array of pointers to samp_clauses
  array_hmap_t clause_hash; //maps clauses to index in clause table
  samp_clause_t **watched; //maps literals to samp_clause pointers
  int32_t num_unsat_clauses; //number of unsat clauses
  samp_clause_t *unsat_clauses;//list of unsat clauses, threaded through link
  samp_clause_t *dead_clauses;//list of unselected clauses
  samp_clause_t *negative_or_unit_clauses; //list of negative weight or unit 
                                           //clauses for fast unit propagation
  samp_clause_t *dead_negative_or_unit_clauses; //killed clauses
  samp_clause_t *sat_clauses; //list of fixed satisfied clauses
} clause_table_t;

// A rule is a clause with variables.  Instantiated forms of the rules are
// added to the clause table.  The variables list is kept with each rule,
// and the literal list is a list of actual literals, in order to reference
// the variables.

typedef struct rule_literal_s {
  bool neg;
  samp_atom_t *atom;
} rule_literal_t;

typedef struct samp_rule_s {
  int32_t num_lits; //number of literal entries
  int32_t num_vars; //number of variables
  var_entry_t **vars; //The variables
  rule_literal_t **literals; //array of pointers to rule_literals
  double weight;
} samp_rule_t;

typedef struct rule_table_s {
  int32_t size;   //size of the samp_rule array
  int32_t num_rules; //number of rule entries
  samp_rule_t **samp_rules; //array of pointers to samp_rules
} rule_table_t;

typedef struct samp_table_s {
  sort_table_t sort_table;
  const_table_t const_table;
  var_table_t var_table;
  pred_table_t pred_table;
  atom_table_t atom_table;
  clause_table_t clause_table;
  rule_table_t rule_table;
  integer_stack_t fixable_stack;
} samp_table_t;

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



/*
 * Explain these functions: what do they do?
 */
extern void init_sort_table(sort_table_t *sort_table);

extern void reset_sort_table(sort_table_t *sort_table);

extern void add_sort(sort_table_t *sort_table, char *name);

extern int32_t sort_name_index(char *name, sort_table_t *sort_table);

extern int32_t *sort_signature(char **in_signature, int32_t arity, sort_table_t *sort_table);

extern void init_const_table(const_table_t *const_table);

extern int32_t const_index(char *name, const_table_t *const_table);

extern int32_t const_sort_index(int32_t const_index, const_table_t *const_table);

extern int32_t var_index(char *name, var_table_t *var_table);

extern void init_pred_table(pred_table_t *pred_table);

extern void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table);

extern bool pred_epred(int32_t predicate);

extern int32_t pred_val_to_index(int32_t val);

extern bool atom_eatom(int32_t atom_id, pred_table_t *pred_table, atom_table_t *atom_table);

extern pred_entry_t *pred_entry(pred_table_t *pred_table, int32_t predicate);

extern int32_t pred_arity(int32_t predicate, pred_table_t *pred_table);

extern int32_t pred_index(char * in_predicate, pred_table_t *pred_table);

extern char *pred_name(int32_t pred, pred_table_t *pred_table);

extern void init_atom_table(atom_table_t *table);

extern void atom_table_resize(atom_table_t *atom_table, clause_table_t *clause_table);

extern samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity);

extern void init_clause_table(clause_table_t *table);

extern void clause_table_resize(clause_table_t *clause_table);


/*
 * Is there a need to make this extern?
 */
extern clause_buffer_t clause_buffer;

extern void clause_buffer_resize(int32_t length);


extern void init_rule_table(rule_table_t *table);

extern void rule_table_resize(rule_table_t *rule_table);

extern void init_samp_table(samp_table_t *table);

extern void print_atoms(samp_table_t *samp_table);

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

extern bool valid_table(samp_table_t *table);
extern bool valid_atom_table(atom_table_t *atom_table,
			     pred_table_t *pred_table,
			     const_table_t *const_table,
			     sort_table_t *sort_table);


#endif /* __UTILS_H */     
