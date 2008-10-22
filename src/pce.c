/* Needs inlcude dirs for libicl.h, glib.h, e.g.,
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/include \
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1/glib \
// -I /homes/owre/src/oaa2.3.2/lib/x86-linux/glib-2.0/include \
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1

// And for linking, needs this (doesn't work on 64-bit at the moment,
//   unless you recompile OAA)
// -L /homes/owre/src/oaa2.3.2/lib/x86-linux -loaa2
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <libicl.h>
#include <liboaa.h>
#include "utils.h"
#include "print.h"
#include "input.h"
#include "mcsat.h"
#include "yacc.tab.h"
#include "samplesat.h"
#include "lazysamplesat.h"
#include "memalloc.h"
#include "pce.h"

#define AGENT_NAME "pce"

#define MALLOC_CHECK_ 2

static bool lazy_pce = false;

static const char *const pce_solvables[] = {
  "pce_add_user_defined_rule(Username,_Text,Rule)",
  "pce_current_assertions(L)",
  "pce_current_subscriptions(L)",
  "pce_dump_cache(Filename)",
  "pce_exit",
  "pce_fact(Formula)",
  "pce_fact(Source,Formula)", // Defined here
  "pce_full_model(M)",
  "pce_get_cost(Cost)",
  "pce_learner_assert(Lid,Formula,Weight)", // Defined here
  "pce_learner_assert_list(Lid,List)", // Defined here
  "pce_load_file(File)",
  "pce_num_samples(N)",
  "pce_process_subscriptions(X)", // Defined here
  "pce_query(QueryFormula)",
  "pce_query_others(LearnerId,Q,T,F,P)",
  "pce_queryp(Q,P)", // Defined here
  "pce_reset",
  "pce_reset(Flags)",
  "pce_reset([])",
  "pce_reset([p1(Val1),p2(Val2),...])",
  "pce_reset_cache(Pattern)", // Defined here
  "pce_status(Status)",
  "pce_subscribe(Lid,Formula,Id)", // Defined here
  "pce_subscribe(Lid,Formula,Who,Condition,Id)", // Defined here
  "pce_unsubscribe(Lid,F)",
  "pce_unsubscribe(Lid,F,Who,Condition)",
  "pce_unsubscribe(SubscriptionId)",
  "pce_unsubscribe_learner(Lid)",
  "pce_untell(Lid,Formula)",
  "pce_untell_file(File,Lid,Formula)",
  "pce_update_model",
  "pce_version(VersionAtom)",
  "pce_write_full_model(M)"
};

static const char *const pce_callbacks[] = {
  "app_do_event",
  "app_idle",
  "app_done"
};

// names_buffer is temporary storage for names
static names_buffer_t names_buffer = {0, 0, NULL};

// Stores learner ids
static names_buffer_t learner_ids = {0, 0, NULL};

// There are two rules_vars_buffers - as a formula is clausified by cnf,
// variables are added to the rules_vars_buffer.  At the end,
static rule_vars_buffer_t rules_vars_buffer = {0, 0, NULL};
static rule_vars_buffer_t rule_vars_buffer = {0, 0, NULL};

static samp_table_t samp_table;

static void pce_error(const char *fmt, ...);
static rule_literal_t ***cnf(ICLTerm *formula);
static rule_literal_t ***cnf_pos(ICLTerm *formula);
static rule_literal_t ***cnf_neg(ICLTerm *formula);
static rule_literal_t ***cnf_product(rule_literal_t ***c1,
				     rule_literal_t ***c2);
static rule_literal_t ***cnf_union(rule_literal_t ***c1,
				   rule_literal_t ***c2);
static rule_literal_t *** icl_to_cnf_literal(ICLTerm *formula, bool neg);
static rule_literal_t *icl_to_rule_literal(ICLTerm *formula, bool neg);
static void names_buffer_resize(names_buffer_t *nbuf);
static void rule_vars_buffer_resize(rule_vars_buffer_t *buf);
static var_entry_t **rule_vars_buffer_copy(rule_vars_buffer_t *buf);
static char **names_buffer_copy(names_buffer_t *nbuf);

static char *pce_init_file = "pce.init";
static char *pce_save_file = "pce.persist";
static FILE *pce_save_fp;
static FILE *pce_log_fp = NULL;

// Logs a message, along with a term if not null
void pce_log(const char *fmt, ...) {
  va_list argp;

  if (pce_log_fp != NULL) {
    va_start(argp, fmt);
    vfprintf(pce_log_fp, fmt, argp);
    va_end(argp);
    fprintf(pce_log_fp, "\n");
    fflush(pce_log_fp);
    fsync(fileno(pce_log_fp));
  }
}

// Saves the given goal, which is then repeated when PCE starts up
void pce_save(ICLTerm *goal) {
  char *str;

  str = icl_NewStringFromTerm(goal);
  fprintf(pce_save_fp, "%s\n", str);
  icl_stFree(str);
  fflush(pce_save_fp);
  fsync(fileno(pce_save_fp));
}

// Check that the Term is a simple atom, i.e., p(a, b, c, ...) where a, b,
// c, ... are all constants or variables.  Also check for free variables if
// vars_allowed is false.
static bool icl_IsAtomStruct(ICLTerm *icl_term, bool vars_allowed) {
  ICLTerm *icl_arg;
  int32_t i;
  char *icl_string;

  if (!icl_IsStruct(icl_term)) {
    icl_string = icl_NewStringFromTerm(icl_term);
    pce_error("%s is not an atom", icl_string);
    icl_stFree(icl_string);
    return false;
  }
  if (strcmp(icl_Functor(icl_term), "iris") == 0 ||
      strcmp(icl_Functor(icl_term), "oaa") == 0) {
    if (icl_NumTerms(icl_term) == 1) {
      return icl_IsAtomStruct(icl_NthTerm(icl_term, 1), vars_allowed);
    } else {
      pce_error("iris and oaa predicates take one argument");
      return false;
    }
  } else {
    for (i = 0; i < icl_NumTerms(icl_term); i++) {
      icl_arg = icl_NthTerm(icl_term, i+1);
      if (icl_IsVar(icl_arg)) {
	if (!vars_allowed) {
	  icl_string = icl_NewStringFromTerm(icl_arg);
	  pce_error("Variable %s is not allowed here", icl_string);
	  icl_stFree(icl_string);
	  return false;
	}
      } else if (!icl_IsStr(icl_arg)) {
	icl_string = icl_NewStringFromTerm(icl_arg);
	pce_error("%s is not a constant or variable", icl_string);
	icl_stFree(icl_string);
	return false;
      }
    }
  }
  return true;
}

bool pce_add_const(char *name, int32_t sort_index, samp_table_t *table) {
  int32_t stat, cidx;

  stat = add_const_internal(name, sort_index, table);
  if (stat == -1) {
    pce_error("Error adding constant %s", name);
    return false;
  }
  if (stat == 1) {
    return true;
  }
  cidx = const_index(name, &table->const_table);
  create_new_const_rule_instances(cidx, table, false, 0);
  create_new_const_query_instances(cidx, table, false, 0);
  return true;
}

// Returns atom or NULL
input_atom_t *icl_term_to_atom(ICLTerm *icl_atom, bool vars_allowed,
			       bool indirect_allowed) {
  pred_table_t *pred_table;
  ICLTerm *icl_arg;
  char *pred;
  int32_t pred_val, pred_idx;
  int32_t i, num_args;
  int32_t *psig;
  input_atom_t *atom;
  
  if (!icl_IsAtomStruct(icl_atom, vars_allowed)) {
    return NULL;
  }
  pred_table = &(samp_table.pred_table);
  pred = icl_Functor(icl_atom);
  num_args = icl_NumTerms(icl_atom);
  // check if we've seen the predicate before and complain if not.
  pred_val = pred_index(pred, pred_table);
  if (pred_val == -1) {
    pce_error("Predicate %s not declared", pred);
    return NULL;
  }
  // Check that pred is direct, and arities match
  pred_idx = pred_val_to_index(pred_val);
  if (!indirect_allowed && pred_idx >= 0) {
    pce_error("Indirect predicate %s used", pred);
    return NULL;
  }
  // Check the arity
  if (pred_arity(pred_idx, pred_table) != num_args) {
    pce_error("Wrong number of args");
    return NULL;
  }
  psig = pred_signature(pred_idx, pred_table);
  for(i=0; i < num_args; i++) {
    icl_arg = icl_NthTerm(icl_atom, i+1);
    names_buffer.name[i] = icl_Str(icl_arg);
    // Add the constant, of sort corresponding to the pred
    pce_add_const(names_buffer.name[i], psig[i], &samp_table);
    //add_const_internal(names_buffer.name[i], psig[i], &samp_table);
  }
  // We have the pred and args - now create an atom
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  atom->pred = pred;
  atom->args = names_buffer_copy(&names_buffer);
  return atom;
}  
  

// This is a plugin compatible version of PCE.  The primary differences
// between this and the earlier PCE, aside from Prolog vs C, is that the new
// PCE has sorts, and that predicates are "direct" or "indirect".  For
// sorts, we simply create a universal sort "U".  Predicates are more
// difficult.  We analyze the facts and assertions to determine the type of
// predicate, which is determined only by ground assertions and facts.  A
// predicate in a pce_fact is treated as direct, unless it also occurs in a
// ground pce_learner_assert.  As these can come in any order, we actually
// wait until a pce_query comes in, then assert all predicates, run MCSAT,
// and answer the query.  After that we run according to the idle loop.
// Thus until we answer the query we simply build declarations and save them.

/* Memory for pce_fact_args */

bool pce_fact(ICLTerm *goal) {
  // The "goal" struct is in the form "pce_fact(Source,Formula)".
  pred_table_t *pred_table = &(samp_table.pred_table);
  ICLTerm *Source = icl_CopyTerm(icl_NthTerm(goal, 1));
  ICLTerm *Formula = icl_CopyTerm(icl_NthTerm(goal, 2));
  ICLTerm *Arg;
  char *source, *iclstr;
  input_atom_t *atom;
  char *pred;
  int32_t pred_val, pred_idx;
  int32_t i, num_args;
  int32_t *psig;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);
  
  if (!icl_IsStr(Source)) {
    pce_error("pce_fact: Source should be a string");
    return false;
  }
  if (!icl_IsStruct(Formula)) {
    pce_error("pce_fact must be an atom");
  }
  source = icl_Str(Source);
  pred = icl_Functor(Formula);
  num_args = icl_NumTerms(Formula);
  // check if we've seen the predicate before and complain if not.
  pred_val = pred_index(pred, pred_table);
  if (pred_val == -1) {
    pce_error("Predicate %s not declared", pred);
    return false;
  }
  // Check that pred is direct, and arities match
  pred_idx = pred_val_to_index(pred_val);
  if (pred_idx < 0) {
    // It is direct - check the arity
    if (pred_arity(pred_idx, pred_table) != num_args) {
      pce_error("Wrong number of args");
      return false;
    }
  } else {
    pce_error("Indirect predicate used in pce_fact");
    return false;
  }
  psig = pred_signature(pred_idx, pred_table);
  for(i=0; i < num_args; i++) {
    Arg = icl_NthTerm(Formula, i+1);
    if(!icl_IsStr(Arg)) {
      // Each Arg must be a string, not a Struct or Var.
      pce_error("Args to Formula must be simple strings, not Variables");
      return false;
    }
    names_buffer_resize(&names_buffer);
    names_buffer.name[i] = icl_Str(Arg);
    // Add the constant, of sort corresponding to the pred
    pce_add_const(names_buffer.name[i], psig[i], &samp_table);
    //add_const_internal(names_buffer.name[i], psig[i], &samp_table);
  }
  names_buffer.size = num_args;
  // We have the pred and args - now create an atom
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  atom->pred = pred;
  atom->args = names_buffer_copy(&names_buffer);
  while (num_args > names_buffer.capacity) {
    names_buffer_resize(&names_buffer);
  }
  // Everything is OK, create the atom and assert it
  assert_atom(&samp_table, atom);
  //dump_sort_table(&samp_table);
  //dump_atom_table(&samp_table);
  //free_atom(&atom);
  return true;
}

/* Callback for pce_fact
   
 * This takes a Source (i.e., learner id), and a Formula, and adds the
 * formula as a fact.  The Source is currently ignored.  The Formula must be
 * an atom.  The predicate must have been previously declared (generally in
 * pce-init.pce), and its sorts are used to declare any new constants.
 */
int pce_fact_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  if (pce_fact(goal)) {
    pce_save(goal);
    return true;
  } else {
    return false;
  }
}

bool eql_samp_atom(samp_atom_t *atom1, samp_atom_t *atom2,
		   samp_table_t *table) {
  int32_t i, arity;

  if (atom1->pred != atom2->pred) {
    return false;
  }
  arity = pred_arity(atom1->pred, &table->pred_table);
  for (i = 0; i < arity; i++) {
    if (atom1->args[i] != atom2->args[i]) {
      return false;
    }
  }
  return true;
}

bool eql_rule_literal(rule_literal_t *lit1, rule_literal_t *lit2,
		      samp_table_t *table) {
  return ((lit1->neg == lit2->neg)
	  && eql_samp_atom(lit1->atom, lit2->atom, table));
}

// Checks if the queries are the same - this is essentially a syntactic
// test, though it won't care about variable names.
bool eql_query_entries(rule_literal_t ***lits, samp_query_t *query,
		       samp_table_t *table) {
  int32_t i, j;
  
  for (i = 0; i < query->num_clauses; i++) {
    if (lits[i] == NULL) {
      return false;
    }
    for (j = 0; query->literals[i][j] != NULL; j++) {
      if (lits[i][j] == NULL) {
	return false;
      }
      if (! eql_rule_literal(query->literals[i][j], lits[i][j], table)) {
	return false;
      }
    }
    // Check if lits[i] has more elements 
    if (lits[i][j] != NULL) {
      return false;
    }
  }
  return (lits[i] == NULL);
}

      
int32_t add_to_query_table(rule_vars_buffer_t *vars, rule_literal_t ***lits,
			   samp_table_t *table) {
  query_table_t *query_table = &table->query_table;
  samp_query_t *query;
  int32_t i, query_index;
  
  for (i = 0; i < query_table->num_queries; i++) {
    if (eql_query_entries(lits, query_table->query[i], &samp_table)) {
      return i;
    }
  }
  // Now save the query in the query_table
  query_index = query_table->num_queries;
  query_table_resize(query_table);
  query = (samp_query_t *) safe_malloc(sizeof(samp_query_t));
  query_table->query[query_table->num_queries++] = query;
  for (i = 0; lits[i] != NULL; i++) {}
  query->num_clauses = i;
  query->num_vars = rules_vars_buffer.size;
  query->literals = lits;
  query->vars = rule_vars_buffer_copy(&rules_vars_buffer);
  query->source_index = NULL;

  if (lazy_pce) {
    // Need to create instances based on the atom table
    all_lazy_query_instances(query, &samp_table);
  } else {
    // Need to create all instances and add to query_instance_table
    all_query_instances(query, &samp_table);
  }
  return query_index;
}
    

// The literals of the clause have samp_atom's whose args are either const
// indices, or (if negative) indices to the rules_vars_buffer.  We collect
// the referenced names, and create the vars entry accordingly, then reset
// the index to point to the new var location.  We use a separate
// buffer for this; the rule_vars_buffer.

static void set_clause_variables(samp_rule_t *clause) {
  pred_table_t *pred_table;
  int32_t i, j, k, arity, vidx, nidx;
  samp_atom_t *atom;

  pred_table = &(samp_table.pred_table);
  rule_vars_buffer.size = 0;
  for (i = 0; i < clause->num_lits; i++) {
    atom = clause->literals[i]->atom;
    arity = pred_arity(atom->pred, pred_table);
    // Loop through the args of the atom
    for (j = 0; j < arity; j++) {
      if (atom->args[j] < 0) {
	// We have a variable - vidx is the index to rules_vars_buffer
	vidx = -(atom->args[j] + 1);
	// Now see if we have already seen this variable in this clause
	for (k = 0; k < rule_vars_buffer.size; k++) {
	  if (strcmp(rules_vars_buffer.vars[vidx]->name,
		     rule_vars_buffer.vars[k]->name) == 0) {
	    break;
	  }
	}
	if (k == rule_vars_buffer.size) {
	  rule_vars_buffer_resize(&rule_vars_buffer);
	  nidx = rule_vars_buffer.size;
	  rule_vars_buffer.vars[nidx] = rules_vars_buffer.vars[vidx];
	  atom->args[j] = -(nidx + 1);
	  rule_vars_buffer.size += 1;
	} else {
	  // Already have this var, just reset the index
	  atom->args[j] = -(k + 1);
	}
      }
    }
  }
  clause->num_vars = rule_vars_buffer.size;
  clause->vars = rule_vars_buffer_copy(&rule_vars_buffer);
}

static rules_buffer_t rules_buffer = {0, 0, NULL};

static void rules_buffer_resize(rules_buffer_t *buf) {
  if (buf->rules == NULL) {
    buf->rules = (samp_rule_t **)
      safe_malloc(INIT_RULES_BUFFER_SIZE * sizeof(samp_rule_t *));
    buf->capacity = INIT_RULES_BUFFER_SIZE;
  }
  int32_t capacity = buf->capacity;
  int32_t size = buf->size;
  if (capacity < size + 1){
    if (MAXSIZE(sizeof(samp_rule_t *), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    capacity += capacity/2;
    buf->rules = (samp_rule_t **)
      safe_realloc(buf->rules, capacity * sizeof(samp_rule_t *));
    buf->capacity = capacity;
  }
}

samp_rule_t *samp_rule_copy(samp_rule_t *rule) {
  samp_rule_t *copy;
  
  copy = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
  copy->num_lits = rule->num_lits;
  copy->num_vars = rule->num_vars;
  copy->vars = rule->vars;
  copy->literals = rule->literals;
  copy->weight = rule->weight;
  return copy;
}

samp_rule_t **rules_buffer_copy(rules_buffer_t *buf) {
  samp_rule_t **copy;
  int32_t i;

  if (buf->size == 0) {
    return NULL;
  }
  copy = (samp_rule_t **) safe_malloc((buf->size + 1) * sizeof(samp_rule_t *));
  for (i = 0; i < buf->size; i++) {
    copy[i] = samp_rule_copy(buf->rules[i]);
  }
  copy[i] = NULL;
  return copy;
}


// The main body of pce_learner_assert

void pce_install_rule(ICLTerm *Formula, double weight) {
  rule_table_t *rule_table = &(samp_table.rule_table);
  pred_table_t *pred_table = &(samp_table.pred_table);
  samp_rule_t *clause, **clauses;
  rule_literal_t *lit, ***lits;
  int32_t i, j, litlen, atom_idx;
  char *str;
  
  // cnf has side effect of setting rules_vars_buffer
  lits = cnf(Formula);

  if (lits == NULL) {
    str = icl_NewStringFromTerm(Formula);
    pce_error("Could not install rule:\n  %s\n", str);
    icl_stFree(str);
    return;
  }
  rules_buffer.size = 0;
  for (i = 0; lits[i] != NULL; i++) {
    clause = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
    for (litlen = 0; lits[i][litlen] != NULL; litlen++) {}
    clause->num_lits = litlen;
    clause->literals = lits[i];
    set_clause_variables(clause);
    rules_buffer_resize(&rules_buffer);
    rules_buffer.rules[rules_buffer.size++] = clause;
  }
  safe_free(lits);
  
  // Make a NULL-terminated copy
  clauses = rules_buffer_copy(&rules_buffer);

  for (i = 0; clauses[i] != NULL; i++) {
    if (clauses[i]->num_vars == 0) {
      clause_buffer_resize(clauses[i]->num_lits);
      for (j = 0; j < clauses[i]->num_lits; j++) {
	lit = clauses[i]->literals[j];
	atom_idx = add_internal_atom(&samp_table, lit->atom);
	clause_buffer.data[j] = lit->neg ? neg_lit(atom_idx) : pos_lit(atom_idx);
      }
      add_internal_clause(&samp_table, clause_buffer.data,
			  clauses[i]->num_lits, weight);
      safe_free(clauses[i]);
    } else {
      int32_t current_rule = rule_table->num_rules;
      samp_rule_t *new_rule = clauses[i];
      new_rule->weight = weight;
      rule_table_resize(rule_table);
      rule_table->samp_rules[current_rule] = new_rule;
      rule_table->num_rules += 1;
      // Now loop through the literals, adding the current rule to each pred
      for (j=0; j<new_rule->num_lits; j++) {
	int32_t pred = new_rule->literals[j]->atom->pred;
	add_rule_to_pred(pred_table, pred, current_rule);
      }
      // Create instances here rather than add_rule, as this is eager
      if (!lazy_pce) {
	all_rule_instances(current_rule, &samp_table);
      }
    }
  }
  cprintf(2, "Processed %d clauses\n", i);

  
  safe_free(clauses);
  //dump_clause_table(&samp_table);
  //dump_rule_table(&samp_table);
}

bool pce_learner_assert_internal(char *lid, ICLTerm *Formula, ICLTerm *Weight) {
  double weight;

  // First check that the args are OK
  if (!icl_IsFloat(Weight)) {
    pce_error("pce_learner_assert: Weight should be a float");
    return false;
  }
  if (!icl_IsStruct(Formula)) {
    pce_error("pce_learner_assert: Formula should be a struct");
    return false;
  }
  weight = icl_Float(Weight);
  pce_install_rule(Formula, weight);
  return true;
}

bool pce_learner_assert(ICLTerm *goal) {
    ICLTerm *Lid, *Formula, *Weight;
  char *lid, *iclstr;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);

  Lid = icl_CopyTerm(icl_NthTerm(goal, 1));
  Formula = icl_CopyTerm(icl_NthTerm(goal, 2));
  Weight = icl_CopyTerm(icl_NthTerm(goal, 3));
  if (!icl_IsStr(Lid)) {
    pce_error("pce_learner_assert: Lid should be a string");
    return false;
  }
  lid = icl_Str(Lid);
  if (pce_learner_assert_internal(lid, Formula, Weight)) {
    pce_save(goal);
    return true;
  } else {
    return false;
  }
}

// goal of form: pce_learner_assert(Lid,Formula,Weight)
//
// Similar to pce_fact, except the formula may be an arbitrary formula that
// must be converted to clausal form.  Rules will have variables which are
// names starting with a capital letter (prolog ocnvention).  New predicates
// will be treated as indirect, and new constants will be added.  After
// generating the clause, we call add_clause or add_rule, depending on
// whether there are variables.
//
int pce_learner_assert_callback(ICLTerm *goal, ICLTerm *params,
				ICLTerm *solutions) {
  return pce_learner_assert(goal);
}


bool pce_learner_assert_list(ICLTerm *goal) {
  ICLTerm *Elt, *List, *Lid, *Formula, *Weight;
  int32_t i;
  char *iclstr, *lid;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);

  Lid = icl_NthTerm(goal, 1);
  if (!icl_IsStr(Lid)) {
    pce_error("pce_learner_assert: Lid should be a string");
    return false;
  }
  lid = icl_Str(Lid);
  List = icl_NthTerm(goal, 2);
  if (! icl_IsList(List)) {
    pce_error("pce_learner_assert_list: List expected");
    return false;
  }
  for(i = 1; i <= icl_NumTerms(List); i++) {
    Elt = icl_NthTerm(List, i);
    if (!icl_IsStruct(Elt)) {
      pce_error("pce_learner_assert_list: List element should be a struct");
      return false;
    }
    if (icl_NumTerms(Elt) != 2) {
      pce_error("pce_learner_assert_list: List element should have two args");
      return false;
    }
    Formula = icl_NthTerm(Elt, 1);
    Weight = icl_NthTerm(Elt, 2);
    if (!pce_learner_assert_internal(lid, Formula, Weight)) {
      return false;
    }
  }
  pce_log("pce_learner_assert_list: processed %d assertions\n",
	  icl_NumTerms(List));
  printf("pce_learner_assert_list: processed %d assertions\n",
	 icl_NumTerms(List));
  return true;
}  

// goal of form: pce_learner_assert_list(Lid,List)
//
// Asserts the elements of the List, which each have the form
// (Formula, Weight)
//
int pce_learner_assert_list_callback(ICLTerm *goal, ICLTerm *params,
				     ICLTerm *solutions) {
  if (pce_learner_assert_list(goal)) {
    pce_save(goal);
    return true;
  } else {
    return false;
  }
}

// Substitutes constants for variables, using the subst field of qinst
ICLTerm *pce_substit(ICLTerm *Term, samp_query_instance_t *qinst) {
  query_table_t *query_table;
  samp_query_t *query;
  const_table_t *const_table;
  char *cname;
  ICLTerm *UTerm;
  int32_t i;
  
  if(icl_IsVar(Term)) {
    query_table = &samp_table.query_table;
    const_table = &samp_table.const_table;
    query = query_table->query[qinst->query_index];
    for (i = 0; i < query->num_vars; i++) {
      if (strcmp(query->vars[i]->name, icl_Str(Term)) == 0) {
	// i is now the index into qinst->subst to find the const
	cname = str_copy(const_name(qinst->subst[i], const_table));
	return icl_NewStr(cname);
      }
    }
    pce_error("pce_subst: Constant not found");
    return NULL;
  }
  if(!(icl_IsList(Term) || icl_IsGroup(Term) || icl_IsStruct(Term))) {
    return Term;
  }
  for(i = 0; i < icl_NumTerms(Term); i++) {
    UTerm = pce_substit(icl_NthTerm(Term, i+1), qinst);
    if (icl_NthTerm(Term, i+1) != UTerm) {
      // Note that icl_NthTerm is 1-based, icl_ReplaceElement is 0-based
      icl_ReplaceElement(Term, i, UTerm, false);
    }
  }
  return Term;
}

void add_query_instance_to_solution(ICLTerm *solutions,
				    ICLTerm *Formula,
				    samp_query_instance_t *qinst,
				    bool is_subscription) {
  ICLTerm *solution, *InstForm, *Prob;
  double prob;
  
  InstForm = icl_CopyTerm(Formula);
  InstForm = pce_substit(InstForm, qinst);
  prob = query_probability(qinst, &samp_table);
  
  Prob = icl_NewFloat(prob);
  if (is_subscription) {
    solution = icl_NewStruct("m", 2, InstForm, Prob);
  } else {
    solution = icl_NewStruct("pce_queryp", 2, InstForm, Prob);
  }
  icl_AddToList(solutions, solution, true);
}


// Goal of the form queryp(Q, P). QueryFormula is a general formula - if it
// is not in the formula table then samples must be generated to get a
// probability.
int pce_queryp_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  atom_table_t *atom_table = &samp_table.atom_table;
  query_instance_table_t *query_instance_table = &samp_table.query_instance_table;
  ICLTerm *QueryFormula = icl_CopyTerm(icl_NthTerm(goal, 1));
  ICLTerm *Probability = icl_CopyTerm(icl_NthTerm(goal, 2));
  int32_t i, query_index;
  rule_literal_t ***lits;
  bool result;
  char *iclstr;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);

  if (!icl_IsVar(Probability)) {
    pce_error("pce_queryp: Probability must be a variable");
    return false;
  }
  if (!icl_IsStruct(QueryFormula)) {
    pce_error("pce_queryp: QueryFormula must be an atom");
    return false;
  }
  
  // cnf has side effect of setting rules_vars_buffer
  lits = cnf(QueryFormula);
  query_index = add_to_query_table(&rules_vars_buffer, lits, &samp_table);
  result = false;

  for (i = 0; i < query_instance_table->num_queries; i++) {
    if (query_instance_table->query_inst[i]->query_index == query_index) {
      result = true;
      if (query_instance_table->query_inst[i]->sampling_num
	  >= atom_table->num_samples) {
	// Need to generate some samples
	if (lazy_pce) {
	  lazy_mc_sat(&samp_table, DEFAULT_SA_PROBABILITY,
		      DEFAULT_SAMP_TEMPERATURE, DEFAULT_RVAR_PROBABILITY,
		      DEFAULT_MAX_FLIPS, DEFAULT_MAX_EXTRA_FLIPS,
		      DEFAULT_MAX_SAMPLES);
	} else {
	  mc_sat(&samp_table, DEFAULT_SA_PROBABILITY, DEFAULT_SAMP_TEMPERATURE,
		 DEFAULT_RVAR_PROBABILITY, DEFAULT_MAX_FLIPS,
		 DEFAULT_MAX_EXTRA_FLIPS, DEFAULT_MAX_SAMPLES);
	}
      }
      // Now add to query answer
      add_query_instance_to_solution(solutions, QueryFormula,
				     query_instance_table->query_inst[i],
				     false);
    }
  }
  // Queries are not saved
  // pce_save(goal);
  iclstr = icl_NewStringFromTerm(solutions);
  icl_stFree(iclstr);
  return result;
}

// The goal is either of the forms
//   pce_subscribe(Lid,Formula,Id)
//   pce_subscribe(Lid,Formula,Who,Condition,Id)
// Lid = learner ID
// Formula = a formula (atom?)
// Who = not currently used in Prolog PCE - one of 'all' or 'notme'
// Condition = somehow flags whether the formula is a negative example
//             possible values:
//               'in_to_out' - F is negative
//               'out_to_in' - F is positive
// Id = the subscription id - currently just uses the subscription counter
//
// For now, Who and Condition will be ignored, and Id is expected to be a
// variable, which will be the value returned.

static subscription_buffer_t subscriptions = {0, 0, NULL};

static void subscriptions_resize() {
  int32_t i, prevcap;
  
  if (subscriptions.subscription == NULL) {
    subscriptions.subscription = (subscription_t **)
      safe_malloc(INIT_SUBSCRIPTIONS_SIZE * sizeof(subscription_t *));
    subscriptions.capacity = INIT_SUBSCRIPTIONS_SIZE;
    for (i = 0; i < subscriptions.capacity; i++) {
      subscriptions.subscription[i] =
	(subscription_t *) safe_malloc(sizeof(subscription_t));
    }
  }
  int32_t capacity = subscriptions.capacity;
  int32_t size = subscriptions.size;
  if (capacity < size + 1){
    if (MAXSIZE(sizeof(subscription_t), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    prevcap = capacity;
    capacity += capacity/2;
    subscriptions.subscription = (subscription_t **)
      safe_realloc(subscriptions.subscription,
		   capacity * sizeof(subscription_t *));
    for (i = prevcap; i < capacity; i++) {
      subscriptions.subscription[i] =
	(subscription_t *) safe_malloc(sizeof(subscription_t));
    }
    subscriptions.capacity = capacity;
  }
}

int32_t add_learner_name(char *lid) {
  int32_t i, size;

  size = learner_ids.size;
  for (i = 0; i < size; i++) {
    if (strcmp(learner_ids.name[i], lid) == 0) {
      return i;
    }
  }
  names_buffer_resize(&learner_ids);
  learner_ids.name[learner_ids.size++] = str_copy(lid);
  return size;
}

void add_subscription_source_to_query(int32_t query_index,
				      int32_t subscr_index) {
  samp_query_t *query;
  int32_t i;

  query = samp_table.query_table.query[query_index];
  if (query->source_index == NULL) {
    query->source_index = (int32_t *) safe_malloc(2 * sizeof(int32_t));
    query->source_index[0] = subscr_index;
    query->source_index[1] = -1;
  } else {
    for (i = 0; query->source_index[i] != -1; i++) {}
    query->source_index = (int32_t *)
      safe_realloc(query->source_index, (i + 1) * sizeof(int32_t));
    query->source_index[i] = subscr_index;
    query->source_index[i+1] = -1;
  }
}


int pce_subscribe_callback(ICLTerm *goal, ICLTerm *params,
			       ICLTerm *solutions) {
  ICLTerm *Lid, *Formula, *Who, *Condition, *Id, *Sid;
  rule_literal_t ***lits;
  char *sid, *lid, idstr[20];
  ICLTerm *solution;
  subscription_t *subscr;
  int32_t i, query_index, subscr_index;
  char *iclstr;

  Lid       = NULL;
  Who       = NULL;
  Condition = NULL;
  Formula   = NULL;
  Id        = NULL;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);

  if (icl_NumTerms(goal) == 3) {
    Lid       = icl_CopyTerm(icl_NthTerm(goal, 1));
    Formula   = icl_CopyTerm(icl_NthTerm(goal, 2));
    Id        = icl_CopyTerm(icl_NthTerm(goal, 3));
  } else if (icl_NumTerms(goal) == 5) {
    Lid       = icl_CopyTerm(icl_NthTerm(goal, 1));
    Formula   = icl_CopyTerm(icl_NthTerm(goal, 2));
    Who       = icl_CopyTerm(icl_NthTerm(goal, 3));
    Condition = icl_CopyTerm(icl_NthTerm(goal, 4));
    Id        = icl_CopyTerm(icl_NthTerm(goal, 5));
  } else {
    pce_error("pce_subscribe: Bad form");
  }
  if (!icl_IsStr(Lid)) {
    pce_error("pce_subscribe: Lid should be a string");
    return false;
  }
  lid = icl_Str(Lid);
  // sid / Sid is the subscription id - derived from Id if not a var
  if (!icl_IsVar(Id)) {
    // See if we have this subscription id already
    sid = str_copy(icl_Str(Id));
    for (i = 0; i < subscriptions.size; i++) {
      if (strcmp(subscriptions.subscription[i]->id, sid) == 0) {
	safe_free(sid);
	pce_error("pce_subscribe: Id is already in use");
	return false;
      }
    }
    Sid = Id;
  } else {
    // TBD - See if we already have a subscription for this formula
    sprintf(idstr, "%"PRIu32, subscriptions.size);
    sid = str_copy(idstr);
    Sid = icl_NewStr(sid);
  }
  // Convert the Formula to rule_literal_t's - side effects rules_vars_buffer
  lits = cnf(Formula);
  if (lits == NULL) {
    return 1;
  }

  // Update subscription buffer
  subscr = (subscription_t *) safe_malloc(sizeof(subscription_t));
  subscr->lid = add_learner_name(lid);
  subscr->id = sid;
  subscr->formula = icl_CopyTerm(Formula);
  subscriptions_resize();
  subscr_index = subscriptions.size;
  subscriptions.subscription[subscriptions.size++] = subscr;

  // Add to query_table
  query_index = add_to_query_table(&rules_vars_buffer, lits, &samp_table);

  // Now have the query and subscription point to each other
  subscr->query_index = query_index;
  add_subscription_source_to_query(query_index, subscr_index);

  // Instantiate with the (possibly new) Sid
  if (icl_NumTerms(goal) == 3) {
    solution = icl_NewStruct("pce_subscribe", 3, Lid, Formula, Sid);
  } else {
    solution = icl_NewStruct("pce_subscribe", 5,
			     Lid, Formula, Who, Condition, Sid);
  }
  icl_AddToList(solutions, solution, true);
  // Currently not saved - if we do save subscriptions, how do we get the
  // saved subscription ID back to the original subscriber?
  // pce_save(goal);
  return true;
}

static ICLTerm *pce_SubscriptionParams = NULL;

void process_subscription(subscription_t *subscription) {
  atom_table_t *atom_table;
  query_instance_table_t *query_instance_table;
  samp_query_instance_t *qinst;
  query_table_t *query_table;
  int32_t i, cnt;
  ICLTerm *callback, *callbacklist, *Out, *Sol;

  if (pce_SubscriptionParams == NULL) {
    pce_SubscriptionParams =
      icl_NewTermFromData("[blocking(false),reply(none)]", 29);
  }

  callbacklist = icl_NewList(NULL); 
  atom_table = &samp_table.atom_table;
  query_instance_table = &samp_table.query_instance_table;
  query_table = &samp_table.query_table;
  
  // Find all query instances for the given subscription, and add them to
  // the solution.
  cnt = 0;
  for (i = 0; i < query_instance_table->num_queries; i++) {
    qinst = query_instance_table->query_inst[i];
    if (subscription->query_index == qinst->query_index) {
      cnt += 1;
      if (query_instance_table->query_inst[i]->sampling_num
	  >= atom_table->num_samples) {
	// Need to generate some samples
	mc_sat(&samp_table, DEFAULT_SA_PROBABILITY, DEFAULT_SAMP_TEMPERATURE,
	   DEFAULT_RVAR_PROBABILITY, DEFAULT_MAX_FLIPS,
	   DEFAULT_MAX_EXTRA_FLIPS, 50);
      }
      // Now add to query answer
      add_query_instance_to_solution(callbacklist, subscription->formula,
				     query_instance_table->query_inst[i], true);
    }
  }
  // Need to call oaa_Solve with solution a list of atoms of the form
  // pce_subscription_callback(m(Sid, Formula, Prob), ...)
  printf("process_subscription: returning %"PRId32" solutions for subscription %s\n",
	 cnt, subscription->id);
  fflush(stdout);
  callback = icl_NewStruct("pce_subscription_callback", 1, callbacklist);
  Out = NULL;
  Sol = NULL;
  oaa_Solve(callback, pce_SubscriptionParams, &Out, &Sol);
}

// Goal is just a variable
int pce_process_subscriptions_callback(ICLTerm *goal, ICLTerm *params,
				       ICLTerm *solutions) {
  int32_t i;
  char *iclstr;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);

  for (i = 0; i < subscriptions.size; i++) {
    process_subscription(subscriptions.subscription[i]);
  }
  // pce_save(goal);
  printf("pce_process_subscriptions_callback called\n");
  fflush(stdout);  
  return true;
}

void free_subscription(subscription_t *subscr) {
  safe_free(subscr->id);
  safe_free(subscr);
}

int32_t get_learner_index(char *lid) {
  int32_t i;
  for (i = 0; i < learner_ids.size; i++) {
    if (strcmp(learner_ids.name[i], lid) == 0) {
      return i;
    }
  }
  return -1;
}

void remove_query(int32_t query_index, samp_table_t *table) {
  query_table_t *query_table;
  query_instance_table_t *qinst_table;
  samp_query_t *query;
  samp_query_instance_t *qinst;
  int32_t i, j, cnt;

  query_table = &table->query_table;
  qinst_table = &table->query_instance_table;
  query = query_table->query[query_index];

  // First find and free all instances of this query

  cnt = 0;
  for (i = 0; i < qinst_table->size; i++) {
    if (qinst_table->query_inst[i]->query_index == query_index) {
      qinst = qinst_table->query_inst[i];
      safe_free(qinst->subst);
      for (j = 0; qinst->lit[j] != NULL; j++) {
	safe_free(qinst->lit[j]);
      }
      safe_free(qinst->lit);
      safe_free(qinst);
      cnt++;
    } else if (cnt > 0) {
      // move this pointer down to the next available slot in the array,
      // past the cnt empty slots
      qinst_table->query_inst[i - cnt] = qinst_table->query_inst[i];
    }
  }
  qinst_table->size -= cnt;
}

bool pce_unsubscribe(int32_t subscr_index) {
  subscription_t *subscr;
  int32_t i, j, k, qidx, si_size;
  samp_query_t *query;
  
  subscr = subscriptions.subscription[subscr_index];
  qidx = subscr->query_index;
  query = samp_table.query_table.query[qidx];
  // The source_index is an array of subscription indices
  // terminated with -1
  for (si_size = 0; query->source_index[si_size] != -1; si_size++) {}
  for (j = 0; j < si_size; j++) {
    if (query->source_index[j] == subscr->lid) {
      break;
    }
  }
  if (j == si_size) {
    pce_error("pce_unsubscribe: something's wrong - couldn't find index");
    return false;
  }
  if (si_size == 1) {
    // This query is only pointed to by this subscription - can
    // remove the query as well as the subscription
    remove_query(qidx, &samp_table);
  } else {
    // Simply remove this source index from the array
    for (k = j; k < si_size; k++) {
      query->source_index[k] = query->source_index[j+1];
    }
    query->source_index = (int32_t *)
      safe_realloc(query->source_index,
		   (si_size - 1) * sizeof(int32_t));
  }
  // Now remove the subscription
  for (i = subscr_index; i < subscriptions.size; i++) {
    subscriptions.subscription[i] = subscriptions.subscription[i+1];
  }
  subscriptions.size--;
  free_subscription(subscr);
  return true;
}

// Goal is in one of 3 forms - the arity tells them apart
//  pce_unsubscribe(Lid,F)
//  pce_unsubscribe(Lid,F,Who,Condition)
//  pce_unsubscribe(SubscriptionId)

int pce_unsubscribe_callback(ICLTerm *goal, ICLTerm *params,
				 ICLTerm *solutions) {
  ICLTerm *Lid, *Formula, *Who, *Condition, *Sid;
  char *sid, *lid;
  subscription_t *subscr;
  int32_t lidx, subscr_index;
  char *iclstr;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);
  Lid = NULL;
  Who = NULL;
  Sid = NULL;
  Formula = NULL;
  if (icl_NumTerms(goal) == 2) {
    Lid       = icl_CopyTerm(icl_NthTerm(goal, 1));
    Formula   = icl_CopyTerm(icl_NthTerm(goal, 2));
  } else if (icl_NumTerms(goal) == 4) {
    Lid       = icl_CopyTerm(icl_NthTerm(goal, 1));
    Formula   = icl_CopyTerm(icl_NthTerm(goal, 2));
    Who       = icl_CopyTerm(icl_NthTerm(goal, 3));
    Condition = icl_CopyTerm(icl_NthTerm(goal, 4));
  } else if (icl_NumTerms(goal) == 1) {
    Sid       = icl_CopyTerm(icl_NthTerm(goal, 1));
  } else {
    pce_error("pce_unsubscribe: Wrong number of arguments");
    return false;
  }
  if (Lid != NULL) {
    // Find subscription based on learner id and formula
    if (!icl_IsStr(Lid)) {
      pce_error("pce_subscribe: Lid should be a string");
      return false;
    }
    lid = icl_Str(Lid);
    // Currently ignoring Who and Condition
    lidx = get_learner_index(lid);
    if (lidx == -1) {
      pce_error("pce_unsubscribe: Learner %s not found", lid);
      return false;
    }
    for (subscr_index = 0;
	 subscr_index < subscriptions.size;
	 subscr_index++) {
      subscr = subscriptions.subscription[subscr_index];
      if (subscr->lid == lidx) {
	if (icl_match_terms(subscr->formula, Formula, NULL)) {
	  break;
	}
      }
    }
    if (subscr_index == subscriptions.size) {
      pce_error("pce_unsubscribe: subscription not found");
      return false;
    }
  } else {
    // Find subscription based on subscription ID
    if (!icl_IsStr(Sid)) {
      pce_error("pce_subscribe: Sid should be a string");
      return false;
    }
    sid = icl_Str(Sid);
    for (subscr_index = 0;
	 subscr_index < subscriptions.size;
	 subscr_index++) {
      if (strcmp(subscriptions.subscription[subscr_index]->id, sid) == 0) {
	break;
      }
    }
    if (subscr_index == subscriptions.size) {
      pce_error("pce_unsubscribe: subscription not found");
      return false;
    }
  }
  // Found a match - call pce_unsubscribe
  pce_unsubscribe(subscr_index);
  return true;
}

// goal of the form pce_unsubscribe_learner(Lid)
int pce_unsubscribe_learner_callback(ICLTerm *goal, ICLTerm *params,
                                         ICLTerm *solutions) {
  ICLTerm *Lid;
  char *lid;
  int32_t i, lidx, subscr_index;
  char *iclstr;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);
  Lid = icl_CopyTerm(icl_NthTerm(goal, 1));
  // Find subscriptions based on learner id
  if (!icl_IsStr(Lid)) {
    pce_error("pce_subscribe_learner: Lid should be a string");
    return false;
  }
  lid = icl_Str(Lid);
  lidx = get_learner_index(lid);
  if (lidx == -1) {
    pce_error("pce_unsubscribe_learner: Learner %s not found", lid);
    return false;
  }
  i = 0;
  for (subscr_index = 0;
       subscr_index < subscriptions.size;
       subscr_index++) {
    if (subscriptions.subscription[i]->lid == lidx) {
      if (pce_unsubscribe(subscr_index)) {
	i++;
      }
    }
  }
  printf("%"PRId32" subscriptions removed", i);
  pce_log("%"PRId32" subscriptions removed", i);
  return true;
}

pce_model_t pce_model = {0, 0, NULL};
icl_atom_t icl_atoms = {0, 0, NULL};

void pce_model_resize(pce_model_t *mod) {
  if (mod->atom == NULL) {
    mod->atom = (int32_t *)
      safe_malloc(INIT_PCE_MODEL_SIZE * sizeof(int32_t));
    mod->capacity = INIT_PCE_MODEL_SIZE;
  }
  int32_t capacity = mod->capacity;
  int32_t size = mod->size;
  if (capacity <= size + 1){
    if (MAXSIZE(sizeof(int32_t), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    capacity += capacity/2;
    mod->atom = (int32_t *)
      safe_realloc(mod->atom, capacity * sizeof(int32_t));
    mod->capacity = capacity;
  }
}

void icl_atoms_resize(int32_t nsize) {
  int32_t i;
  
  if (icl_atoms.size < nsize) {
    if (MAXSIZE(sizeof(ICLTerm *), 0) - icl_atoms.size <= nsize) {
      out_of_memory();
    }
    if (icl_atoms.size == 0) {
      icl_atoms.Atom = safe_malloc(nsize * sizeof(ICLTerm *));
      for (i = 0; i < nsize; i++) {
	icl_atoms.Atom[i] = NULL;
      }
    } else {
      icl_atoms.Atom = safe_realloc(icl_atoms.Atom, nsize * sizeof(ICLTerm *));
      for (i = icl_atoms.size; i < nsize; i++) {
	icl_atoms.Atom[i] = NULL;
      }
    }
    icl_atoms.size = nsize;
  }
}

ICLTerm *get_icl_atom(int32_t index) {
  atom_table_t *atom_table;
  pred_table_t *pred_table;
  const_table_t *const_table;
  samp_atom_t *atom;
  ICLTerm *Args;
  char *pred;
  int32_t i;

  atom_table = &samp_table.atom_table;
  pred_table = &samp_table.pred_table;
  const_table = &samp_table.const_table;
  if (icl_atoms.Atom[index] == NULL) {
    atom = atom_table->atom[index];
    pred = pred_name(atom->pred, pred_table);
    Args = icl_NewList(NULL);
    for (i = 0; i < pred_arity(atom->pred, pred_table); i++) {
      icl_AddToList(Args, icl_NewStr(const_name(atom->args[i], const_table)), true);
    }
    icl_atoms.Atom[index] = icl_NewStructFromList(pred, Args);
  }
  return icl_atoms.Atom[index];
}

double pce_model_threshold = .51;

int cmp_atom_prob(const void *a1, const void *a2) {
  double p1, p2;
  p1 = atom_probability(* (int32_t *) a1, &samp_table);
  p2 = atom_probability(* (int32_t *) a2, &samp_table);
  return p1 < p2 ? -1 : p1 > p2 ? 1 : 0;
}

// goal of the form pce_full_model(M)
int pce_full_model_callback(ICLTerm *goal, ICLTerm *params,
				ICLTerm *solutions) {
  atom_table_t *atom_table;
  pred_table_t *pred_table;
  const_table_t *const_table;
  ICLTerm *M, *Args;
  int32_t i, j, aidx;
  char *iclstr;
  samp_atom_t *atom;
  double prob;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);
  M = icl_NthTerm(goal, 1);
  atom_table = &samp_table.atom_table;
  pred_table = &samp_table.pred_table;
  const_table = &samp_table.const_table;
  for (i = 0; i < atom_table->num_vars; i++) {
    prob = atom_probability(i, &samp_table);
    if (prob > pce_model_threshold) {
      pce_model_resize(&pce_model);
      pce_model.atom[pce_model.size] = i;
      pce_model.size += 1;
    }
  }
  for (i = 0; i < pce_model.size; i++) {
    aidx = pce_model.atom[i];
    atom = atom_table->atom[aidx];
    Args = icl_NewList(NULL);
    for (j = 0; j < pred_arity(atom->pred, pred_table); j++) {
      icl_AddToList(Args, icl_NewStr(const_name(atom->args[j], const_table)),
		    true);
    }
    icl_AddToList(solutions,
		  icl_NewStructFromList(pred_name(atom->pred, pred_table),
					Args),
		  true);
  }
  return true;
}


int pce_update_model_callback(ICLTerm *goal, ICLTerm *params,
				  ICLTerm *solutions) {
  return true;
}
  
// Updates the model, and calls
// oaaSolve(pce_update_model_callback(Retract, BecameTrueWeights, Flag)
// where: Retract - a list of Atoms that went from true to false
//        BecameTrueWeights - a list of pairs (Atom, Wt) of atoms that became true
//                            along with their weights (we use p(Atom, Wt) here).
//        Flag - all or incremental
// The previous model is kept in the pce_model array, which is simply an array
// of atoms whose parobability is >= the threshold.
int32_t pce_update_model() {
  atom_table_t *atom_table;
  pred_table_t *pred_table;
  const_table_t *const_table;
  ICLTerm *Retract, *BecameTrueWeights, *Flag, *Pair, *Callback, *Out, *Sol;
  int32_t i, j, cur;
  double prob;
  bool was_true;

  atom_table = &samp_table.atom_table;
  pred_table = &samp_table.pred_table;
  const_table = &samp_table.const_table;
  icl_atoms_resize(atom_table->num_vars);
  Retract = icl_NewList(NULL);
  BecameTrueWeights = icl_NewList(NULL);
  Flag = pce_model.size == 0 ? icl_NewStr("full") : icl_NewStr("incremental");
  Out = NULL;
  Sol = NULL;
  for (i = 0; i < atom_table->num_vars; i++) {
    was_true = false;
    for (j = 0; j < pce_model.size; j++) {
      if (pce_model.atom[j] == i) {
	was_true = true;
	break;
      }
    }
    prob = atom_probability(i, &samp_table);
    if (!was_true && (prob >= pce_model_threshold)) {
      // Atom became true - add to model and to BecameTrueWeights list
      pce_model_resize(&pce_model);
      pce_model.atom[pce_model.size] = i;
      pce_model.size += 1;
      Pair = icl_NewStruct("p", 2, get_icl_atom(i), icl_NewFloat(prob));
      icl_AddToList(BecameTrueWeights, Pair, true);
    } else if (was_true && prob < pce_model_threshold) {
      // Atom became false - remove from model and add to Retract list
      pce_model.atom[j] = -1; // Just mark it for now.
      icl_AddToList(Retract, get_icl_atom(i), true);
    }
  }
  // Now remove marked retracts from the model
  cur = 0;
  for (i = 0; i < pce_model.size; i++) {
    if (pce_model.atom[i] != -1) {
      pce_model.atom[cur] = pce_model.atom[i];
      cur += 1;
    }
  }
  pce_model.size = cur;

  if (icl_ListLen(Retract) > 0 || icl_ListLen(BecameTrueWeights) > 0) {
    Callback = icl_NewStruct("pce_update_model_callback", 3, Retract, BecameTrueWeights, Flag);
    oaa_Solve(Callback, icl_NewList(NULL), &Out, &Sol);
    pce_log("pce_update_model_callback: Retract %d, BecameTrueWeights %d, Flag %s",
	    icl_ListLen(Retract), icl_ListLen(BecameTrueWeights), icl_Str(Flag));
    printf("pce_update_model_callback: Retract %d, BecameTrueWeights %d, Flag %s\n",
	   icl_ListLen(Retract), icl_ListLen(BecameTrueWeights), icl_Str(Flag));
  }
  return true;
}

char *pce_make_lname(ICLTerm *Username) {
  char *name;

  if (icl_IsStr(Username)) {
    // Prepend 'user:'
    name = safe_malloc((strlen(icl_Str(Username)) + 6) * sizeof(char));
    strcpy(name, "user:");
    strcat(name, icl_Str(Username));
    return name;
  } else {
    // PCE1 checked for a list of chars here - assume not needed.
    return "user_defined";
  }
}

double pce_translate_weight(ICLTerm *Strength) {
  ICLTerm *Arg;
  char *str;
  
  assert(strcmp(icl_Functor(Strength), "strength") == 0);
  Arg = icl_NthTerm(Strength, 1);
  if (icl_IsStr(Arg) && strcmp(icl_Str(Arg),"medium") == 0) {
    return 10.0;
  } else if (icl_IsStr(Arg) && strcmp(icl_Str(Arg),"high") == 0) {
    return 20.0;
  } else {
    str = icl_NewStringFromTerm(Strength);
    printf("* Note: %s defaulting to 5.0\n", str);
    icl_stFree(str);
    return 5.0;
  }
}

// From calo/apps/tester/src/prolog/calo_syntax.pl
// expansion_table('rdf', 'http://www.w3.org/1999/02/22-rdf-syntax-ns').
// expansion_table('rdfs', 'http://www.w3.org/2000/01/rdf-schema').
// expansion_table('xsd', 'http://www.w3.org/2001/XMLSchema').
// expansion_table('owl', 'http://www.w3.org/2002/07/owl').
// expansion_table('clib', 'http://calo.sri.com/core-plus-office').
// expansion_table('iris', 'http://iris.sri.com/irisOntology').
// expansion_table('msg', 'http://iris.sri.com/irisMessages').
// expansion_table('doc', 'http://iris.sri.com/doc').
// expansion_table('collab', 'http://calo.sri.com/collab').
// expansion_table('ui', 'http://iris.sri.com/ui').
// expansion_table('std', 'http://iris.sri.com/sparkTestData.owl').
// %% expansion_table('task':'http://iris.sri.com/abstractTask').
// %% Change for CALO Year 4:
// expansion_table('task', 'http://calo.sri.com/tasks').

char *uri_abbr[12] = {"rdf:", "rdfs:", "xsd:", "owl:", "clib:", "iris:",
		       "msg:", "doc:", "collab:", "ui:", "std:", "task:"};
char *uri_expn[12] = {"http://www.w3.org/1999/02/22-rdf-syntax-ns#",
		       "http://www.w3.org/2000/01/rdf-schema#",
		       "http://www.w3.org/2001/XMLSchema#",
		       "http://www.w3.org/2002/07/owl#",
		       "http://calo.sri.com/core-plus-office#",
		       "http://iris.sri.com/irisOntology#",
		       "http://iris.sri.com/irisMessages#",
		       "http://iris.sri.com/doc#",
		       "http://calo.sri.com/collab#",
		       "http://iris.sri.com/ui#",
		       "http://iris.sri.com/sparkTestData.owl#",
		       "http://calo.sri.com/tasks#"};

char *calo_string_expansion(char *str) {
  char *nstr;
  int32_t i, len;
  
  for (i = 0; i < 12; i++) {
    if (strncmp(str, uri_abbr[i], strlen(uri_abbr[i])) == 0) {
      len = strlen(str) - strlen(uri_abbr[i]) + strlen(uri_expn[i]) + 1;
      nstr = safe_malloc(len * sizeof(char));
      strcpy(nstr,uri_expn[i]);
      strcat(nstr,&str[strlen(uri_abbr[i])]);
      return nstr;
    }
  }
  return str;
}

ICLTerm *calo_expansion(ICLTerm *Atom) {
  char *str, *nstr;

  str = icl_Str(Atom);
  nstr = calo_string_expansion(str);
  if (str == nstr) {
    return Atom;
  } else {
    return icl_NewStr(nstr);
  }
}

// Mixing the following functions from prolog PCE:
// clean atom:
//   equ(A,B) ==> if A=B then true else A==B
//   nl_rule_expansion
//   'clib:'FOO(A,B) ==> iris(FOO(A,B))
//   ow calo_syntax:expand
// nl rule expansion:
//   equ(A,B) ==> A==B
//   related_to(A,B) ==> 'http://calo.sri.com/core-plus-office#relatedProjectIs'(A,B)
//   contains(A,B) ==> iris(fullTextSearch(A,B))
//   'iris:bp_email_subject'(A,B) ==> iris('iris:bp_email_subject'(A,B))
//   ow return Atom
// calo_syntax_expand:
//   don't recurse down iris(...) terms
//   translate abbrs to expanded URIs

ICLTerm *calo_syntax_expand(ICLTerm *Term) {
  ICLTerm *Nterm, *Arg, *Args;
  char *func, *nfunc;
  bool changed;
  int32_t i;
  
  // Recurse down the Term to atoms, then translate short forms to long forms
  // Don't go down 'iris(...)' terms
  if (icl_IsStruct(Term)) {
    func = icl_Functor(Term);
    if (strcmp(func,"iris") == 0) {
      return Term;
    }
    changed = false;
    nfunc = calo_string_expansion(func);
    Nterm = icl_CopyTerm(Term);
    if (nfunc != func) {
      Args = icl_NewList(NULL);
      for (i = 0; i < icl_NumTerms(Term); i++) {
	icl_AddToList(Args, icl_NthTerm(Term, i+1), true);
      }
      Nterm = icl_NewStructFromList(nfunc, Args);
      changed = true;
    }
    for (i = 0; i < icl_NumTerms(Nterm); i++) {
      Arg = calo_syntax_expand(icl_NthTerm(Nterm, i+1));
      if (icl_NthTerm(Nterm, i+1) != Arg) {
	// Note that icl_NthTerm is 1-based, icl_ReplaceElement is 0-based
	icl_ReplaceElement(Nterm, i, Arg, false);
	changed = true;
      }
    }
    if (!changed) {
      icl_Free(Nterm);
      Nterm = Term;
    }
    if (strcmp(func,"equ") == 0) {
      if (icl_match_terms(icl_NthTerm(Nterm, 1),icl_NthTerm(Nterm, 2), NULL)) {
	return icl_True();
      } else {
	return icl_NewStruct("==",2,icl_NthTerm(Nterm, 1),icl_NthTerm(Nterm, 2));
      }
    } else if (strcmp(func,"related_to") == 0) {
      return icl_NewStruct("http://calo.sri.com/core-plus-office#relatedProjectIs",
			   2, icl_NthTerm(Nterm, 1), icl_NthTerm(Nterm, 2));
    } else if (strcmp(func,"contains") == 0) {
      return icl_NewStruct("iris", 1,
			   icl_NewStruct("iris:bp_email_subject", 2,
					 icl_NthTerm(Nterm, 1), icl_NthTerm(Nterm, 2)));
    }
    return Nterm;
  } else if (icl_IsStr(Term)) {
    return calo_expansion(Term);
  }
  return Term;
}

// Goal of form pce_add_user_defined_rule(Username,Text,Rule)
int pce_add_user_defined_rule_callback(ICLTerm *goal, ICLTerm *params,
				       ICLTerm *solutions) {
  ICLTerm *Username, *Text, *Rule, *Strength, *Antecedent, *Consequent, *Impl;
  char *name, *iclstr;
  double strength;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);

  Username = icl_CopyTerm(icl_NthTerm(goal, 1));
  name = pce_make_lname(Username);
  // We don't actually care about the Text
  Text = icl_CopyTerm(icl_NthTerm(goal, 2));
  // Rule is of the form 'implies(Strength,Antecedent,Consequent)'
  Rule = icl_CopyTerm(icl_NthTerm(goal, 3));
  assert(strcmp(icl_Functor(Rule),"implies") == 0);
  Strength = icl_NthTerm(Rule, 1);
  Antecedent = icl_NthTerm(icl_NthTerm(Rule, 2), 1);
  Consequent = icl_NthTerm(icl_NthTerm(Rule, 3), 1);
  Impl = calo_syntax_expand(icl_NewStruct("implies", 2, Antecedent, Consequent));
  // Convert Strength to Weight
  strength = pce_translate_weight(Strength);
  pce_install_rule(Impl, strength);
  return true;
}

int pce_reset_cache_callback(ICLTerm *goal, ICLTerm *params,
			     ICLTerm *solutions) {
  char *iclstr;

  iclstr = icl_NewStringFromTerm(goal);  
  pce_log("%s", iclstr);
  icl_stFree(iclstr);
  // Currently a stub
  // pce_save(goal);
  return 0;
}

// Interface to query manager

static qm_buffer_t qm_buffer = {0, 0, NULL};

static void qm_buffer_resize() {
  int32_t i, prevcap, capacity, size;
  
  if (qm_buffer.entry == NULL) {
    qm_buffer.entry = (qm_entry_t **)
      safe_malloc(INIT_QM_BUFFER_SIZE * sizeof(qm_entry_t *));
    qm_buffer.capacity = INIT_QM_BUFFER_SIZE;
    for (i = 0; i < qm_buffer.capacity; i++) {
      qm_buffer.entry[i] =
	(qm_entry_t *) safe_malloc(sizeof(qm_entry_t));
    }
  }
  capacity = qm_buffer.capacity;
  size = qm_buffer.size;
  if (capacity <= size + 1){
    if (MAXSIZE(sizeof(qm_entry_t), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    prevcap = capacity;
    capacity += capacity/2;
    qm_buffer.entry = (qm_entry_t **)
      safe_realloc(qm_buffer.entry, capacity * sizeof(qm_entry_t *));
    for (i = prevcap; i < capacity; i++) {
      qm_buffer.entry[i] =
	(qm_entry_t *) safe_malloc(sizeof(qm_entry_t));
    }
    qm_buffer.capacity = capacity;
  }
  assert(qm_buffer.size+1 < qm_buffer.capacity);
}

// Check the availablity of the Query Manager
bool qm_available() {
  ICLTerm *Agtdata, *Params, *Out, *Result;

  Params = icl_NewTermFromString("[block(true)]");
  Agtdata = icl_NewTermFromString("agent_data(Address, Type, ready, Solvables, 'QueryManager', Info)");
  Out = NULL;
  Result = NULL;

  oaa_Solve(Agtdata, Params, &Out, &Result);
  return icl_NumTerms(Result) == 0 ? false : true;
}

ICLTerm *empty_params = NULL;

static void get_qm_instances(samp_table_t *table) {
  pred_table_t *pred_table;
  pred_tbl_t *evpred_tbl;
  const_table_t *const_table;
  char *pred, *query, argstr[8], *inststr, *cname, *bindingstr;
  int32_t i, j, k, arity, qsize, *psig, cidx, newconsts;
  int success;
  bool error;
  ICLTerm *callback, *Instances, *Answer, *Bindings, *Params, *Binding, *Arg, *Out;
  time_t start_time;

  if (!qm_available()) {
    printf("QueryManager not yet available\n");
    return;
  }

  Binding = NULL;
  start_time = time(NULL);
  pred_table = &table->pred_table;
  evpred_tbl = &pred_table->evpred_tbl;
  const_table = &table->const_table;
  for (i = 0; i<evpred_tbl->num_preds; i++) {
    pred = evpred_tbl->entries[i].name;
    arity = evpred_tbl->entries[i].arity;
    if (arity > 0) {
      psig = evpred_tbl->entries[i].signature;
      if (i >= qm_buffer.size) {
	// Now we build the query - has the form
	//  query(query_pattern('(PRED ?x01 ?x02)'),[answer_pattern('[{?x01},{?x02}]')],Results)
	qsize = 22 + strlen(pred) + (arity * 5) + 22 + (arity * 7) + 13;
	query = (char *) safe_malloc(qsize * sizeof(char));
	strcpy(query, "query(query_pattern('(");
	strcat(query, pred);
	for (j = 0; j < arity; j++) {
	  sprintf(argstr, " ?x%.2"PRId32, j+1);
	  strcat(query, argstr);
	}
	strcat(query, ")'),[answer_pattern('[");
	for (j = 0; j < arity; j++) {
	  sprintf(argstr, "{?x%.2"PRId32"}", j+1);
	  strcat(query, argstr);
	  if (j+1 < arity) {
	    strcat(query, ",");
	  }
	}
	strcat(query, "]')],Results)");
	assert(strlen(query) == qsize-1);
	callback = icl_NewTermFromData(query, qsize);
	qm_buffer_resize();
	assert(qm_buffer.size < qm_buffer.capacity);
	qm_buffer.size += 1;
	qm_buffer.entry[i]->query = callback;
      } else {
	callback = qm_buffer.entry[i]->query;
      }
      if (callback != NULL) {
	//oaa_Solve(callback, NULL, NULL, Instances);
	//      callback = icl_NewTermFromString("foo(X)");
	Instances = NULL;
	cprintf(1, "Calling query manager for predicate %s", pred);
	fflush(stdout);
	if (empty_params == NULL) {
	  empty_params = icl_NewTermFromString("[]");
	}
	Out = NULL;
	success = oaa_Solve(callback, empty_params, &Out, &Instances);
	if (success) {
	  inststr = icl_NewStringFromTerm(Instances);
	  printf("Instances for %s are %s\n", pred, inststr);
	  fflush(stdout);
	  icl_stFree(inststr);
	  // We now have Instances in the form [query(X, Y, answer([Binding], [Params]))]
	  //  Should be a singleton
	  //  Binding is a list of the form '["c1", "c2"]', ...
	  //  Params should be a list of the form [process_handle(N), status([Stat])] or [error(ErrString)]
	  if (! icl_IsList(Instances) || icl_NumTerms(Instances) != 1) {
	    pce_error("query: list of 1 element expected for Instances");
	    inststr = icl_NewStringFromTerm(Instances);
	    printf("Instances for %s are %s\n", pred, inststr);
	    fflush(stdout);
	    icl_stFree(inststr);
	  } else {
	    Answer = icl_NthTerm(icl_NthTerm(Instances, 1), 3);
	    Bindings = icl_NthTerm(Answer, 1);
	    Params = icl_NthTerm(Answer, 2);
	    // Check the Params - only care about errors for now.
	    error = false;
	    for (j = 0; j < icl_NumTerms(Params); j++) {
	      if (strcmp(icl_Functor(icl_NthTerm(Params,j+1)),"error") == 0) {
		error = true;
		break;
	      }
	    }
	    if (error) {
	      // Assume this means the predicate is unknown - set query to NULL
	      cprintf(1, ": unknown\n");
	      qm_buffer.entry[i]->query = NULL;
	    } else {
	      newconsts = 0;
	      for (j = 0; j < icl_NumTerms(Bindings); j++) {
		Binding = icl_NthTerm(Bindings, j+1);
		assert(icl_IsStr(Binding));
		bindingstr = str_copy(icl_Str(Binding));
		// Need to replace " with ' so we can get the term
		for (k = 0; k < strlen(bindingstr); k++) {
		  if (bindingstr[k] == '"') {
		    bindingstr[k] = '\'';
		  }
		}
		Binding = icl_NewTermFromString(bindingstr);
		assert(icl_IsList(Binding));
		for (k = 0; k < arity; k++) {
		  Arg = icl_NthTerm(Binding, k+1);
		  if (icl_IsStr(Arg)) {
		    cname = icl_Str(Arg);
		    cidx = const_index(cname, const_table);
		    if (cidx == -1) {
		      newconsts += 1;
		      inststr = icl_NewStringFromTerm(Arg);
		      fprintf(stderr, "Adding new constant %s\n", inststr);
		      icl_stFree(inststr);
		      pce_add_const(cname, psig[k], table);
		    }
		  } else {
		    inststr = icl_NewStringFromTerm(Arg);
		    fprintf(stderr, "Arg is %s\n", inststr);
		    icl_stFree(inststr);
		    char *icltype =
		      icl_IsList(Arg) ? "List" :
		      icl_IsGroup(Arg) ? "Group" :
		      icl_IsStruct(Arg) ? "Struct" :
		      icl_IsVar(Arg) ? "Var" :
		      icl_IsInt(Arg) ? "Int" :
		      icl_IsFloat(Arg) ? "Float" :
		      icl_IsDataQ(Arg) ? "DataQ" :
		      icl_IsValid(Arg) ? "Valid" : "Invalid";
		    fprintf(stderr, "Query Manager returned a %s for binding no. %"PRId32" (0-based):\n", icltype, k);
		    query = icl_NewStringFromTerm(callback);
		    inststr = icl_NewStringFromTerm(Instances);
		    fprintf(stderr, "  query: %s\n  binding: %s\n", query, inststr);
		    icl_stFree(query);
		    icl_stFree(inststr);
		  }
		}
	      }
	    cprintf(1, " %d instances, %d new constants\n", icl_NumTerms(Binding), newconsts);
	    }
	  }
	}
      }
    }
  }
  cprintf(1, "Query Manager calls took %d sec\n", time(NULL) - start_time); 
}

time_t pce_timeout = 60;
static time_t idle_timer = 0;

int pce_idle_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  time_t curtime;

  curtime = time(&curtime);
  if (difftime(curtime, idle_timer) > pce_timeout) {
    get_qm_instances(&samp_table);

    if (samp_table.atom_table.num_vars > 0) {
      pce_log("pce_idle_callback generating %d samples", DEFAULT_MAX_SAMPLES);
      if (lazy_pce) {
	lazy_mc_sat(&samp_table, DEFAULT_SA_PROBABILITY,
		    DEFAULT_SAMP_TEMPERATURE, DEFAULT_RVAR_PROBABILITY,
		    DEFAULT_MAX_FLIPS, DEFAULT_MAX_EXTRA_FLIPS,
		    DEFAULT_MAX_SAMPLES);
      } else {
	mc_sat(&samp_table, DEFAULT_SA_PROBABILITY, DEFAULT_SAMP_TEMPERATURE,
	       DEFAULT_RVAR_PROBABILITY, DEFAULT_MAX_FLIPS,
	       DEFAULT_MAX_EXTRA_FLIPS, DEFAULT_MAX_SAMPLES);
      }
    }
    printf("Calling pce_update_model\n");
    pce_update_model();
    time(&idle_timer);
  }
  // Should these be saved?
  // pce_save(goal);
  return 0;
}


/* int pce_do_event_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) { */
/*   printf("Just checking if app_do_event is being called\n"); */
  
/*   return 0; */
/* } */

// The cnf function basically converts a formula to CNF, returning a list of
// samp_rules.

// The Prolog PCE has the following formula operators: false, true, not,
// and, or, =>, implies, iff, eq, mutex, oneof.

// The X operator is the cross-product which takes the disjunction A \/ B
// for each clause A in Gamma and B in Delta to compute Gamma X Delta.

// CNF(A \/ B) = CNF(A) X CNF(B)

// CNF(A implies B) = CNF(not A) X CNF(B)

// CNF(A /\ B) = CNF(A) U CNF(B)

// CNF(not(A \/ B)) = CNF(not A) U CNF(not B)

// CNF(not(A implies B)) = CNF(A) U CNF(not B) 

// CNF (not(A /\ B)) = CNF(not A) X CNF(not B)

// CNF(not(not(A))) = CNF(A)

rule_literal_t ***cnf(ICLTerm *formula) {
  // Initialize the rules_vars_buffer, which will be used to hold the
  // variables as we go along
  rules_vars_buffer.size = 0;
  
  return cnf_pos(formula);
}

rule_literal_t ***cnf_pos(ICLTerm *formula) {
  char *functor;
  
  if (icl_IsStruct(formula)) {
    functor = icl_Functor(formula);
    if (strcmp(functor, "not") == 0) {
      return cnf_neg(icl_NthTerm(formula, 1));
    } else if (strcmp(functor, "or") == 0) {
      return cnf_product(cnf_pos(icl_NthTerm(formula, 1)),
			 cnf_pos(icl_NthTerm(formula, 2)));
    } else if ((strcmp(functor, "implies") == 0)
	       || (strcmp(functor, "=>") == 0)) {
      return cnf_product(cnf_neg(icl_NthTerm(formula, 1)),
			 cnf_pos(icl_NthTerm(formula, 2)));
    } else if (strcmp(functor, "and") == 0) {
      return cnf_union(cnf_pos(icl_NthTerm(formula, 1)),
		       cnf_pos(icl_NthTerm(formula, 2)));
    } else if (strcmp(functor, "iff") == 0) {
      // CNF(A iff B) = CNF(A /\ B) X CNF(not(A) /\ not(B))
      return cnf_product(cnf_union(cnf_pos(icl_NthTerm(formula, 1)),
				   cnf_pos(icl_NthTerm(formula, 2))),
			 cnf_union(cnf_neg(icl_NthTerm(formula, 1)),
				   cnf_neg(icl_NthTerm(formula, 2))));
/*     } else if (strcmp(functor, "eq") == 0) { */
/*     } else if (strcmp(functor, "mutex") == 0) { */
/*     } else if (strcmp(functor, "oneof") == 0) { */
    } else if (strcmp(functor, "iris") == 0 ||
	       strcmp(functor, "oaa") == 0) {
      return cnf_pos(icl_NthTerm(formula, 1));
    } else {
      return icl_to_cnf_literal(formula, false);
    }
  } else {
    pce_error("cnf_pos: Expect a struct here");
    return NULL;
  }
}

rule_literal_t ***cnf_neg(ICLTerm *formula) {
  char *functor;
  
  if (icl_IsStruct(formula)) {
    functor = icl_Functor(formula);
    if (strcmp(functor, "not") == 0) {
      return cnf_pos(icl_NthTerm(formula, 1));
    } else if (strcmp(functor, "or") == 0) {
      return cnf_union(cnf_neg(icl_NthTerm(formula, 1)),
		       cnf_neg(icl_NthTerm(formula, 2)));
    } else if ((strcmp(functor, "implies") == 0)
	       || (strcmp(functor, "=>") == 0)) {
      return cnf_union(cnf_pos(icl_NthTerm(formula, 1)),
		       cnf_neg(icl_NthTerm(formula, 2)));
    } else if (strcmp(functor, "and") == 0) {
      return cnf_product(cnf_neg(icl_NthTerm(formula, 1)),
			 cnf_neg(icl_NthTerm(formula, 2)));
    } else if (strcmp(functor, "iff") == 0) {
      // CNF(not(A iff B)) = CNF(A \/ B) U CNF(not(A) \/ not(B))
      return cnf_union(cnf_product(cnf_pos(icl_NthTerm(formula, 1)),
				   cnf_pos(icl_NthTerm(formula, 2))),
		       cnf_product(cnf_neg(icl_NthTerm(formula, 1)),
				   cnf_neg(icl_NthTerm(formula, 2))));
/*     } else if (strcmp(functor, "eq") == 0) { */
/*     } else if (strcmp(functor, "mutex") == 0) { */
/*     } else if (strcmp(functor, "oneof") == 0) { */
    } else if (strcmp(functor, "iris") == 0 ||
	       strcmp(functor, "oaa") == 0) {
      return cnf_neg(icl_NthTerm(formula, 1));
    } else {
      return icl_to_cnf_literal(formula, true);
    }
  } else {
    pce_error("cnf_neg: Expect a struct here");
    return NULL;
  }
}

// Given two CNFs - essentially lists of clauses, which are lists of
// literals.  We construct the product.  Here the CNFs are represented by
// (NULL-terminated) arrays of arrays of literals.
//   ((a b) (c d)) X ((x y z) (w)) == ((a b x y z) (a b w) (c d x y z) (c d w))
// For now, no simplification is done: so
// ((a b) (c)) X ((a ~c)) == ((a b a ~c) (c a ~c)), which could simplify to
//  ((a b ~c))

static rule_literal_t ***cnf_product(rule_literal_t ***c1,
				     rule_literal_t ***c2) {
  int32_t i, j, ii, jj, len1, len2, cnflen1, cnflen2, idx;
  rule_literal_t ***productcnf;
  rule_literal_t **conjunct;

  if (c1 == NULL || c2 == NULL) {
    return NULL;
  }
  for (cnflen1 = 0; c1[cnflen1] != 0; cnflen1++) {}
  for (cnflen2 = 0; c1[cnflen2] != 0; cnflen2++) {}
  productcnf = (rule_literal_t ***)
    safe_malloc((cnflen1 * cnflen2 + 1) * sizeof(rule_literal_t **));
  idx = 0;
  for (i = 0; c1[i] != NULL; i++) {
    for (len1 = 0; c1[i][len1] != NULL; len1++) {}
    for (j = 0; c2[j] != NULL; j++) {
      for (len2 = 0; c2[j][len2] != NULL; len2++) {}
      conjunct = (rule_literal_t **)
	safe_malloc((len1 + len2 + 1) * sizeof(rule_literal_t *));
      for (ii = 0; ii < len1; ii++) {
	conjunct[ii] = c1[i][ii];
      }
      for (jj = 0; jj < len2; jj++) {
	conjunct[len1+jj] = c2[j][jj];
      }
      conjunct[len1+len2] = NULL;
      productcnf[idx++] = conjunct;
    }
  }
  productcnf[idx] = NULL;
  for (i = 0; c1[i] != NULL; i++) {
    safe_free(c1[i]);
  }
  safe_free(c1);
  for (j = 0; c2[j] != NULL; j++) {
    safe_free(c2[j]);
  }
  safe_free(c2);
  return productcnf;
}

// Given two CNFs, return the union.
// ((a b) (c d)) U ((x y z) (w)) == ((a b) (c d) (x y z) (w))
// Again, no simplification involved.

static rule_literal_t ***cnf_union(rule_literal_t ***c1,
				   rule_literal_t ***c2) {
  int32_t i, cnflen1, cnflen2;
  rule_literal_t ***unioncnf;
  
  if (c1 == NULL || c2 == NULL) {
    return NULL;
  }
  for (cnflen1 = 0; c1[cnflen1] != 0; cnflen1++) {}
  for (cnflen2 = 0; c1[cnflen2] != 0; cnflen2++) {}
  unioncnf = (rule_literal_t ***)
    safe_malloc((cnflen1 + cnflen2 + 1) * sizeof(rule_literal_t **));
  for (i = 0; c1[i] != NULL; i++) {
    unioncnf[i] = c1[i];
  }
  for (i = 0; c2[i] != NULL; i++) {
    unioncnf[cnflen1+i] = c2[i];
  }
  unioncnf[cnflen1+i] = NULL;
  safe_free(c1);
  safe_free(c2);
  return unioncnf;
}

static rule_literal_t *** icl_to_cnf_literal(ICLTerm *formula, bool neg) {
  rule_literal_t *lit;
  rule_literal_t **conj_lit;
  rule_literal_t ***cnf_lit;
  
  lit = icl_to_rule_literal(formula, neg);
  if (lit == NULL) {
    return NULL;
  }
  conj_lit = (rule_literal_t **) safe_malloc(2 * sizeof(rule_literal_t *));
  cnf_lit = (rule_literal_t ***) safe_malloc(2 * sizeof(rule_literal_t **));
  cnf_lit[0] = conj_lit;
  cnf_lit[1] = NULL;
  conj_lit[0] = lit;
  conj_lit[1] = NULL;
  return cnf_lit;
}

static rule_literal_t *icl_to_rule_literal(ICLTerm *formula, bool neg) {
  ICLTerm *Arg;
  pred_table_t *pred_table = &samp_table.pred_table;
  const_table_t *const_table = &samp_table.const_table;
  int32_t i, j, num_args;
  char *icl_string;
  char *pred, *var;
  int32_t *psig;
  int32_t pred_val, pred_idx;
  samp_atom_t *atom;
  rule_literal_t *lit;
  
  if (!icl_IsAtomStruct(formula, true)) {
    icl_string = icl_NewStringFromTerm(formula);
    pce_error("Cannot convert %s to a literal\n", icl_string);
    icl_stFree(icl_string);
    return NULL;
  }
  pred = icl_Functor(formula);
  num_args = icl_NumTerms(formula);
  // check if we've seen the predicate before and complain if not.
  pred_val = pred_index(pred, pred_table);
  if (pred_val == -1) {
    pce_error("Predicate %s not declared", pred);
    return NULL;
  }
  pred_idx = pred_val_to_index(pred_val);
  if (pred_arity(pred_idx, pred_table) != num_args) {
    pce_error("Wrong number of args");
    return NULL;
  }

  atom = (samp_atom_t *) safe_malloc((num_args + 1) * sizeof(samp_atom_t));
  atom->pred = pred_idx;
  psig = pred_signature(pred_idx, pred_table);
  for(i=0; i < num_args; i++) {
    Arg = icl_NthTerm(formula, i+1);
    if (icl_IsVar(Arg)) {
      var = icl_Str(Arg);
      for (j = 0; j < rules_vars_buffer.size; j++) {
	if (strcmp(rules_vars_buffer.vars[j]->name, var) == 0) {
	  break;
	}
      }
      if (j == rules_vars_buffer.size) {
	// Wasn't found - add the new var entry
	rule_vars_buffer_resize(&rules_vars_buffer);
	// Copy the string
	rules_vars_buffer.vars[rules_vars_buffer.size]->name = str_copy(var);
	rules_vars_buffer.vars[rules_vars_buffer.size++]->sort_index = psig[i];
      }
      // Variables are negated indices into the var array, which will match
      // the names_buffer.  Need to add one in order not to hit zero.
      atom->args[i] = -(j + 1);
    } else {
      // Add the constant, of sort corresponding to the pred
      pce_add_const(icl_Str(Arg), psig[i], &samp_table);
      atom->args[i] = const_index(icl_Str(Arg), const_table);
    }
  }
  //print_atom(atom, &samp_table);
  //fflush(stdout);
  lit = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
  lit->neg = neg;
  lit->atom = atom;
  return lit;
}

static void rule_vars_buffer_resize(rule_vars_buffer_t *buf) {
  int32_t i, capacity, size;
  
  if (buf->vars == NULL) {
    buf->vars = (var_entry_t **)
      safe_malloc(INIT_NAMES_BUFFER_SIZE * sizeof(var_entry_t *));
    buf->capacity = INIT_NAMES_BUFFER_SIZE;
    for (i = 0; i < buf->capacity; i++) {
      buf->vars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
    }
  }
  capacity = buf->capacity;
  size = buf->size;
  if (capacity < size + 1){
    if (MAXSIZE(sizeof(var_entry_t *), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    capacity += capacity/2;
    buf->vars = (var_entry_t **)
      safe_realloc(buf->vars, capacity * sizeof(var_entry_t *));
    for (i = buf->capacity; i < capacity; i++) {
      buf->vars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
    }
    buf->capacity = capacity;
  }
}

var_entry_t **rule_vars_buffer_copy(rule_vars_buffer_t *buf) {
  var_entry_t **vars;
  int32_t i;

  if (buf->size == 0) {
    return NULL;
  }

  vars = (var_entry_t **) safe_malloc(buf->size * sizeof(var_entry_t *));
  for (i = 0; i < buf->size; i++) {
    vars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
    vars[i]->name = buf->vars[i]->name;
    vars[i]->sort_index = buf->vars[i]->sort_index;
  }
  return vars;
}

static void names_buffer_resize(names_buffer_t *nbuf) {
  if (nbuf->name == NULL) {
    nbuf->name = (char **) safe_malloc(INIT_NAMES_BUFFER_SIZE * sizeof(char *));
    nbuf->capacity = INIT_NAMES_BUFFER_SIZE;
  }
  int32_t capacity = nbuf->capacity;
  int32_t size = nbuf->size;
  if (capacity < size + 1){
    if (MAXSIZE(sizeof(char *), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    capacity += capacity/2;
    nbuf->name = (char **) safe_realloc(nbuf->name, capacity * sizeof(char *));
    nbuf->capacity = capacity;
  }
}

static char **names_buffer_copy(names_buffer_t *nbuf) {
  char **names;
  int32_t i;
  
  names = (char **) safe_malloc((nbuf->size + 1) * sizeof(char *));
  for (i = 0; i < nbuf->size; i++) {
    names[i] = nbuf->name[i];
  }
  names[nbuf->size] = NULL;
  return names;
}


// Need to figure out error handling in OAA
static void pce_error(const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vprintf(fmt, argp);
  va_end(argp);
  if (pce_log_fp != NULL) {
    va_start(argp, fmt);
    vfprintf(pce_log_fp, fmt, argp);
    vprintf(pce_log_fp, "\n");
    va_end(argp);
  }
  printf("\n");
}

/**
 * Setups up the the connection between this agent
 * and the facilitator.
 * @param argc passed in but not used in this implementation
 * @param argv passed in but not used in this implementation
 * @return true if successful
 */
int setup_oaa_connection(int argc, char *argv[]) {
  ICLTerm *pceSolvables;

  printf("Setting up OAA connection\n");
  if (!oaa_SetupCommunication(AGENT_NAME)) {
    printf("Could not connect\n");
    return false;
  }

  // Prepare a list of solvables that this agent is capable of
  // handling.
  printf("Setting up solvables\n");
  pceSolvables = icl_NewList(NULL); 
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_fact(Source,Formula),[callback(pce_fact)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_learner_assert(Lid,Formula,Weight),[callback(pce_learner_assert)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_learner_assert_list(Lid,List),[callback(pce_learner_assert_list)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_queryp(Q,P),[callback(pce_queryp)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_subscribe(Lid,Formula,Id),[callback(pce_subscribe)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_subscribe(Lid,Formula,Who,Condition,Id),[callback(pce_subscribe)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_process_subscriptions(X),[callback(pce_process_subscriptions)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_unsubscribe(Lid,F),[callback(pce_unsubscribe)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_unsubscribe(Lid,F,Who,Condition),[callback(pce_unsubscribe)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_unsubscribe(SubscriptionId),[callback(pce_unsubscribe)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_unsubscribe_learner(Lid),[callback(pce_unsubscribe_learner)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_full_model(M),[callback(pce_full_model)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_update_model(),[callback(pce_update_model)])"),
		true);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_add_user_defined_rule(Username,Text,Rule),[callback(pce_add_user_defined_rule)])"),
		true);

  // Register solvables with the Facilitator.
  // The string "parent" represents the Facilitator.
  printf("Registering solvables\n");
  if (!oaa_Register("parent", AGENT_NAME, pceSolvables)) {
    printf("Could not register\n");
    return false;
  }

  // Register the callbacks.
  if (!oaa_RegisterCallback("pce_fact", pce_fact_callback)) {
    printf("Could not register pce_fact callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_learner_assert",
			    pce_learner_assert_callback)) {
    printf("Could not register pce_learner_assert callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_learner_assert_list",
			    pce_learner_assert_list_callback)) {
    printf("Could not register pce_learner_assert_list callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_queryp", pce_queryp_callback)) {
    printf("Could not register pce_queryp callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_subscribe", pce_subscribe_callback)) {
    printf("Could not register pce_subscribe callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_process_subscriptions",
			    pce_process_subscriptions_callback)) {
    printf("Could not register pce_process_subscriptions callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_unsubscribe", pce_unsubscribe_callback)) {
    printf("Could not register pce_unsubscribe callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_unsubscribe_learner", pce_unsubscribe_learner_callback)) {
    printf("Could not register pce_unsubscribe_learner callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_full_model", pce_full_model_callback)) {
    printf("Could not register pce_full_model callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_update_model", pce_update_model_callback)) {
    printf("Could not register pce_update_model callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_add_user_defined_rule", pce_add_user_defined_rule_callback)) {
    printf("Could not register pce_add_user_defined_rule callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("pce_reset_cache", pce_reset_cache_callback)) {
    printf("Could not register pce_reset_cache callback\n");
    return false;
  }
  if (!oaa_RegisterCallback("app_idle", pce_idle_callback)) {
    pce_error("Could not register idle callback\n");
    return false;
  }

  // This seems to be ignored
  oaa_SetTimeout(pce_timeout);

  // Clean up.
  printf("Freeing solvables\n");
  icl_Free(pceSolvables);

  printf("Returning from setup_oaa_connection\n");
  return true;
}

void process_save_file_cmd(char *goal) {
  ICLTerm *Goal;
  int32_t len;

  if (goal == NULL) {
    return;
  }
  len = strlen(goal);
  Goal = icl_NewTermFromData(goal, len);
  
  // We assume the Goal is well-formed
  if (Goal == NULL) {
    pce_error("pce.persist: %s not parseable", goal);
    return;
  }
  if (strcmp(icl_Functor(Goal), "pce_fact") == 0) {
    pce_fact(Goal);
  } else if (strcmp(icl_Functor(Goal), "pce_learner_assert") == 0) {
    pce_learner_assert(Goal);
  } else if (strcmp(icl_Functor(Goal), "pce_learner_assert_list") == 0) {
    pce_learner_assert_list(Goal);
  } else {
    fprintf(stderr, "Something's wrong: save file contains\n  %s\n", goal);
  }
  icl_Free(Goal);
}

bool load_save_file(char *curdir) {
  bool save_file_exists;
  int32_t i;
  int32_t readbufsize;
  static char *readbuf;
  char *copy;
  char c;

  readbufsize = 80;
  readbuf = (char *) safe_malloc(readbufsize * sizeof(char));
  
  // Open the save file, and restore previous sessions
  save_file_exists = (access(pce_save_file, F_OK) == 0);
  pce_save_fp = fopen(pce_save_file, "a+");
  if (pce_save_fp == NULL) {
    char* err = (char *)
      safe_malloc((strlen(curdir) + strlen(pce_save_file) + 15) * sizeof(char));
    strncpy(err, "Error opening ", 15);
    strcat(err, getenv("PWD"));
    strcat(err, pce_save_file);
    perror(err);
    safe_free(err);
    return false;
  }
  if (save_file_exists) {
    i = 0;
    while ((c = fgetc(pce_save_fp)) != EOF) {
      if (c == '\n') {
	readbuf[i] = '\0';
	copy = str_copy(readbuf);
	process_save_file_cmd(copy);
	safe_free(copy);
	i = 0;
      } else {
	if (i + 1 >= readbufsize) {
	  readbufsize += readbufsize/2;
	  readbuf = (char *)
	    safe_realloc(readbuf, readbufsize * sizeof(char));
	}
	readbuf[i++] = c;
      }
    }
  }
  return true;
}

void create_log_file(char *curdir) {
  time_t t;
  struct tm *tmp;
  char logfile[28];

  t = time(NULL);
  tmp = localtime(&t);
  if (tmp == NULL) {
    perror("Log file not generated");
    return;
  }
  if (strftime(logfile, 28, "pce_%FT%H-%M-%S.log", tmp) == 0) {
    printf("Log file not generated: strftime returned 0\n");
    return;
  }
  printf("Attempting to open log file %s\n", logfile);
  pce_log_fp = fopen(logfile, "w");
  if (pce_log_fp == NULL) {
    perror("Could not open log file");
  } else {
    printf("Created Log file %s/%s\n", curdir, logfile);
  }
}

int main(int argc, char *argv[]){
  char *curdir;

  curdir = getenv("PWD");
  create_log_file(curdir);
  
  // Initialize OAA
  pce_log("Initializing OAA\n");
  printf("Initializing OAA\n");
  oaa_Init(argc, argv);
  if (!setup_oaa_connection(argc, argv)) {
    pce_error("Could not set up OAA connections...exiting\n");
    return EXIT_FAILURE;
  }
  
  init_samp_table(&samp_table);

  pce_log("Loading %s/%s\n", curdir, pce_init_file);
  printf("Loading %s/%s\n", curdir, pce_init_file);
  load_mcsat_file(pce_init_file, &samp_table);
  if (!load_save_file(curdir)) {
    return EXIT_FAILURE;
  }
  
  pce_log("Entering MainLoop\n");
  printf("Entering MainLoop\n");
  oaa_MainLoop(true);
  return EXIT_SUCCESS;
}
