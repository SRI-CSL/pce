#ifndef __UTILS_H
#define __UTILS_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include "array_hash_map.h"
#include "symbol_tables.h"
#include "integer_stack.h"

extern int32_t verbosity_level;
extern void cprintf(int32_t level, const char *fmt, ...);
extern char * str_copy(char *name);
  
//from simplexstructures.h
#define MAX_SIZE(size, offset) ((UINT32_MAX - offset)/size)

//The representation of Boolean variables and literals is taken from
//dpll.h 

//The active clauses are represented in an array.  
/* #define WEIGHT_MAX INT32_MAX >> 2 */
/* #define ALIVE_MASK 1 */
/* #define ACTIVE_MASK 2 */

/* int32_t get_alive(int32_t weight){ */
/*   return (weight & ALIVE_MASK); */
/* } */

/* int32_t get_active(int32_t weight){ */
/*   return (weight & ACTIVE_MASK); */
/* } */

/* int32_t get_weight(int32_t weight){ */
/*   return (weight/4); */
/* } */

//From dpll.h

/***************************************
 *    BOOLEAN VARIABLES AND LITERALS   *
 **************************************/

/*
 * Boolean variables: integers between 0 and nvars - 1
 * Literals: integers between 0 and 2nvar - 1.
 *
 * For a variable x, the positive literal is 2x, the negative
 * literal is 2x + 1.
 *
 * -1 is a special marker for both variables and literals
 *
 * Optionally, two literals representing true/false can be created
 * when the solver is initialized.
 * - if this is done, variable 0 = the constant
 * - literal 0 == true, literal 1 == false
 */
typedef int32_t samp_bvar_t;
typedef int32_t samp_literal_t;

enum {
  null_samp_bvar = -1,
  null_literal = -1,

  // constants
  bool_const = 0, // bool variable
  true_literal = 0,
  false_literal = 1,
};

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

/*
#define ALIVE_MASK ((samp_clause_t *) 1)

bool alive_clause(samp_clause_t *clause){
  return (clause->link & ALIVE_MASK); //will this cast to bool?
}

samp_clause_t *link_link(samp_clause_t *link){//is this correct? 
  return (link &  ~ALIVE_MASK);
}
*/

/*A predicate symbol uses its least ARITY_BITS (6) bits
#define ARITY_BITS 6
#define ARITY_MASK ((1 << ARITY_BITS) - 1)
#define EPRED_BIT 7
#define EPRED_MASK (1 << ARITY_BITS)
#define PRED_MAX (INT32_MAX >> EPRED_BIT)
  
extern int32_t  pred_index(int32_t predicate);


extern int32_t pred_epred(int32_t predicate);

extern int32_t pred_arity(int32_t predicate);
*/

//Constants will be encoded as positive integers and
//variables as negative integers.  

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

//A samp_atomlit is a literal that contains a pointer to the atom (NS: Not used)
/*
typedef (samp_atom_t *) samp_atomlit_t;
//the low-order bit is one for negated literal

#define NEG_TAG ((intptr_t) 0x1)

samp_atomlit_t negate_samp_atom(samp_atomlit_t lit){
  return ((samp_atomlit_t)(((intptr_t) lit) ^ NEG_TAG));
}

bool is_neg_samp_atomlit(samp_atomlit_t lit){
  return ((((intptr_t) lit) & NEG_TAG) == 1);
}

samp_atomlit_t get_samp_atomlit_atom(samp_atomlit_t lit){
  return ((samp_atomlit_t)(((intptr_t) lit) & ~NEG_TAG));
}
*/


//To check for clause equality, we sort the literals and calculate a
//hash: a hashmap maps the clause to its index in the clause table


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

extern void init_sort_table(sort_table_t *sort_table);

extern void reset_sort_table(sort_table_t *sort_table);

extern void add_sort(sort_table_t *sort_table, char *name);


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
  double *pmodel;
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

int32_t sort_name_index(char *name, sort_table_t *sort_table);

int32_t *sort_signature(char **in_signature, int32_t arity, sort_table_t *sort_table);

void init_const_table(const_table_t *const_table);

int32_t const_index(char *name, const_table_t *const_table);

int32_t const_sort_index(int32_t const_index, const_table_t *const_table);

int32_t var_index(char *name, var_table_t *var_table);

void init_pred_table(pred_table_t *pred_table);

void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table);

bool pred_epred(int32_t predicate);

int32_t pred_val_to_index(int32_t val);

bool atom_eatom(int32_t atom_id, pred_table_t *pred_table, atom_table_t *atom_table);

pred_entry_t *pred_entry(pred_table_t *pred_table, int32_t predicate);

int32_t pred_arity(int32_t predicate, pred_table_t *pred_table);

int32_t pred_index(char * in_predicate, pred_table_t *pred_table);

char *pred_name(int32_t pred, pred_table_t *pred_table);

void init_atom_table(atom_table_t *table);

void atom_table_resize(atom_table_t *atom_table, clause_table_t *clause_table);

samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity);

void init_clause_table(clause_table_t *table);

void clause_table_resize(clause_table_t *clause_table);

extern clause_buffer_t clause_buffer;

void clause_buffer_resize (int32_t length);

void init_rule_table(rule_table_t *table);

void rule_table_resize(rule_table_t *rule_table);

void init_samp_table(samp_table_t *table);

void print_atoms(samp_table_t *samp_table);

void print_clauses(samp_table_t *samp_table);

void print_clause_table(samp_table_t *table, int32_t num_vars);

void print_rules(rule_table_t *rule_table);

void print_state(samp_table_t *table);

clause_buffer_t substit_buffer;

void substit_buffer_resize(int32_t length);

bool assigned_undef(samp_truth_value_t value);
bool assigned_true(samp_truth_value_t value);
bool assigned_false(samp_truth_value_t value);
bool assigned_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
bool assigned_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);
bool assigned_fixed_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
bool assigned_fixed_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);

bool valid_atom_table(atom_table_t *atom_table,
		      pred_table_t *pred_table,
		      const_table_t *const_table,
		      sort_table_t *sort_table);

int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause);

extern bool valid_table(samp_table_t *table);

#endif /* __UTILS_H */     
