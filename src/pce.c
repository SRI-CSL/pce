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
#include "memalloc.h"
#include "pce.h"

#define AGENT_NAME "PCE"

static const char *const pce_solvables[] = {
  "pce_add_user_defined_rule(Username,_Text,Rule)",
  "pce_current_assertions(L)",
  "pce_current_subscriptions(L)",
  "pce_dump_cache(Filename)",
  "pce_exit",
  "pce_fact(Formula)",
  "pce_fact(Source,Formula)",
  "pce_full_model(M)",
  "pce_get_cost(Cost)",
  "pce_learner_assert(Lid,Formula,Weight)",
  "pce_learner_assert_list(Lid,List)",
  "pce_load_file(File)",
  "pce_num_samples(N)",
  "pce_process_subscriptions(X)",
  "pce_query(QueryFormula)",
  "pce_query_others(LearnerId,Q,T,F,P)",
  "pce_queryp(Q,P)",
  "pce_reset",
  "pce_reset(Flags)",
  "pce_reset([])",
  "pce_reset([p1(Val1),p2(Val2),...])",
  "pce_reset_cache(Pattern)",
  "pce_status(Status)",
  "pce_subscribe(Lid,Formula,Id)",
  "pce_subscribe(Lid,Formula,Who,Condition,Id)",
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

static names_buffer_t names_buffer = {0, 0, NULL};

// There are two rules_vars_buffers - as a formula is clausified by cnf,
// variables are added to the rules_vars_buffer.  At the end,
// set_clause_variable uses the rule_vars_buffer to create the variables for
// each individual clause (represented by a samp_rule_t).
static rule_vars_buffer_t rules_vars_buffer = {0, 0, NULL};
static rule_vars_buffer_t rule_vars_buffer = {0, 0, NULL};

static samp_table_t samp_table;

static void pce_error(const char *fmt, ...);
static samp_rule_t **cnf(ICLTerm *formula);
static rule_literal_t ***cnf_pos(ICLTerm *formula);
static rule_literal_t ***cnf_neg(ICLTerm *formula);
static rule_literal_t ***cnf_product(rule_literal_t ***c1,
				     rule_literal_t ***c2);
static rule_literal_t ***cnf_union(rule_literal_t ***c1,
				   rule_literal_t ***c2);
static rule_literal_t *** icl_to_cnf_literal(ICLTerm *formula, bool neg);
static rule_literal_t *icl_to_rule_literal(ICLTerm *formula, bool neg);
static void set_clause_variables(samp_rule_t *clause);
static void names_buffer_resize(names_buffer_t *nbuf);
static void rule_vars_buffer_resize(rule_vars_buffer_t *buf);
static var_entry_t **rule_vars_buffer_copy(rule_vars_buffer_t *buf);
static char **names_buffer_copy(names_buffer_t *nbuf);
static void free_query_clause (samp_rule_t *clause);

// Check that the Term is a simple atom, i.e., p(a, b, c, ...) where a, b,
// c, ... are all constants or variables.  Also check for free variables if
// vars_allowed is false.
static bool icl_IsAtomStruct(ICLTerm *icl_term, bool vars_allowed) {
  ICLTerm *icl_arg;
  int32_t i;
  char *icl_string;

  if (!icl_IsStruct(icl_term)) {
    icl_string = icl_NewStringStructFromTerm(icl_term);
    pce_error("%s is not an atom", icl_string);
    icl_stFree(icl_string);
    return false;
  }
  for (i = 0; i < icl_NumTerms(icl_term); i++) {
    icl_arg = icl_NthTerm(icl_term, i+1);
    if (icl_IsVar(icl_arg)) {
      if (!vars_allowed) {
	icl_string = icl_NewStringStructFromTerm(icl_arg);
	pce_error("Variable %s is not allowed here", icl_string);
	icl_stFree(icl_string);
	return false;
      }
    } else if (!icl_IsStr(icl_arg)) {
      icl_string = icl_NewStringStructFromTerm(icl_arg);
      pce_error("%s is not a constant or variable", icl_string);
      icl_stFree(icl_string);
      return false;
    }
  }
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
    add_const_internal(names_buffer.name[i], psig[i], &samp_table);
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


/* Callback for pce_fact
   
 * This takes a Source (i.e., learner id), and a Formula, and adds the
 * formula as a fact.  The Source is currently ignored.  The Formula must be
 * an atom.  The predicate must have been previously declared (generally in
 * pce-init.pce), and its sorts are used to declare any new constants.
 */
int pce_fact_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  // The "goal" struct is in the form "pce_fact(Source,Formula)".
  pred_table_t *pred_table = &(samp_table.pred_table);
  //const_table_t *const_table = &(samp_table.const_table);
  //sort_table_t *sort_table = &(samp_table.sort_table);
  ICLTerm *Source = icl_CopyTerm(icl_NthTerm(goal, 1));
  ICLTerm *Formula = icl_CopyTerm(icl_NthTerm(goal, 2));
  ICLTerm *Arg;
  char *source;
  input_atom_t *atom;
  char *pred;
  int32_t pred_val, pred_idx;
  int32_t i, num_args;
  int32_t *psig;

  if (!icl_IsStr(Source)) {
    printf("pce_fact: Source should be a string");
    return FALSE;
  }
  source = icl_Str(Source);
  if (!icl_IsStruct(Formula)) {
    pce_error("pce_fact must be an atom");
  }
  pred = icl_Functor(Formula);
  num_args = icl_NumTerms(Formula);
  // check if we've seen the predicate before and complain if not.
  pred_val = pred_index(pred, pred_table);
  if (pred_val == -1) {
    pce_error("Predicate %s not declared", pred);
    return FALSE;
  }
  // Check that pred is direct, and arities match
  pred_idx = pred_val_to_index(pred_val);
  if (pred_idx < 0) {
    // It is direct - check the arity
    if (pred_arity(pred_idx, pred_table) != num_args) {
      pce_error("Wrong number of args");
      return FALSE;
    }
  } else {
    pce_error("Indirect predicate used in pce_fact");
    return FALSE;
  }
  psig = pred_signature(pred_idx, pred_table);
  for(i=0; i < num_args; i++) {
    Arg = icl_NthTerm(Formula, i+1);
    if(!icl_IsStr(Arg)) {
      // Each Arg must be a string, not a Struct or Var.
      pce_error("Args to Formula must be simple strings, not Variables");
      return FALSE;
    }
    names_buffer_resize(&names_buffer);
    names_buffer.name[i] = icl_Str(Arg);
    // Add the constant, of sort corresponding to the pred
    add_const_internal(names_buffer.name[i], psig[i], &samp_table);
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
  return TRUE;
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
  pred_table_t *pred_table = &(samp_table.pred_table);
  rule_table_t *rule_table = &(samp_table.rule_table);
  ICLTerm *Lid = icl_CopyTerm(icl_NthTerm(goal, 1));
  ICLTerm *Formula = icl_CopyTerm(icl_NthTerm(goal, 2));
  ICLTerm *Weight = icl_CopyTerm(icl_NthTerm(goal, 3));
  // ICLTerm *Arg;
  char *lid;
  samp_rule_t **clauses;
  double weight;
  int32_t ruleidx;
  int32_t i, j, atom_idx;
  rule_literal_t *lit;

  // First check that the args are OK
  if (!icl_IsStr(Lid)) {
    pce_error("pce_learner_assert: Lid should be a string");
    return FALSE;
  }
  if (!icl_IsFloat(Weight)) {
    pce_error("pce_learner_assert: Weight should be a float");
    return FALSE;
  }
  if (!icl_IsStruct(Formula)) {
    pce_error("pce_learner_assert: Formula should be a struct");
    return FALSE;
  }
  lid = icl_Str(Lid);
  weight = icl_Float(Weight);
  clauses = cnf(Formula);
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
      if (ruleidx != -1) {
	all_ground_instances_of_rule(current_rule, &samp_table);
      }
    }
  }
  //dump_clause_table(&samp_table);
  //dump_rule_table(&samp_table);
  return TRUE;
}

// Goal of the form queryp(Q, P).  The QueryFormula must be an
// atom possibly involving variables.
int pce_queryp_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  pred_table_t *pred_table = &(samp_table.pred_table);
  const_table_t *const_table = &(samp_table.const_table);
  atom_table_t *atom_table = &(samp_table.atom_table);
  ICLTerm *QueryFormula = icl_CopyTerm(icl_NthTerm(goal, 1));
  ICLTerm *Probability = icl_CopyTerm(icl_NthTerm(goal, 2));
  ICLTerm *Arg, *Narg;
  char *pred_str;
  int32_t pred_val, pred_idx, const_idx, i, j;
  pred_entry_t *pred_e;
  ICLTerm *solution, *list, *pterm;
  samp_rule_t *query_clause;
  samp_atom_t *patom;
  double prob;

  if (!icl_IsVar(Probability)) {
    pce_error("pce_queryp: Probability must be a variable");
    return false;
  }
  if (!icl_IsStruct(QueryFormula)) {
    pce_error("pce_queryp: QueryFormula must be an atom");
    return false;
  }
  pred_str = icl_Functor(QueryFormula);
  pred_val = pred_index(pred_str, pred_table);
  if (pred_val == -1) {
    pce_error("pce_queryp: Predicate %s is not declared", pred_str);
    return false;
  }
  pred_idx = pred_val_to_index(pred_val);
  pred_e = pred_entry(pred_table, pred_idx);
  if (icl_NumTerms(QueryFormula) != pred_e->arity) {
    pce_error("pce_queryp: Wrong arity");
    return false;
  }
  // Optimistically create the query_clause - must be freed before returning.
  query_clause = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
  // Currently only one literal
  query_clause->num_lits = 1;
  query_clause->literals = (rule_literal_t **)
    safe_malloc(sizeof(rule_literal_t *));
  query_clause->literals[0]->neg = false;
  query_clause->literals[0]->atom = (samp_atom_t *)
    // Includes pred and args
    safe_malloc((pred_e->arity + 1) * sizeof(int32_t));
  query_clause->literals[0]->atom->pred = pred_idx;
  
  // Now determine the variables array, and check that the constants
  // exist.  We use the names_buffer to keep the variables array.
  names_buffer.size = 0;
  for (i = 0; i < pred_e->arity; i++) {
    Arg = icl_NthTerm(QueryFormula, i+1);
    if (icl_IsStruct(Arg)) {
      pce_error("pce_queryp: Arguments must all be constants or variables");
      free_query_clause(query_clause);
      return false;
    }
    if (icl_IsVar(Arg)) {
      for (j = 0; j < names_buffer.size; j++) {
	if (strcmp(names_buffer.name[j], icl_Str(Arg)) == 0) {
	  break;
	}
      }
      if (j == names_buffer.size) {
	// Wasn't found - add the new name
	names_buffer_resize(&names_buffer);
	names_buffer.name[names_buffer.size++] = icl_Str(Arg);
      }
      // Variables are negated indices into the var array, which will match
      // the names_buffer.  Need to add one in order not to hit zero.
      query_clause->literals[0]->atom->args[i] = -(j + 1);
    } else {
      // Assume a constant; must already be known
      const_idx = const_index(icl_Str(Arg), const_table);
      if (const_idx == -1) {
	fprintf(stderr, "Argument %s not found\n", icl_Str(Arg));
	free_query_clause(query_clause);
	return false;
      }
      query_clause->literals[0]->atom->args[i] = const_idx;
    }
  }
  query_clause->num_vars = names_buffer.size;
  query_clause->vars = safe_malloc(names_buffer.size * sizeof(var_entry_t));
  for (i = 0; i < names_buffer.size; i++) {
    j = strlen(names_buffer.name[i]) + 1;
    query_clause->vars[i]->name = safe_malloc(j * sizeof(char));
    strcpy(query_clause->vars[i]->name, names_buffer.name[i]);
  }

  // Set up the substit buffer
  substit_buffer_resize(names_buffer.size);
  substit_buffer.size = names_buffer.size;
  for (i = 0; i < substit_buffer.size; i++) {
    substit_buffer.entries[i].fixed = false;
  }
  for (i = 0; i < pred_e->num_atoms; i++) {
    patom = atom_table->atom[pred_e->atoms[i]];
    if (match_atom_in_rule_atom(patom, query_clause->literals[0],
				pred_e->arity)) {
      prob = (double)
	(atom_table->pmodel[pred_e->atoms[i]]/(double) atom_table->num_samples);
      list = icl_NewList(NULL);
      for (j = 0; j < pred_e->arity; j++) {
	Narg = icl_NewStr(const_name(patom->args[j], const_table));
	icl_AddToList(list, Narg, TRUE);
      }
      pterm = icl_NewStructFromList(pred_str, list);
      solution = icl_NewStruct("queryp", 2, pterm, icl_NewFloat(prob));
      icl_AddToList(solutions, solution, TRUE);
    }
  }

  return TRUE;
}

static void free_query_clause (samp_rule_t *clause) {
  int32_t i;

  for (i = 0; i < clause->num_lits; i++) {
    safe_free(clause->literals[i]->atom);
  }
  safe_free(clause->literals);
  safe_free(clause);
}

time_t pce_timeout = 60;
static time_t idle_timer = 0;

int pce_idle_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  time_t curtime;

  curtime = time(&curtime);
  if (difftime(curtime, idle_timer) > pce_timeout) {
    mc_sat(&samp_table, DEFAULT_SA_PROBABILITY, DEFAULT_SAMP_TEMPERATURE,
	   DEFAULT_RVAR_PROBABILITY, DEFAULT_MAX_FLIPS,
	   DEFAULT_MAX_EXTRA_FLIPS, 50);
    time(&idle_timer);
  }
  return 0;
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

samp_rule_t **cnf(ICLTerm *formula) {
  samp_rule_t **clauses;
  samp_rule_t *clause;
  rule_literal_t ***lits;
  int32_t i, litlen;

  // Initialize the rules_vars_buffer, which will be used to hold the
  // variables as we go along, and indices for variables will be created
  // relative to the rules_vars_buffer index..  At the end, we call
  // set_clause_variables to create separate variables lists for each
  // generated samp_rule, which will generally be a subset of the
  // rules_vars_buffer, and indices will be adjusted accordingly.
  rules_vars_buffer.size = 0;
  rules_buffer.size = 0;
  
  lits = cnf_pos(formula);
  for (i = 0; lits[i] != NULL; i++) {
    clause = safe_malloc(sizeof(samp_rule_t));
    for (litlen = 0; lits[i][litlen] != NULL; litlen++) {}
    clause->num_lits = litlen;
    clause->literals = lits[i];
    set_clause_variables(clause);
    rules_buffer_resize(&rules_buffer);
    rules_buffer.rules[rules_buffer.size++] = clause;
  }
  rules_buffer_resize(&rules_buffer);
  rules_buffer.rules[rules_buffer.size++] = NULL;
  // Make a copy
  clauses = safe_malloc(clause_buffer.size * sizeof(samp_rule_t *));
  for (i = 0; i < rules_buffer.size; i++) {
    clauses[i] = rules_buffer.rules[i];
  }
  return clauses;
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
    } else {
      return icl_to_cnf_literal(formula, false);
    }
  } else {
    printf("Expect a struct here");
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
    } else {
      return icl_to_cnf_literal(formula, true);
    }
  } else {
    printf("Expect a struct here");
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

  for (cnflen1 = 0; c1[cnflen1] != 0; cnflen1++) {}
  for (cnflen2 = 0; c1[cnflen2] != 0; cnflen2++) {}
  productcnf = safe_malloc((cnflen1 * cnflen2 + 1) * sizeof(rule_literal_t *));
  idx = 0;
  for (i = 0; c1[i] != NULL; i++) {
    for (len1 = 0; c1[i][len1] != NULL; len1++) {}
    for (j = 0; c2[j] != NULL; j++) {
      for (len2 = 0; c2[j][len2] != NULL; len2++) {}
      conjunct = safe_malloc((len1 + len2 + 1) * sizeof(rule_literal_t));
      for (ii = 0; ii < len1; ii++) {
	conjunct[ii] = c1[i][ii];
      }
      for (jj = 0; jj < len2; jj++) {
	conjunct[len1+jj] = c2[j][jj];
      }
      conjunct[len1+len2] = NULL;
      productcnf[idx++] = conjunct;
      //safe_free(c2[j]);
    }
  }
  productcnf[idx] = NULL;
//   for (i = 0; c1[i] != NULL; i++) {
//     safe_free(c1[i]);
//   }
//   for (j = 0; c2[j] != NULL; j++) {
//     safe_free(c2[j]);
//   }
  return productcnf;
}

// Given two CNFs, return the union.
// ((a b) (c d)) U ((x y z) (w)) == ((a b) (c d) (x y z) (w))
// Again, no simplification involved.

static rule_literal_t ***cnf_union(rule_literal_t ***c1,
				   rule_literal_t ***c2) {
  int32_t i, cnflen1, cnflen2;
  rule_literal_t ***unioncnf;
  
  for (cnflen1 = 0; c1[cnflen1] != 0; cnflen1++) {}
  for (cnflen2 = 0; c1[cnflen2] != 0; cnflen2++) {}
  unioncnf = safe_malloc((cnflen1 + cnflen2 + 1) * sizeof(rule_literal_t *));
  for (i = 0; c1[i] != NULL; i++) {
    unioncnf[i] = c1[i];
  }
  for (i = 0; c2[i] != NULL; i++) {
    unioncnf[cnflen1+i] = c2[i];
  }
  unioncnf[cnflen1+i] = NULL;
  printf("cnf_union: length is %d\n", cnflen1+i);
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
  conj_lit = safe_malloc(2 * sizeof(rule_literal_t *));
  cnf_lit = safe_malloc(2 * sizeof(rule_literal_t **));
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
    icl_string = icl_NewStringStructFromTerm(formula);
    printf("Cannot convert %s to a literal\n", icl_string);
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
	// Should we copy the string here?
	rules_vars_buffer.vars[rules_vars_buffer.size]->name = var;
	rules_vars_buffer.vars[rules_vars_buffer.size++]->sort_index = psig[i];
      }
      // Variables are negated indices into the var array, which will match
      // the names_buffer.  Need to add one in order not to hit zero.
      atom->args[i] = -(j + 1);
    } else {
      // Add the constant, of sort corresponding to the pred
      if (add_const_internal(icl_Str(Arg), psig[i], &samp_table) == -1) {
	pce_error("Error adding constant %s", Arg);
	return NULL;
      }
      atom->args[i] = const_index(icl_Str(Arg), const_table);
    }
  }
  //print_atom(atom, &samp_table);
  //fflush(stdout);
  lit = safe_malloc(sizeof(rule_literal_t));
  lit->neg = neg;
  lit->atom = atom;
  return lit;
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

static void rule_vars_buffer_resize(rule_vars_buffer_t *buf) {
  int32_t i, capacity, size;
  
  if (buf->vars == NULL) {
    buf->vars = (var_entry_t **)
      safe_malloc(INIT_NAMES_BUFFER_SIZE * sizeof(var_entry_t *));
    buf->capacity = INIT_NAMES_BUFFER_SIZE;
    for (i = 0; i < buf->capacity; i++) {
      buf->vars[i] = safe_malloc(sizeof(var_entry_t));
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
      buf->vars[i] = safe_malloc(sizeof(var_entry_t));
    }
    buf->capacity = capacity;
  }
}

static var_entry_t **rule_vars_buffer_copy(rule_vars_buffer_t *buf) {
  var_entry_t **vars;
  int32_t i;
  
  vars = safe_malloc(buf->size * sizeof(var_entry_t *));
  for (i = 0; i < buf->size; i++) {
    vars[i] = safe_malloc(sizeof(var_entry_t));
    vars[i]->name = buf->vars[i]->name;
    vars[i]->sort_index = buf->vars[i]->sort_index;
  }
  return vars;
}

static void names_buffer_resize(names_buffer_t *nbuf) {
  if (nbuf->name == NULL) {
    nbuf->name = (char **)
      safe_malloc(INIT_NAMES_BUFFER_SIZE * sizeof(char *));
    nbuf->capacity = INIT_NAMES_BUFFER_SIZE;
  }
  int32_t capacity = nbuf->capacity;
  int32_t size = nbuf->size;
  if (capacity < size + 1){
    if (MAXSIZE(sizeof(char *), 0) - capacity <= capacity/2){
      out_of_memory();
    }
    capacity += capacity/2;
    nbuf->name =
      (char **) safe_realloc(nbuf->name, capacity * sizeof(char *));
    nbuf->capacity = capacity;
  }
}

static char **names_buffer_copy(names_buffer_t *nbuf) {
  char **names;
  int32_t i;
  
  names = safe_malloc((nbuf->size + 1) * sizeof(char *));
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
}

/**
 * Setups up the the connection between this agent
 * and the facilitator.
 * @param argc passed in but not used in this implementation
 * @param argv passed in but not used in this implementation
 * @return TRUE if successful
 */
int setup_oaa_connection(int argc, char *argv[]) {
  ICLTerm *pceSolvables;

  printf("Setting up OAA connection\n");
  if (!oaa_SetupCommunication(AGENT_NAME)) {
    printf("Could not connect\n");
    return FALSE;
  }

  // Prepare a list of solvables that this agent is capable of
  // handling.
  printf("Setting up solvables\n");
  pceSolvables = icl_NewList(NULL); 
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_fact(Source,Formula),[callback(pce_fact)])"),
		TRUE);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_learner_assert(Lid,Formula,Weight),[callback(pce_learner_assert)])"),
		TRUE);
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_queryp(Q,P),[callback(pce_queryp)])"),
		TRUE);

  // Register solvables with the Facilitator.
  // The string "parent" represents the Facilitator.
  printf("Registering solvables\n");
  if (!oaa_Register("parent", AGENT_NAME, pceSolvables)) {
    printf("Could not register\n");
    return FALSE;
  }

  // Register the callbacks.
  if (!oaa_RegisterCallback("pce_fact", pce_fact_callback)) {
    printf("Could not register pce_fact callback\n");
    return FALSE;
  }
  if (!oaa_RegisterCallback("pce_learner_assert", pce_learner_assert_callback)) {
    printf("Could not register pce_learner_assert callback\n");
    return FALSE;
  }
  if (!oaa_RegisterCallback("pce_queryp", pce_queryp_callback)) {
    printf("Could not register pce_queryp callback\n");
    return FALSE;
  }
  if (!oaa_RegisterCallback("app_idle", pce_idle_callback)) {
    pce_error("Could not register idle callback\n");
    return FALSE;
  }

  // This seems to be ignored
  oaa_SetTimeout(pce_timeout);

  // Clean up.
  printf("Freeing solvables\n");
  icl_Free(pceSolvables);

  printf("Returning from setup_oaa_connection\n");
  return TRUE;
}

int main(int argc, char *argv[]){
  char init_file[256];

  // Initialize OAA
  printf("Initializing OAA\n");
  oaa_Init(argc, argv);
  if (!setup_oaa_connection(argc, argv)) {
    printf("Could not set up OAA connections...exiting\n");
    return EXIT_FAILURE;
  }

  init_samp_table(&samp_table);
  //sprintf(init_file, "%s/pce_init.pce", getenv("HOME"));
  printf("Loading %s/pce_init.pce\n", getenv("PWD"));
  sprintf(init_file, "pce_init.pce");
  load_mcsat_file(init_file, &samp_table);
  fprintf(stderr, "Entering MainLoop\n");
  oaa_MainLoop(TRUE);
  return EXIT_SUCCESS;
}
