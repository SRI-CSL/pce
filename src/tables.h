#ifndef __TABLES_H
#define __TABLES_H 1

#include "symbol_tables.h"
#include "integer_stack.h"
#include "utils.h"
#include "vectors.h"

/*
 * MCSAT MAIN DATA STRUCTURES
 */

/*
 * TODO: Truth values: explain??
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

/*
 * Boolean variables are represented by 32bit integers.
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
  bool *frozen; // array indicating whether associated literal is frozen
  samp_literal_t disjunct[0];//array of literals
} samp_clause_t;


// An atom has a predicate symbol and an array of the corresponding arity.
// Note that to get the arity, the pred_table must be available unless the
// predicate is builtin.
typedef struct samp_atom_s {
  int32_t pred; // <= 0: direct pred; > 0: indirect pred
  int32_t args[0];
} samp_atom_t;

typedef struct input_sortdef_s {
  int32_t lower_bound;
  int32_t upper_bound;
} input_sortdef_t;

typedef struct input_atom_s {
  char *pred;
  int32_t builtinop;
  char **args;
} input_atom_t;

typedef struct input_literal_s {
  bool neg;
  input_atom_t *atom;
} input_literal_t;

// This is used for both clauses (no vars) and rules.
typedef struct input_clause_s {
  int32_t varlen;
  char **variables; //Input variables
  int32_t litlen;
  input_literal_t **literals;
} input_clause_t;

// This is used for formulas
typedef struct input_comp_fmla_s {
  int32_t op;
  struct input_fmla_s *arg1;
  struct input_fmla_s *arg2;
} input_comp_fmla_t;

typedef union input_ufmla_s {
  input_atom_t *atom;
  input_comp_fmla_t *cfmla;
} input_ufmla_t;

typedef struct input_fmla_s {
  bool atomic;
  input_ufmla_t *ufmla;
} input_fmla_t;

typedef struct var_entry_s {
  char *name;
  int32_t sort_index; 
} var_entry_t;

/* original formula that is recursively defined,
 * will be converted to CNF later
 */
typedef struct input_formula_s {
  var_entry_t **vars; // NULL terminated list of vars
  input_fmla_t *fmla;
} input_formula_t;


//Each sort has a name (string), and a cardinality.  Each element is also assigned
//a number; the elements are prefixed with the sort.
typedef struct sort_entry_s {
  int32_t size;
  int32_t cardinality; //number of elements in sort i
  char *name;//print name of the sort
  int32_t *constants; //array of constants in the given sort
  int32_t *ints; // array of integers for sparse integer sorts
  int32_t lower_bound; // lower and upper bounds for integer sorts
  int32_t upper_bound; //    (could be a union type)
  int32_t *subsorts; // array of subsort indices
  int32_t *supersorts; // array of supersort indices
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
#define INIT_QUERY_TABLE_SIZE 16
#define INIT_QUERY_INSTANCE_TABLE_SIZE 64
#define INIT_SOURCE_TABLE_SIZE 16
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
  int32_t *atoms; // This is atom indices, not atoms
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
  pred_tbl_t pred_tbl;//signature map for non-evidence predicates
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

typedef struct var_table_s {
  int32_t size;
  int32_t num_vars;
  var_entry_t *entries; 
  stbl_t var_name_index; //table mapping var name to index
} var_table_t;

// typedef struct atom_entry_s {
//   samp_atom_t *atom;
//   samp_truth_value_t assignment; 
// } atom_entry_t;

typedef struct atom_table_s {
  int32_t size; //size of the var_atom array (double for watched)
  int32_t num_vars;  //number of bvars
  int32_t num_unfixed_vars;
  samp_atom_t **atom; // atom_entry_t *entries;
  int32_t *sampling_nums; // Keeps track of number of samplings when atom was
                      // introduced - used for more accurate probs.
  uint32_t current_assignment; //which of two assignment arrays is current
  samp_truth_value_t *assignment[2];//maps atom ids to samp_truth_value_t
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

typedef enum {constant, variable, integer} arg_kind_t;

// In a rule, an argument may be:
//  a constant (value is an index into the const_table)
//  a variable (value is an index into the variable array of the associated rule)
//  an integer (value is the integer)
// When the rule is instantiated, a samp_atom_t is created, which only has constants
// and integers - these are determined by the sorts.
// We can't do this for rules, as this would not allow a distinction between
// integers and variables.
typedef struct rule_atom_arg_s {
  arg_kind_t kind;
  int32_t value;
} rule_atom_arg_t;

typedef struct rule_atom_s {
  int32_t pred;
  int32_t builtinop;
  rule_atom_arg_t *args;
} rule_atom_t;

typedef struct rule_literal_s {
  bool neg;
  rule_atom_t *atom;
} rule_literal_t;

/* ground clause */
typedef struct samp_rule_s {
  int32_t num_lits; //number of literal entries
  int32_t num_vars; //number of variables
  int32_t num_frozen; //number of frozen predicates
  var_entry_t **vars; //The variables
  rule_literal_t **literals; //array of pointers to rule_literals
  int32_t *frozen_preds; //array of frozen predicates
  int32_t *clause_indices; //array of indices into clause_table
  double weight;
} samp_rule_t;

typedef struct rule_table_s {
  int32_t size;   //size of the samp_rule array
  int32_t num_rules; //number of rule entries
  samp_rule_t **samp_rules; //array of pointers to samp_rules
} rule_table_t;


// Queries involve two tables; the query_table_t keeps the uninstantiated
// form of the query, to be instantiated as new constants come in.  The
// query_instance_table_t keeps the instantiated form, along with the
// sampling numbers and pmodels as for the atom_table_t.

// Similar to rules, but we can't separate the clauses, so the literals
// array is one level deeper.
typedef struct samp_query_s {
  int32_t *source_index; // The source of this query, e.g., formula, learner id
  int32_t num_clauses;
  int32_t num_vars;
  var_entry_t **vars;
  rule_literal_t ***literals;
} samp_query_t;

typedef struct query_table_s {
  int32_t size; //size of the query array
  int32_t num_queries;
  samp_query_t **query;
} query_table_t;

typedef struct samp_query_instance_s {
  //int32_t query_index; // Index to the query from which this was generated
  ivector_t query_indices; // Indices to the queries from which this was generated
  int32_t sampling_num; // The num_samples when this instance was created
  int32_t pmodel; // The nimber of samples for which this instance was true
  int32_t *subst; // Holds the mapping from vars to consts
  bool *constp; // Whether given var is a constant or integer
  samp_literal_t **lit; // The instance - a conjunction of disjunctions of lits
} samp_query_instance_t;

typedef struct query_instance_table_s {
  int32_t size; //size of the lit array
  int32_t num_queries;
  samp_query_instance_t **query_inst;
} query_instance_table_t;

// Sources indicate the source of the assertion, rule, etc.
// and is used to reset, untell, etc.  For each explicitly
// provided source, we keep track of the formula and assertion
// So we can undo the effects.  For weighted formulas, we keep
// track of the weight so we can subtract it later.
typedef struct source_entry_s {
  char *name;
  int32_t *assertion; // indices to atom_table, -1 terminated
  int32_t *clause; // indices to clause_table; -1 terminated
  double *weight; // weights corresponding to clause list
} source_entry_t;

typedef struct source_table_s {
  int32_t size;
  int32_t num_entries;
  source_entry_t **entry;
} source_table_t;

typedef struct samp_table_s {
  sort_table_t sort_table;
  const_table_t const_table;
  var_table_t var_table;
  pred_table_t pred_table;
  atom_table_t atom_table;
  clause_table_t clause_table; // ground formulas (without variables)
  rule_table_t rule_table; // formulas with variables
  query_table_t query_table;
  query_instance_table_t query_instance_table;
  source_table_t source_table;
  integer_stack_t fixable_stack;
} samp_table_t;
  

/*
 * Explain these functions: what do they do?
 */
extern void init_sort_table(sort_table_t *sort_table);

extern void reset_sort_table(sort_table_t *sort_table);

extern void add_sort(sort_table_t *sort_table, char *name);

void add_sortdef(sort_table_t *sort_table, char *sort, input_sortdef_t *sortdef);

extern void add_subsort(sort_table_t *sort_table, char *subsort, char *supersort);

extern int32_t add_pred(pred_table_t *pred_table, char *name, bool evidence,
			int32_t arity, sort_table_t *sort_table, char **in_signature);

extern int32_t sort_name_index(char *name, sort_table_t *sort_table);

extern int32_t *sort_signature(char **in_signature, int32_t arity, sort_table_t *sort_table);

extern void init_const_table(const_table_t *const_table);

extern int32_t const_index(char *name, const_table_t *const_table);

extern int32_t const_sort_index(int32_t const_index, const_table_t *const_table);

extern void add_const_to_sort(int32_t const_index,
			      int32_t sort_index,
			      sort_table_t *sort_table);

extern int32_t var_index(char *name, var_table_t *var_table);

extern void init_pred_table(pred_table_t *pred_table);

extern bool pred_epred(int32_t predicate);

extern int32_t pred_val_to_index(int32_t val);

extern bool atom_eatom(int32_t atom_id, pred_table_t *pred_table, atom_table_t *atom_table);

extern pred_entry_t *pred_entry(pred_table_t *pred_table, int32_t predicate);

extern int32_t pred_arity(int32_t predicate, pred_table_t *pred_table);

extern int32_t pred_index(char * in_predicate, pred_table_t *pred_table);

extern char *pred_name(int32_t pred, pred_table_t *pred_table);

extern int32_t *pred_signature(int32_t predicate, pred_table_t *pred_table);

extern void init_atom_table(atom_table_t *table);

extern void atom_table_resize(atom_table_t *atom_table, clause_table_t *clause_table);

extern samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity);

extern void init_clause_table(clause_table_t *table);

extern void clause_table_resize(clause_table_t *clause_table);

extern void init_rule_table(rule_table_t *table);

extern void rule_table_resize(rule_table_t *rule_table);

extern void init_query_table(query_table_t *table);

extern void query_table_resize(query_table_t *table);

extern void init_query_instance_table(query_instance_table_t *table);

extern void query_instance_table_resize(query_instance_table_t *table);

extern void reset_query_instance_table(query_instance_table_t *table);

extern void init_source_table(source_table_t *table);

extern void source_table_extend(source_table_t *table);

extern void reset_source_table(source_table_t *table);

extern void init_samp_table(samp_table_t *table);

extern bool valid_table(samp_table_t *table);

extern bool valid_atom_table(atom_table_t *atom_table,
			     pred_table_t *pred_table,
			     const_table_t *const_table,
			     sort_table_t *sort_table);
extern int32_t add_const_internal (char *name, int32_t sort_index,
				   samp_table_t *table);

extern int32_t add_const(char *name, char * sort_name, samp_table_t *table);

extern char *const_name(int32_t const_index, const_table_t *const_table);

// The builtin binary predicates

extern char* builtinop_string (int32_t bop);

extern int32_t builtin_arity (int32_t op);

extern int32_t atom_arity(rule_atom_t *atom, pred_table_t *pred_table);

extern bool call_builtin (int32_t bop, int32_t arity, int32_t *args);
  
#endif /* __TABLES_H */
