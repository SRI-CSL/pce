#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "memalloc.h"
#include "prng.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "gcd.h"
#include "utils.h"
#include "print.h"
#include "input.h"
#include "samplesat.h"
#include "lazysamplesat.h"
#include "vectors.h"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

#ifdef MINGW

/*
 * Need some version of random()
 * rand() exists on mingw but random() does not
 */
static inline int random(void) {
  return rand();
}

#endif
  
static void add_atom_to_pred(pred_table_t *pred_table,
			     int32_t predicate,
			     int32_t current_atom_index) {
  pred_entry_t *entry = pred_entry(pred_table, predicate);
  int32_t size;
  if (entry->num_atoms >= entry->size_atoms){
    if (entry->size_atoms == 0){
      entry->atoms = (int32_t *) safe_malloc(INIT_ATOM_PRED_SIZE * sizeof(int32_t));
      entry->size_atoms = INIT_ATOM_PRED_SIZE;
    } else {
      size = entry->size_atoms;
      if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
	out_of_memory();
      }
      size += size/2;
      entry->atoms = (int32_t *) safe_realloc(entry->atoms, size * sizeof(int32_t));
      entry->size_atoms = size;
    }
  }
  entry->atoms[entry->num_atoms++] = current_atom_index;
}

void add_rule_to_pred(pred_table_t *pred_table,
		      int32_t predicate,
		      int32_t current_rule_index) {
  pred_entry_t *entry = pred_entry(pred_table, predicate);
  int32_t i, size;
  
  for (i = 0; i < entry->num_rules; i++) {
    if (entry->rules[i] == current_rule_index) {
      return;
    }
  }
  if (entry->num_rules >= entry->size_rules){
    if (entry->size_rules == 0){
      entry->rules = (int32_t *) safe_malloc(INIT_RULE_PRED_SIZE * sizeof(int32_t));
      entry->size_rules = INIT_RULE_PRED_SIZE;
    } else {
      size = entry->size_rules;
      if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
	out_of_memory();
      }
      size += size/2;
      entry->rules = (int32_t *) safe_realloc(entry->rules, size * sizeof(int32_t));
      entry->size_rules = size;
    }
  }
  entry->rules[entry->num_rules++] = current_rule_index;
}

clause_buffer_t atom_buffer = {0, NULL}; 

int32_t add_internal_atom(samp_table_t *table,
			  samp_atom_t *atom){
  atom_table_t *atom_table = &(table->atom_table);
  pred_table_t *pred_table = &(table->pred_table);
  clause_table_t *clause_table = &(table->clause_table);
  int32_t i;
  int32_t predicate = atom->pred;
  int32_t arity = pred_arity(predicate, pred_table);
  samp_bvar_t current_atom_index;
  array_hmap_pair_t *atom_map;
  atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
				 arity+1, //+1 for pred
				 (int32_t *) atom);
  if (atom_map == NULL){
    //assert(valid_table(table));
    atom_table_resize(atom_table, clause_table);
    current_atom_index = atom_table->num_vars++;
    samp_atom_t * current_atom = (samp_atom_t *) safe_malloc((arity+1) * sizeof(int32_t));
    current_atom->pred = predicate;
    for (i = 0; i < arity; i++){
      current_atom->args[i] = atom->args[i];
    }
    atom_table->atom[current_atom_index] = current_atom;
    // Keep track of the number of samples when atom was introduced - these
    // will be subtracted in computing probs
    atom_table->sampling_nums[current_atom_index] = atom_table->num_samples;

    atom_map = array_size_hmap_get(&(atom_table->atom_var_hash),
				   arity+1,
				   (int32_t *) current_atom);
    atom_map->val = current_atom_index;
    
    if (pred_epred(predicate)){ //closed world assumption
      atom_table->assignment[0][current_atom_index] = v_fixed_false;
      atom_table->assignment[1][current_atom_index] = v_fixed_false;
    } else {//set pmodel to 0
      atom_table->pmodel[current_atom_index] = 0; //atom_table->num_samples/2;
      atom_table->assignment[0][current_atom_index] = v_false;
      atom_table->assignment[1][current_atom_index] = v_false;
      atom_table->num_unfixed_vars++;
    }
    add_atom_to_pred(pred_table, predicate, current_atom_index);
    clause_table->watched[pos_lit(current_atom_index)] = NULL;
    clause_table->watched[neg_lit(current_atom_index)] = NULL;
    //assert(valid_table(table));
    return current_atom_index;
  } else {
    return atom_map->val;
  }
}

void atom_buffer_resize(int32_t arity) {
  int32_t size = atom_buffer.size;

  if (size < arity+1){
    if (size == 0) {
      size = INIT_CLAUSE_SIZE;
    } else {
      if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
	out_of_memory();
      }
      size += size/2;
    }
    if (size < arity+1) {
      if (MAXSIZE(sizeof(int32_t), 0) <= arity)
	out_of_memory();
      size = arity+1;
    }
    atom_buffer.data = (int32_t  *)
      safe_realloc(atom_buffer.data, size * sizeof(int32_t));
    atom_buffer.size = size;
  }
}


int32_t add_atom(samp_table_t *table, input_atom_t *current_atom){
  const_table_t *const_table = &(table->const_table);
  pred_table_t *pred_table = &(table->pred_table);
  char * in_predicate = current_atom->pred;
  int32_t pred_id = pred_index(in_predicate, pred_table);
  if (pred_id == -1){
    mcsat_err("\nPredicate %s is not declared", in_predicate);
    return -1;
  }
  int32_t predicate = pred_val_to_index(pred_id);
  int32_t i;
  int32_t arity = pred_arity(predicate, pred_table);

  atom_buffer_resize(arity);
  samp_atom_t *atom = (samp_atom_t *) atom_buffer.data;
  atom->pred = predicate;
  for (i = 0; i < arity; i++){
    atom->args[i] = const_index(current_atom->args[i], const_table);
    if (atom->args[i] == -1){
      mcsat_err("\nConstant %s is not declared.", current_atom->args[i]);
      return -1;
    }
  }
  int32_t result = add_internal_atom(table, atom);
  return result;
}

void add_source_to_assertion(char *source, int32_t atom_index, samp_table_t *table) {
  source_table_t *source_table = &table->source_table;
  source_entry_t *source_entry;
  int32_t numassn, srcidx;
  
  for (srcidx = 0; srcidx < source_table->num_entries; srcidx++) {
    if (strcmp(source, source_table->entry[srcidx]->name) == 0) {
      break;
    }
  }
  if (srcidx == source_table->num_entries) {
    source_table_extend(source_table);
    source_entry = (source_entry_t *) safe_malloc(sizeof(source_entry_t));
    source_table->entry[srcidx] = source_entry;
    source_entry->name = source;
    source_entry->assertion = (int32_t *) safe_malloc(2 * sizeof(int32_t));
    source_entry->assertion[0] = atom_index;
    source_entry->assertion[1] = -1;
    source_entry->clause = NULL;
    source_entry->weight = NULL;
  } else {
    source_entry = source_table->entry[srcidx];
    if (source_entry->assertion == NULL) {
      source_entry->assertion = (int32_t *) safe_malloc(2 * sizeof(int32_t));
      source_entry->assertion[0] = atom_index;
      source_entry->assertion[1] = -1;
    } else {
      for (numassn = 0; source_entry->assertion[numassn] != -1; numassn++) {}
      source_entry->assertion = (int32_t *)
	safe_realloc(source_entry->assertion, (numassn+2) * sizeof(int32_t));
      source_entry->assertion[numassn] = atom_index;
      source_entry->assertion[numassn+1] = -1;
    }
  }
}
    

int32_t assert_atom(samp_table_t *table, input_atom_t *current_atom,
		    char *source){
  pred_table_t *pred_table = &table->pred_table;
  char *in_predicate = current_atom->pred;
  int32_t pred_id = pred_index(in_predicate, pred_table);
  
  if (pred_id == -1) {
    mcsat_err("assert: Predicate %s not found\n", in_predicate);
    return -1;
  }
  pred_id = pred_val_to_index(pred_id);
  if (pred_id >= 0) {
    mcsat_err("assert: May not assert atoms with indirect predicate %s\n", in_predicate);
    return -1;
  }
  int32_t atom_index = add_atom(table, current_atom);
  if (atom_index == -1){
    return -1;
  } else {
    table->atom_table.assignment[0][atom_index] = v_fixed_true;
    table->atom_table.assignment[1][atom_index] = v_fixed_true;
    if (source != NULL) {
      add_source_to_assertion(source, atom_index, table);
    }
    return atom_index;
  }
}

/* add_internal_clause is an internal operation used to add a clause.  The
   external operation is add_rule, where the rule can be ground or
   quantified.  These clauses must already be simplified so that they do not
   contain any ground evidence literals.
 */

int32_t add_internal_clause(samp_table_t *table,
			    int32_t *clause,
			    int32_t length,
			    double weight){
  uint32_t i;
  clause_table_t *clause_table;
  samp_clause_t **samp_clauses;
  array_hmap_pair_t *clause_map;

  clause_table = &table->clause_table;
  int_array_sort(clause, length);
  clause_map = array_size_hmap_find(&(clause_table->clause_hash), length, (int32_t *) clause);
  if (clause_map == NULL){
    clause_table_resize(clause_table);
    if (MAXSIZE(sizeof(int32_t), sizeof(samp_clause_t)) < length){
      out_of_memory();
    }
    int32_t index = clause_table->num_clauses++;
    samp_clause_t *entry = (samp_clause_t *) safe_malloc(sizeof(samp_clause_t) + length * sizeof(int32_t));
    samp_clauses = clause_table->samp_clauses;
    samp_clauses[index] = entry;
    entry->numlits = length;
    entry->weight = weight;
    entry->link = NULL;
    for (i=0; i < length; i++){
      entry->disjunct[i] = clause[i];
    }
    clause_map = array_size_hmap_get(&clause_table->clause_hash, length, (int32_t *) entry->disjunct);
    clause_map->val = index;
    // printf("\nAdded clause %"PRId32"\n", index);
    return index;
  } else {//add the weight to the existing clause
    samp_clauses = clause_table->samp_clauses;
    if (DBL_MAX - weight >= samp_clauses[clause_map->val]->weight){
      samp_clauses[clause_map->val]->weight += weight;
    } else {
      samp_clauses[clause_map->val]->weight = DBL_MAX;
    }  //do we need a similar check for negative weights? 
    
    return clause_map->val;
  }
}

void add_source_to_clause(char *source, int32_t clause_index, double weight,
			  samp_table_t *table) {
  source_table_t *source_table = &table->source_table;
  source_entry_t *source_entry;
  int32_t numclauses, srcidx;
  
  for (srcidx = 0; srcidx < source_table->num_entries; srcidx++) {
    if (strcmp(source, source_table->entry[srcidx]->name) == 0) {
      break;
    }
  }
  if (srcidx == source_table->num_entries) {
    source_table_extend(source_table);
    source_entry = (source_entry_t *) safe_malloc(sizeof(source_entry_t));
    source_table->entry[srcidx] = source_entry;
    source_entry->name = source;
    source_entry->assertion = NULL;
    source_entry->clause = (int32_t *) safe_malloc(2 * sizeof(int32_t));
    source_entry->weight = (double *) safe_malloc(2 * sizeof(double));
    source_entry->clause[0] = clause_index;
    source_entry->clause[1] = -1;
    source_entry->weight[0] = weight;
    source_entry->weight[1] = -1;
  } else {
    source_entry = source_table->entry[srcidx];
    if (source_entry->clause == NULL) {
      source_entry->clause = (int32_t *) safe_malloc(2 * sizeof(int32_t));
      source_entry->weight = (double *) safe_malloc(2 * sizeof(double));
      source_entry->clause[0] = clause_index;
      source_entry->clause[1] = -1;
      source_entry->weight[0] = clause_index;
      source_entry->weight[1] = -1;
    } else {
      for (numclauses = 0; source_entry->assertion[numclauses] != -1; numclauses++) {}
      source_entry->clause = (int32_t *)
	safe_realloc(source_entry->clause, (numclauses+2) * sizeof(int32_t));
      source_entry->weight = (double *)
	safe_realloc(source_entry->weight, (numclauses+2) * sizeof(double));
      source_entry->clause[numclauses] = clause_index;
      source_entry->clause[numclauses+1] = -1;
      source_entry->weight[numclauses] = weight;
      source_entry->weight[numclauses+1] = -1;
    }
  }
}
  

//add_clause is used to add an input clause with named atoms and constants.
//the disjunct array is assumed to be NULL terminated; in_clause is non-NULL.

int32_t add_clause(samp_table_t *table,
		   input_literal_t **in_clause,
		   double weight, char *source){
  int32_t i, atom, length, clause_index;
  
  length = 0;
  while (in_clause[length] != NULL){
    length++;
  }

  clause_buffer_resize(length);

  for (i=0; i < length; i++){
    atom = add_atom(table, in_clause[i]->atom);
    
    if (atom == -1){
      mcsat_err("\nBad atom");
      return -1;
    }
    if (in_clause[i]->neg){
      clause_buffer.data[i] = neg_lit(atom);
    } else {
      clause_buffer.data[i] = pos_lit(atom);
    }
  }
  clause_index = add_internal_clause(table, clause_buffer.data, length, weight);
  add_source_to_clause(source, clause_index, weight, table);
  return clause_index;
}

// new_samp_rule sets up a samp_rule, initializing what it can, and leaving
// the rest to typecheck_atom.  In particular, the variable sorts are set to -1,
// and the literals are uninitialized.
samp_rule_t * new_samp_rule(input_clause_t *rule) {
  int i;
  samp_rule_t *new_rule =
    (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
  // Allocate the vars
  new_rule->num_vars = rule->varlen;
  var_entry_t **vars =
    (var_entry_t **) safe_malloc(rule->varlen * sizeof(var_entry_t *));
  for (i=0; i<rule->varlen; i++) {
    vars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
    vars[i]->name = str_copy(rule->variables[i]);
    vars[i]->sort_index = -1;
  }
  new_rule->vars = vars;
  // Now the literals
  new_rule->num_lits = rule->litlen;
  rule_literal_t **lits =
    (rule_literal_t **) safe_malloc(rule->litlen * sizeof(rule_literal_t *));
  for(i=0; i<rule->litlen; i++) {
    lits[i] = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
    lits[i]->neg = rule->literals[i]->neg;
  }
  new_rule->literals = lits;
  return new_rule;
}

// Given an atom, a pred_table, an array of variables, and a const_table
// return true if:
//   atom->pred is in the pred_table
//   the arities match
//   for each arg:
//     the arg is either in the variable array or the const table
//     and its sort is the same as for the pred.
// Assumes that the variable array has already been checked for:
//   no duplicates
//   no clashes with the const_table (this is OK if shadowing is allowed,
//      and the variables are checked for first, for this possibility).
// The var_sorts array is the same length as the variables array, and
//    initialized to all -1.  As sorts are determined from the pred, the
//    array is set to the corresponding sort index.  If the sort is already
//    set, it is checked to see that it is the same.  Thus an error is flagged
//    if different occurrences of a variable are used with inconsistent sorts.
// If all goes well, a samp_atom_t is returned, with the predicate and args
//    replaced by indexes.  Note that variables are replaced with negative indices,
//    i.e., for variable number n, the index is -(n + 1)

samp_atom_t * typecheck_atom(input_atom_t *atom,
			     samp_rule_t *rule,
			     samp_table_t *samp_table){
  pred_table_t *pred_table = &(samp_table->pred_table);
  const_table_t *const_table = &(samp_table->const_table);
  char * pred = atom->pred;
  int32_t pred_val = pred_index(pred, pred_table);
  if (pred_val == -1) {
    mcsat_err("Predicate %s not previously declared\n", pred);
    return (samp_atom_t *) NULL;
  }
  int32_t pred_idx = pred_val_to_index(pred_val);
  pred_entry_t pred_entry;
  if (pred_idx < 0) {
    pred_entry = pred_table->evpred_tbl.entries[-pred_idx];
  } else {
    pred_entry = pred_table->pred_tbl.entries[pred_idx];
  }
  int32_t arglen = 0;
  while (atom->args[arglen] != NULL){
    arglen++;
  }
  if (pred_entry.arity != arglen) {
    mcsat_err("Predicate %s has arity %"PRId32", but given %"PRId32" args\n",
	    pred, pred_entry.arity, arglen);
    return (samp_atom_t *) NULL;
  }
  int argidx;
  //  int32_t size = pred_entry.arity * sizeof(int32_t);
  // Create a new atom - note that we will need to free it if there is an error
  samp_atom_t * new_atom = (samp_atom_t *)
    safe_malloc(sizeof(samp_atom_t) + pred_entry.arity * sizeof(int32_t));
  new_atom->pred = pred_idx;
  for (argidx=0; argidx<arglen; argidx++) {
    int32_t varidx = -1;
    int j;
    for (j=0; j<rule->num_vars; j++) {
      if (strcmp(atom->args[argidx], rule->vars[j]->name) == 0) {
	varidx = j;
	break;
      }
    }
    if (varidx != -1) {
      if (rule->vars[varidx]->sort_index == -1) {
	// Sort not set, we set it to the corresponding pred sort
	rule->vars[varidx]->sort_index = pred_entry.signature[argidx];
      }
      // Check that sort matches, else it's an error
      else if (rule->vars[varidx]->sort_index != pred_entry.signature[argidx]) {
	mcsat_err("Variable %s used with multiple sorts\n",
		  rule->vars[varidx]->name);
	safe_free(new_atom);
	return (samp_atom_t *) NULL;
      }
      new_atom->args[argidx] = -(varidx + 1);
    }
    else {
      // Not a variable, should be in the const_table
      int32_t constidx = const_index(atom->args[argidx], const_table);
      if (constidx == -1) {
	mcsat_err("Argument %s not found\n", atom->args[argidx]);
	safe_free(new_atom);
	return (samp_atom_t *) NULL;
      }
      else
	// Have a constant, check the sort
	if (const_sort_index(constidx,const_table) != pred_entry.signature[argidx]) {
	  mcsat_err("Constant %s has wrong sort for predicate %s\n",
		    atom->args[argidx], pred);
	  safe_free(new_atom);
	  return (samp_atom_t *) NULL;
	}
      new_atom->args[argidx] = constidx;
    }
  } // End of arg loop
  return new_atom;
}

int32_t add_rule(input_clause_t *rule,
		 double weight,
		 char *source,
		 samp_table_t *samp_table){
  rule_table_t *rule_table = &(samp_table->rule_table);
  pred_table_t *pred_table = &(samp_table->pred_table);
  int32_t i;
  // Make sure rule has variables - else it is a ground clause
  assert(rule->varlen > 0);
  // Might as well make sure it also has literals
  assert(rule->litlen > 0);
  // Need to check that variables are all used, and used consistently
  // Need to check args against predicate signatures.
  // We will use the new_rule for the variables
  samp_rule_t *new_rule = new_samp_rule(rule);
  new_rule->weight = weight;
  for (i=0; i<rule->litlen; i++) {
    input_atom_t *in_atom = rule->literals[i]->atom;
    samp_atom_t *atom = typecheck_atom(in_atom, new_rule, samp_table);
    if (atom == NULL) {
      // Free up the earlier atoms
      int32_t j;
      for (j=0; j<i; j++) {
	safe_free(new_rule->literals[j]->atom);
      }
      safe_free(new_rule);
      return -1;
    }
    new_rule->literals[i]->atom = atom;
  }
  // New rule is OK, now add it to the rule_table.  For now, we ignore the
  // fact that it may already be there - a future optimization is to
  // recognize duplicate rules and add their weights.
  int32_t current_rule = rule_table->num_rules;
  rule_table_resize(rule_table);
  rule_table->samp_rules[current_rule] = new_rule;
  rule_table->num_rules += 1;
  // Now loop through the literals, adding the current rule to each pred
  for (i=0; i<rule->litlen; i++) {
    int32_t pred = new_rule->literals[i]->atom->pred;
    add_rule_to_pred(pred_table, pred, current_rule);
  }
  return current_rule;
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

bool eql_query_instance_lits(samp_literal_t **lit1, samp_literal_t **lit2) {
  int32_t i, j;

  for (i = 0; lit1[i] != NULL; i++) {
    if (lit2[i] == NULL) {
      return false;
    }
    for (j = 0; lit1[i][j] != -1; j++) {
      // Note there is no need to test lit2 != -1
      if (lit2[i][j] != lit1[i][j]) {
	return false;
      }
    }
    // Check if lit2 has more lits
    if (lit2[i][j] != -1) {
      return false;
    }
  }
  // Check if lit2 has more clauses
  if (lit2[i] != NULL) {
    return false;
  }
  return true;
}

int32_t add_query(var_entry_t **vars, rule_literal_t ***lits,
		  samp_table_t *table) {
  query_table_t *query_table = &table->query_table;
  samp_query_t *query;
  int32_t i, numvars, query_index;
  
  for (i = 0; i < query_table->num_queries; i++) {
    if (eql_query_entries(lits, query_table->query[i], table)) {
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
  if (vars == NULL) {
    numvars = 0;
  } else {
    for (numvars = 0; vars[numvars] != NULL; numvars++) {}
  }
  query->num_vars = numvars;
  query->literals = lits;
  query->vars = vars;
  query->source_index = NULL;
  // Need to create instances and add to query_instance_table
  all_query_instances(query, table);
  return query_index;
}

// int32_t add_query_instance(samp_literal_t **lits, int32_t num_lits, samp_table_t *table) {
//   query_instance_table_t *query_instance_table;
//   query_table_t *query_table;
//   samp_query_t *query;
//   samp_query_instance_t *qinst;
//   int32_t i, qindex;
  
//   query_instance_table = &table->query_instance_table;
//   query_table = &table->query_table;
//   for (i = 0; i < query_instance_table->num_queries; i++) {
//     if (eql_query_instance_lits(lits, query_instance_table->query_inst[i]->lit)) {
// 	break;
//     }
//   }
//   qindex = i;
//   if (i < query_instance_table->num_queries) {
//     // Already have this instance - just associate it with the query
//   } else {
//     query_instance_table_resize(query_instance_table);
//     qinst = (samp_query_instance_t *) safe_malloc(sizeof(samp_query_instance_t));
//     query_instance_table->query_inst[qindex] = qinst;
//     query_instance_table->num_queries += 1;
//     for (i = 0; i < query_table->num_queries; i++) {
//       if (query_table->query[i] == query) {
// 	break;
//       }
//     }
//     qinst->query_index = i;
//     qinst->sampling_num = table->atom_table.num_samples;
//     qinst->pmodel = 0;
//     // Copy the substs - used to return the OAA formula instances
//     qinst->subst = (int32_t *) safe_malloc(query->num_vars * sizeof(int32_t));
//     for (i = 0; i < query->num_vars; i++) {
//       qinst->subst[i] = substs[i].const_index;
//     }
//     qinst->lit = litinst;
//   }
//   return qindex;
// }

void retract_source(char *source, samp_table_t *table) {
  source_table_t *source_table = &table->source_table;
  clause_table_t *clause_table;
  source_entry_t *source_entry;
  int32_t i, j, srcidx, sidx, clause_idx;
  samp_clause_t *clause;
  double wt;

  for (srcidx = 0; srcidx < source_table->num_entries; srcidx++) {
    if (strcmp(source, source_table->entry[srcidx]->name) == 0) {
      break;
    }
  }
  if (srcidx == source_table->num_entries) {
    mcsat_err("\nSource %s unknown\n", source);
    return;
  }
  source_entry = source_table->entry[srcidx];
  if (source_entry->assertion != NULL) {
    // Not sure what to do here
  }
  if (source_entry->clause != NULL) {
    clause_table = &table->clause_table;
    for (i = 0; source_entry->clause[i] != -1; i++) {
      clause_idx = source_entry->clause[i];
      clause = clause_table->samp_clauses[clause_idx];
      if (source_entry->weight[i] == DBL_MAX) {
	// Need to go through all other sources and add them up
	wt = 0.0;
	for (sidx = 0; sidx < source_table->num_entries; sidx++) {
	  if (sidx != srcidx
	      && source_table->entry[sidx]->clause != NULL) {
	    for (j = 0; source_table->entry[sidx]->clause[j] != -1; j++) {
	      if (source_table->entry[sidx]->clause[j] == clause_idx) {
		wt += source_table->entry[sidx]->weight[j];
		// Assume clause occurs only once for given source,
		// as we can simply sum up all such occurrences.
		break;
	      }
	    }
	  }
	}
	clause->weight = wt;
      } else if (clause->weight != DBL_MAX) {
	// Subtract the weight of this source from the clause
	clause->weight -= source_entry->weight[i];
      } // Nothing to do if current weight is DBL_MAX
    }
  }
}
  

//Next, need to write the main samplesat routine.
//How should hard clauses be represented, with weight MAX_DOUBLE?
//The parameters include clause_set, DB, KB, maxflips, p_anneal,
//anneal_temp, p_walk.

//The assignment will map each atom to undef, T, F, FixedT, FixedF,
//DB_T, or DB_F.   The latter assignments are fixed by the closed world
//assumption on the DB.  The other fixed assignments are those obtained
//from unit propagation on the selected clauses.

//The samplesat routine starts with a random assignment to the non-fixed
//variables and a selection of alive clauses.  The cost is the
//number of unsat clauses.  It then either (with prob p_anneal) picks a
//variable and flips it (if it reduces cost) or with probability (based on
//the cost diff).  Otherwise, it does a walksat step.
//The tricky question is what it means to activate a clause or atom.

//Code for random selection with filtering is in smtcore.c
//(select_random_bvar)

//First step is to write a unit-propagator.  The propagation is
//done repeatedly to handle recent additions.


//propagates a truth assignment on a link of watched clauses for a literal that
//has been assigned false.  All the
//literals are assumed to have truth values.  The watched literal points to 
//a true literal if there is one in the clause.  Whenever a watched literal
//is assigned false, the propagation must find a new true literal, or retain
//the existing one.  If a clause is falsified, its weight is added to the
//delta cost.  The clauses containing the negation of the literal must be
//reevaluated and assigned true if previously false.

void link_propagate(samp_table_t *table, samp_literal_t lit){
  int32_t numlits, i;
  samp_literal_t tmp;
  samp_clause_t *new_link; 
  samp_clause_t *link;
  samp_clause_t **link_ptr;
  samp_literal_t *disjunct;
  samp_truth_value_t *assignment;

  //cprintf(0, "link_propagate called\n");

  assignment = table->atom_table.assignment[table->atom_table.current_assignment];

  assert(assigned_false_lit(assignment, lit)); // ?? this assertion fails!

  link_ptr = &(table->clause_table.watched[lit]);
  link = table->clause_table.watched[lit];
  while (link != NULL){
    //cprintf(0, "Checking watched\n");
    numlits = link->numlits;
    disjunct = link->disjunct;

    assert(disjunct[0] == lit && numlits > 1);

    i = 1;
    while (i<numlits && assigned_false_lit(assignment, disjunct[i])) {
      i++;
    }

    if (i < numlits) { 
      /*
       * the clause is still true: make tmp = disjunct[i] the new watched literal
       */
      tmp = disjunct[i];

      // swap lit and tmp
      disjunct[i] = lit;
      disjunct[0] = tmp;
      
      // add the clause to the head of watched[tmp]
      new_link = link->link;
      *link_ptr = new_link;
      link->link = table->clause_table.watched[tmp];
      table->clause_table.watched[tmp] = link;
      link = new_link;
      assert(assigned_true_lit(assignment,
				 table->clause_table.watched[tmp]->disjunct[0]));
    } else {
      /*
       * move the clause to the unsat_clause list
       */
      //cprintf(0, "Adding to usat_clauses\n");
      new_link = link->link;
      *link_ptr = new_link;
      link->link = table->clause_table.unsat_clauses;
      table->clause_table.unsat_clauses = link;
      table->clause_table.num_unsat_clauses++;
      link = new_link;
    }
  }

  table->clause_table.watched[lit] = NULL;//since there are no clauses where it is true
}


// returns a literal index corresponding to a fixed true literal or a
// unique non-fixed implied literal
int32_t get_fixable_literal(samp_truth_value_t *assignment,
			    samp_literal_t *disjunct,
			    int32_t numlits){
  int32_t i, j;
  i = 0;
  while (i < numlits && assigned_fixed_false_lit(assignment, disjunct[i])){
    i++;
  } // i = numlits or not fixed, or disjunct[i] is true; now check the remaining lits
  if (i < numlits) {
    if (assigned_fixed_true_lit(assignment, disjunct[i]))
      return i;
    j = i+1;
    while (j < numlits && assigned_fixed_false_lit(assignment, disjunct[j])){
      j++;
    } // if j = numlits, then i is propagated
    if (j < numlits) {
      if (assigned_fixed_true_lit(assignment, disjunct[j]))
	return j;
      return -1; // there are two unfixed lits
    }
  }
  return i; // i is fixable
}

int32_t get_true_lit(samp_truth_value_t *assignment,
		     samp_literal_t *disjunct,
		     int32_t numlits){
  int32_t i;
  i = 0;
  while (i < numlits && assigned_false_lit(assignment, disjunct[i])){
    i++;
  }
  return i;
}

/*
 * Scans the unsat clauses to find those that are sat or to propagate
 * fixed truth values.  The propagating clauses are placed on the sat_clauses
 * list, and the propagated literals are placed in fixable_stack so that they
 *
 * Returns -1 if a conflict is detected, 0 otherwise.
 */
int32_t scan_unsat_clauses(samp_table_t *table){
  samp_clause_t *unsat_clause;
  samp_clause_t **unsat_clause_ptr;
  unsat_clause_ptr = &table->clause_table.unsat_clauses;
  unsat_clause = *unsat_clause_ptr;
  int32_t i;
  samp_truth_value_t *assignment = table->atom_table.assignment[table->atom_table.current_assignment];
  int32_t fixable;
  samp_literal_t lit;
  while (unsat_clause != NULL){//see if the clause is fixed-unit propagating
    //cprintf(0, "Scanning unsat clauses\n");
    fixable = get_fixable_literal(assignment, unsat_clause->disjunct, unsat_clause->numlits);
    if (fixable >= unsat_clause->numlits) {
      // Conflict detected
      return -1;
    }
    if (fixable == -1){//if not propagating
      i = get_true_lit(assignment, unsat_clause->disjunct, unsat_clause->numlits);
      if (i< unsat_clause->numlits){//then lit occurs in the clause
      //      *dcost -= unsat_clause->weight;  //subtract weight from dcost
	lit = unsat_clause->disjunct[i];              //swap literal to watched position
	unsat_clause->disjunct[i] = unsat_clause->disjunct[0];
	unsat_clause->disjunct[0] = lit;
	*unsat_clause_ptr = unsat_clause->link;
	unsat_clause->link = table->clause_table.watched[lit];  //push into watched list
	table->clause_table.watched[lit] = unsat_clause;
	table->clause_table.num_unsat_clauses--;
	unsat_clause = *unsat_clause_ptr;
	assert(assigned_true_lit(assignment, table->clause_table.watched[lit]->disjunct[0]));
      } else {
	unsat_clause_ptr = &(unsat_clause->link);  //move to next unsat clause
	unsat_clause = unsat_clause->link;
      }
    } else {//we need to fix the truth value of disjunct[fixable]
      lit = unsat_clause->disjunct[fixable]; // swap the literal to the front

      unsat_clause->disjunct[fixable] = unsat_clause->disjunct[0];
      unsat_clause->disjunct[0] = lit;
      cprintf(2, "Fixing variable %"PRId32"\n", var_of(lit));
      if (!fixed_tval(assignment[var_of(lit)])){
	if (is_pos(lit)){
	  assignment[var_of(lit)] = v_fixed_true;
	} else {
	  assignment[var_of(lit)] = v_fixed_false;
	}
	table->atom_table.num_unfixed_vars--;
      }
      if (assigned_true_lit(assignment, lit)){ //push clause into fixed sat list
	push_integer_stack(lit, &(table->fixable_stack));
      }
      *unsat_clause_ptr = unsat_clause->link;
      unsat_clause->link = table->clause_table.sat_clauses;
      table->clause_table.sat_clauses = unsat_clause;
      assert(assigned_fixed_true_lit(assignment, table->clause_table.sat_clauses->disjunct[0]));
      unsat_clause = *unsat_clause_ptr;
      table->clause_table.num_unsat_clauses--;
    }
  }
  return 0;
}

int32_t process_fixable_stack(samp_table_t *table){
  samp_literal_t lit;
  int32_t conflict = 0;
  while (!empty_integer_stack(&(table->fixable_stack)) && conflict == 0){
    while (!empty_integer_stack(&(table->fixable_stack)) && conflict == 0){
      lit = top_integer_stack(&(table->fixable_stack));
      pop_integer_stack(&(table->fixable_stack));
      link_propagate(table, not(lit));
    }
     conflict = scan_unsat_clauses(table);
  }
  return conflict;
}


//Executes a variable flip by first scanning all the previously sat clauses
//in the watched list, and then moving any previously unsat clauses to the
//sat/watched list.  
int32_t flip_unfixed_variable(samp_table_t *table,
			      int32_t var) {
  //  double dcost = 0;   //dcost seems unnecessary
  atom_table_t *atom_table = &table->atom_table;
  samp_truth_value_t *assignment = atom_table->assignment[atom_table->current_assignment]; 
  cprintf(2,"Flipping variable %"PRId32" to %s\n",
	  var, assigned_true(assignment[var]) ? "false" : "true");
  if (assigned_true(assignment[var])){
    assignment[var] = v_false;
    link_propagate(table, pos_lit(var));
  } else {
    assignment[var] = v_true;
    link_propagate(table, neg_lit(var));
  }
  if (scan_unsat_clauses(table) == -1) {
    return -1;
  }
  return process_fixable_stack(table);
}

//computes the cost of flipping an unfixed variable without the actual flip
void cost_flip_unfixed_variable(samp_table_t *table,
				int32_t *dcost, 
				int32_t var){
  *dcost = 0;
  samp_literal_t lit, nlit;
  uint32_t i;
  atom_table_t *atom_table = &(table->atom_table);
  samp_truth_value_t *assignment = atom_table->assignment[atom_table->current_assignment]; 

  if (assigned_true(assignment[var])){
    lit = neg_lit(var);
    nlit = pos_lit(var);
  } else {
    lit = pos_lit(var);
    nlit = neg_lit(var);
  }
  samp_clause_t *link  = table->clause_table.watched[lit];
  while (link != NULL){
    i = 1;
    while (i < link->numlits && assigned_false_lit(assignment, link->disjunct[i])){
      i++;
    }
    if (i >= link->numlits){
      *dcost += 1;
    }
    link = link->link;
  }
  //Now, subtract the weight of the unsatisfied clauses that can be flipped
  link = table->clause_table.unsat_clauses;
  while (link != NULL){
    i = 0;
    while (i < link->numlits && link->disjunct[i] != lit){
      i++;
    }
    if (i < link->numlits){
      *dcost -= 1;
    }
    link = link->link;
  }
}




/*
 * SELECTION OF LIVE CLAUSES
 */

/*
 * Temporary print function for debugging
 * - print_clause is in util.c
 */

static void print_list(samp_clause_t *link, samp_table_t *table) {
  while (link != NULL) {
    print_clause(link, table);
    printf("\n");
    link = link->link;
  }
}

/*
 * Random number generator:
 * - returns a floating point number in the interval [0.0, 1.0)
 */
double choose(){
#ifdef _WIN32
  // Get the cryptographic service provider
  HCRYPTPROV hCryptProv;
  CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, 0);

  // Get the random number
  union {
    BYTE bs[sizeof(unsigned long int)];
    unsigned long int li;
  } rand_buf;
  CryptGenRandom(hCryptProv, sizeof(rand_buf), (BYTE*) &rand_buf);

  return ((double) rand_buf.li) / ((double) ULONG_MAX + 1.0);
#else
  return ((double) random()) / ((double) RAND_MAX + 1.0);
#endif
}


/*
 * Randomly select live clauses from a list:
 * - link = start of the list and *link_ptr == link
 * - all clauses in the list must be satisfied in the current assignment
 * - a clause of weight W is killed with probability exp(-|W|)
 *   (if W == DBL_MAX then it's a hard clause and it's not killed).
 */
void kill_clause_list(samp_clause_t **link_ptr, samp_clause_t *link,
		      clause_table_t *clause_table) {
  samp_clause_t *next;
  double abs_weight;

  while (link != NULL) {
    abs_weight = fabs(link->weight);
    if (abs_weight < DBL_MAX && choose() < exp(- abs_weight)) {
      // move it to the dead_clauses list
      next = link->link;
      link->link = clause_table->dead_clauses;
      clause_table->dead_clauses = link;
      link = next;
    } else {
      // keep it
      *link_ptr = link;
      link_ptr = &link->link;
      link = link->link;
    }
  }
  *link_ptr = NULL;
}


void kill_clauses(samp_table_t *table){
  clause_table_t *clause_table = &(table->clause_table);
  atom_table_t *atom_table = &(table->atom_table);
  int32_t i, lit;

  //unsat_clauses is empty; need to only kill satisfied clauses
  //kill sat_clauses
  kill_clause_list(&clause_table->sat_clauses, clause_table->sat_clauses, clause_table);

  //kill watched clauses
  for (i = 0; i < atom_table->num_vars; i++){
    lit = pos_lit(i);
    kill_clause_list(&clause_table->watched[lit], clause_table->watched[lit], clause_table);
    lit = neg_lit(i); 
    kill_clause_list(&clause_table->watched[lit], clause_table->watched[lit], clause_table);
  }

  // TEMPORARY
  if (get_verbosity_level() > 1){
    printf("Live clauses:\n");
    print_list(clause_table->sat_clauses, table);
    for (i = 0; i < atom_table->num_vars; i++){
      lit = pos_lit(i);
      print_list(clause_table->watched[lit], table);
      lit = neg_lit(i);
      print_list(clause_table->watched[lit], table);
    }
    printf("\n");
  }
}




/*
 * UNIT PROPAGATION
 */
int32_t fix_lit_true(samp_table_t *table, int32_t lit){
  int32_t var = var_of(lit);
  uint32_t sign = sign_of_lit(lit);
  atom_table_t *atom_table = &(table->atom_table);
  samp_truth_value_t *assignment = atom_table->assignment[atom_table->current_assignment]; 

  if (fixed_tval(assignment[var])){
    if (sign
	? assigned_true(assignment[var])
	: assigned_false(assignment[var])){
      return -1;//Conflict: The unit literal can't be fixed true,
       //since it is fixed false
    }
  } else {
    if (is_pos(lit)){
      cprintf(2, "Fixing %"PRId32" to true\n", var);
      assignment[var] = v_fixed_true;
      table->atom_table.num_unfixed_vars--;
      link_propagate(table, neg_lit(var));
    } else {
      cprintf(2, "Fixing %"PRId32" to false\n", var);	  
      assignment[var] = v_fixed_false;
      table->atom_table.num_unfixed_vars--;
      link_propagate(table, pos_lit(var));
    }
  }
  return 0;
}

int32_t fix_lit_false(samp_table_t *table, int32_t lit){
  int32_t var = var_of(lit);
  uint32_t sign = sign_of_lit(lit);
  atom_table_t *atom_table = &(table->atom_table);
  samp_truth_value_t *assignment = atom_table->assignment[atom_table->current_assignment]; 

  if (fixed_tval(assignment[var])){
    if (sign
	? assigned_false(assignment[var])
	: assigned_true(assignment[var]))
	return -1; //Conflict: The unit literal can't be fixed false,
                           //since it is fixed true
  } else {
    if (is_pos(lit)){
      cprintf(2, "Fixing %"PRId32" to true\n", var);
      assignment[var] = v_fixed_false;
      table->atom_table.num_unfixed_vars--;
      link_propagate(table, pos_lit(var));
    } else {
      cprintf(2, "Fixing %"PRId32" to false\n", var);	  
      assignment[var] = v_fixed_true;
      table->atom_table.num_unfixed_vars--;
      link_propagate(table, neg_lit(var)); 
    }
  }
  return 0;
}


//Fixes the truth values derived from unit and negative weight clauses
int32_t negative_unit_propagate(samp_table_t *table){
  samp_clause_t *link;
  samp_clause_t **link_ptr;
  int32_t i;
  clause_table_t *clause_table;
  int32_t conflict = 0;
  double abs_weight;

  clause_table = &table->clause_table;
  link_ptr = &clause_table->negative_or_unit_clauses;
  link = *link_ptr;
  while (link != NULL) {
    abs_weight = fabs(link->weight);
    if (abs_weight == DBL_MAX || choose() > exp(- abs_weight)) {
      /*
       * This clause is considered alive from now on
       */
      if (link->weight >= 0){ //must be a unit clause
	// FIXME
	conflict = fix_lit_true(table, link->disjunct[0]);
	if (conflict == -1)
	  return -1;
      } else { //we have a negative weight clause
	i = 0;
	while (i < link->numlits) {
	  conflict = fix_lit_false(table, link->disjunct[i++]);
	  if (conflict == -1)
	    return -1;
	}
      }
      link_ptr = &link->link;
      link = link->link;

    } else {
      /*
       * Move the clause to the dead_negative_or_unit_clauses list
       */
      *link_ptr = link->link;
      link->link = clause_table->dead_negative_or_unit_clauses;
      clause_table->dead_negative_or_unit_clauses = link;
      link = *link_ptr;
    }
  }

  return 0; //no conflict
}

/*  The initialization phase for the mcmcsat step.  First place all the clauses
    in the unsat list, and then use scan_unsat_clauses to move them into
    sat_clauses or the watched (sat) lists.  

 */

//assigns a random truth value to unfixed vars and returns number of unfixed vars.
void init_random_assignment(samp_truth_value_t *assignment, int32_t num_vars,
			    int32_t *num_unfixed_vars){
  uint32_t i, num;
  *num_unfixed_vars = 0;
  num = num_vars;
  for (i = 0; i < num_vars; i++){
    if (!fixed_tval(assignment[i])){
      if (choose() < 0.5){
	assignment[i] = v_false;
      } else {
	assignment[i] = v_true;
      }
      (*num_unfixed_vars)++;
    } else {
      num--;
    }
  }
}

void push_clause(samp_clause_t *clause, samp_clause_t **list){
  clause->link = *list;
  *list = clause;
}

void push_negative_unit_clause(clause_table_t *clause_table, uint32_t i){
  clause_table->samp_clauses[i]->link = clause_table->negative_or_unit_clauses;
  clause_table->negative_or_unit_clauses = clause_table->samp_clauses[i];
}

void push_unsat_clause(clause_table_t *clause_table, uint32_t i){
  clause_table->samp_clauses[i]->link = clause_table->unsat_clauses;
  clause_table->unsat_clauses = clause_table->samp_clauses[i];
  clause_table->num_unsat_clauses++;
}

static integer_stack_t clause_var_stack = {0, 0, NULL};

/* 
 * Sets all the clause lists in clause_table to NULL.  
 */
void empty_clause_lists(samp_table_t *table){
  clause_table_t *clause_table = &(table->clause_table);
  atom_table_t *atom_table = &(table->atom_table);

  clause_table->sat_clauses = NULL;
  clause_table->unsat_clauses = NULL;
  clause_table->num_unsat_clauses = 0; 
  clause_table->negative_or_unit_clauses = NULL;
  clause_table->dead_negative_or_unit_clauses = NULL;
  clause_table->dead_clauses = NULL;
  uint32_t i;
  int32_t num_unfixed = 0;
  for (i = 0; i < atom_table->num_vars; i++){
    if (!fixed_tval(atom_table->assignment[atom_table->current_assignment][i])){
      num_unfixed++;
    }
    atom_table->num_unfixed_vars = num_unfixed;
    clause_table->watched[pos_lit(i)] = NULL;
    clause_table->watched[neg_lit(i)] = NULL;
  }
}


/*
 * Pushes each of the hard clauses into either negative_or_unit_clauses
 * if weight is negative or numlits is 1, or into unsat_clauses if weight
 * is non-negative.  Non-hard clauses go into dead_negative_or_unit_clauses
 * or dead_clauses, respectively.  
 */
void init_clause_lists(clause_table_t *clause_table){
  uint32_t i;

  for (i = 0; i < clause_table->num_clauses; i++){
    if (clause_table->samp_clauses[i]->weight < 0){
      if (clause_table->samp_clauses[i]->weight == DBL_MIN){
	push_negative_unit_clause(clause_table, i);
      } else {
	push_clause(clause_table->samp_clauses[i],
		    &(clause_table->dead_negative_or_unit_clauses));
      }
    } else {
      if (clause_table->samp_clauses[i]->numlits == 1){
	if (clause_table->samp_clauses[i]->weight == DBL_MAX){
	  push_negative_unit_clause(clause_table, i);
	} else {
	  push_clause(clause_table->samp_clauses[i],
		    &(clause_table->dead_negative_or_unit_clauses));
	}
      } else {
	if (clause_table->samp_clauses[i]->weight == DBL_MAX){
	  push_unsat_clause(clause_table, i);
	} else {
	  push_clause(clause_table->samp_clauses[i],
		    &(clause_table->dead_clauses));
	}
      }
    }
  }
}

int32_t  init_sample_sat(samp_table_t *table){
  clause_table_t *clause_table = &(table->clause_table);
  atom_table_t *atom_table = &(table->atom_table);
  int32_t conflict = 0;
  if (clause_var_stack.size == 0) {
    init_integer_stack(&(clause_var_stack), 0);
  } else {
    clause_var_stack.top = 0;
  }
  
  empty_clause_lists(table);
  init_clause_lists(clause_table);
  conflict = negative_unit_propagate(table);
  if (conflict == -1) return -1;
  int32_t num_unfixed_vars;
  samp_truth_value_t *assignment = atom_table->assignment[atom_table->current_assignment];
  init_random_assignment(assignment, atom_table->num_vars, &num_unfixed_vars);
  atom_table->num_unfixed_vars = num_unfixed_vars;
  if (scan_unsat_clauses(table) == -1) {
    return -1;
  }
  return process_fixable_stack(table);
}


/* The next step is to define the main samplesat procedure.  Here we have placed
   some clauses among the dead clauses, and are trying to satisfy the live ones.
   The first step is to perform negative_unit propagation.  Then we pick a
   random assignment to the remaining variables.  

   Scan the clauses, if there is a fixed-satisfied, it goes in sat_clauses.
   If there is a fixed-unsat, we have a conflict.
   If there is a sat clause, we place it in watched list.
   If the sat clause has a fixed-propagation, then we mark the variable
   as having a fixed truth value, and move the clause to sat_clauses, while
   link-propagating this truth value.  When scanning, look for unit propagation in
   unsat clause list.  

   We then randomly flip a variable, and move all the resulting satisfied clauses
   to the sat/watched list using the scan_unsat_clauses operation.
   When this is done, if we have no unsat clauses, we stop.
   Otherwise, either pick a random unfixed variable and flip it,
   or pick a random unsat clause and a flip either a random variable or the
   minimal dcost variable and flip it.  Keep flipping until there are no unsat clauses.  
 
   First scan the dead clauses to move the satisfied ones to the appropriate
   lists.  Then move all the unsatisfied clauses to the dead list.
   Then select-and-move the satisfied clauses to the unsat list.
   Pick a random truth assignment.  Then repeat the scan-propagate loop.
   Then selectively, either pick an unfixed variable and flip and scan, or
   pick an unsat clause and, selectively, a variable and flip and scan.  
*/



/*
 * Like init_sample_sat, but takes an existing state and sets it up for a
 * second round of sampling
 */


/*
 * Scan the list of dead clauses:
 * - move all dead clauses that are satisfied by assignment
 *   into an appropriate watched literal list
 */
void restore_sat_dead_clauses(clause_table_t *clause_table, samp_truth_value_t *assignment) {
  int32_t lit, val;
  samp_clause_t *link, *next;
  samp_clause_t **link_ptr;

  link_ptr = &clause_table->dead_clauses;
  link = *link_ptr;
  while (link != NULL){
    val = eval_clause(assignment, link);
    if (val != -1) {
      cprintf(2, "---> restoring dead clause %p (val = %"PRId32")\n", link, val); //BD
      lit = link->disjunct[val];
      link->disjunct[val] = link->disjunct[0];
      link->disjunct[0] = lit;
      next = link->link;
      link->link = clause_table->watched[lit];
      clause_table->watched[lit] = link;
      link = next;
      assert(assigned_true_lit(assignment, clause_table->watched[lit]->disjunct[0]));
    } else {
      cprintf(2, "---> dead clause %p stays dead (val = %"PRId32")\n", link, val);  //BD 
      *link_ptr = link;
      link_ptr = &(link->link);
      link = link->link;
    }
  }
  *link_ptr = NULL;
}


/*
 * Scan the list of dead clauses that are unit clauses or have negative weight
 * - any clause satisfied by assignment is moved into the list of (not dead)
 *   unit or negative weight clauses.
 */
void restore_sat_dead_negative_unit_clauses(clause_table_t *clause_table, samp_truth_value_t *assignment) {
  bool restore;
  samp_clause_t *link, *next;
  samp_clause_t **link_ptr;

  link_ptr = &clause_table->dead_negative_or_unit_clauses;
  link = *link_ptr;
  while (link != NULL){
    if (link->weight < 0) {
      restore = (eval_neg_clause(assignment, link) == -1);
    } else {//unit clause
      restore = (eval_clause(assignment, link) != -1);
    }
    if (restore){
      next = link->link;
      link->link = clause_table->negative_or_unit_clauses;
      clause_table->negative_or_unit_clauses = link;
      link = next;
    } else {
      *link_ptr = link;
      link_ptr = &(link->link);
      link = link->link;
    }
  }
  *link_ptr = NULL;
}


int32_t length_clause_list(samp_clause_t *link){
  int32_t length = 0;
  while (link != NULL){
    link = link->link;
    length++;
  }
  return length;
}

void move_sat_to_unsat_clauses(clause_table_t *clause_table){
  int32_t length = length_clause_list(clause_table->sat_clauses);
  samp_clause_t **link_ptr = &(clause_table->unsat_clauses);
  samp_clause_t *link = clause_table->unsat_clauses;
  while (link != NULL){
    link_ptr = &(link->link);
    link = link->link;
  }
  *link_ptr = clause_table->sat_clauses;
  clause_table->sat_clauses = NULL;
  clause_table->num_unsat_clauses += length;
}



/*
 * Prepare for a new round in mcsat:
 * - select a new set of live clauses
 * - select a random assignment
 * - return -1 if a conflict is detected by unit propagation (in the fixed clauses)
 * - return 0 otherwise
 */
int32_t reset_sample_sat(samp_table_t *table){
  clause_table_t *clause_table;
  atom_table_t *atom_table;
  pred_table_t *pred_table;
  samp_truth_value_t *assignment, *new_assignment;
  uint32_t i, choice;
  int32_t num_unfixed_vars;
  int32_t conflict = 0;

  clause_table = &table->clause_table;
  atom_table = &table->atom_table;
  pred_table = &table->pred_table;
  assignment = atom_table->assignment[atom_table->current_assignment];

  clear_integer_stack(&(table->fixable_stack)); //clear fixable_stack

  /* 
   * move dead clauses that are satisfied into appropriate lists
   * so that they can be selected as live clauses in this round.
   */
  restore_sat_dead_negative_unit_clauses(clause_table, assignment);
  restore_sat_dead_clauses(clause_table, assignment);
  
  /*
   * kill selected satisfied clauses: select which clauses will
   * be live in this round.
   *
   * This is partial: live unit or negative weight clauses are 
   * selected within negative_unit_propagate
   */
  kill_clauses(table);

  /*
   * Flip the assignment vectors:
   * - the current assignment is kept as a backup in case sample_sat fails
   */
  assert(atom_table->current_assignment == 0 || atom_table->current_assignment == 1);
  atom_table->current_assignment ^= 1; // flip low order bit: 1 --> 0, 0 --> 1
  new_assignment = atom_table->assignment[atom_table->current_assignment];  

  /*
   * Pick random assignments for the unfixed vars
   */
  num_unfixed_vars = 0; 
  for (i = 0; i < atom_table->num_vars; i++){
    if (! atom_eatom(i, pred_table, atom_table)) {
      choice = (choose() < 0.5);
      if (choice == 0){
	new_assignment[i] = v_false;
      } else {
	new_assignment[i] = v_true;
      }
      num_unfixed_vars++;
    }
  }

  atom_table->num_unfixed_vars = num_unfixed_vars;

  for (i = 0; i < atom_table->num_vars; i++){
    if (assigned_true(assignment[i]) &&
	assigned_false(new_assignment[i])){
      link_propagate(table, pos_lit(i));
    }
    if (assigned_false(assignment[i]) &&
	assigned_true(new_assignment[i])){
      link_propagate(table, neg_lit(i));
    }
  }

  conflict = negative_unit_propagate(table);
  if (conflict == -1) return -1;

  //move all sat_clauses to unsat_clauses for rescanning
  
  move_sat_to_unsat_clauses(clause_table);

  if (scan_unsat_clauses(table) == -1) {
    return -1;
  }
  return process_fixable_stack(table);
}

int32_t choose_unfixed_variable(samp_truth_value_t *assignment, int32_t num_vars, int32_t num_unfixed_vars){
  uint32_t var, d, y;

  if (num_unfixed_vars == 0) return -1;

  var = random_uint(num_vars);
  if (!fixed_tval(assignment[var])) return var;
  d = 1 + random_uint(num_vars - 1);
  while (gcd32(d, num_vars) != 1) d--;

  y = var;
  do {
    y += d;
    if (y >= num_vars) y -= num_vars;
    assert(var != y);
  } while (fixed_tval(assignment[y]));
  return y;
}


int32_t choose_clause_var(samp_table_t *table,
			  samp_clause_t *link,
			  samp_truth_value_t *assignment,
			  double rvar_probability){
			  
  uint32_t i, var;
  clear_integer_stack(&clause_var_stack);
  for (i = 0; i < link->numlits; i++){
    if (!fixed_tval(assignment[var_of(link->disjunct[i])]))
      push_integer_stack(i, &clause_var_stack);
  } //all unfixed vars are now in clause_var_stack

  double choice = choose();
  if (choice < rvar_probability){//flip a random unfixed variable
    var = random_uint(length_integer_stack(&clause_var_stack));
  } else {
    int32_t dcost = INT32_MAX;
    int32_t vcost = 0;
    var = length_integer_stack(&clause_var_stack);
    for (i = 0; i < length_integer_stack(&clause_var_stack); i++){
      cost_flip_unfixed_variable(table, &vcost,
				 var_of(link->disjunct[nth_integer_stack(i, &clause_var_stack)]));
      if (vcost < dcost){
	dcost = vcost;
	vcost = 0;
	var = i;
      }
    }
  }
  assert (var < length_integer_stack(&clause_var_stack));
  return var_of(link->disjunct[nth_integer_stack(var, &clause_var_stack)]);
}

int32_t sample_sat_body(samp_table_t *table, double sa_probability,
			double samp_temperature, double rvar_probability){
  //Assumed that table is in a valid state with a random assignment.
  //We first decide on simulated annealing vs. walksat.
  clause_table_t *clause_table = &(table->clause_table);
  atom_table_t *atom_table = &(table->atom_table);
  samp_truth_value_t *assignment =
    atom_table->assignment[atom_table->current_assignment];
  int32_t dcost;
  double choice; 
  int32_t var;
  uint32_t clause_position;
  samp_clause_t *link;
  int32_t conflict = 0;

  choice = choose();
  if (clause_table->num_unsat_clauses <= 0 || choice < sa_probability) {
    /*
     * Simulated annealing step
     */
    var = choose_unfixed_variable(assignment, atom_table->num_vars,
				  atom_table->num_unfixed_vars);
    if (var == -1) return 0;
    cost_flip_unfixed_variable(table, &dcost, var);
    if (dcost < 0){
      conflict = flip_unfixed_variable(table, var);
    } else {
      choice = choose();
      if (choice < exp(-dcost/samp_temperature)) {
	conflict = flip_unfixed_variable(table, var);
      }
    }
  } else {
    /*
     * Walksat step
     */
    //choose an unsat clause
    clause_position = random_uint(clause_table->num_unsat_clauses);
    link = clause_table->unsat_clauses;
    while (clause_position != 0){
      link = link->link;
      clause_position--;
    } 
    //link points to chosen clause
    var = choose_clause_var(table, link, assignment, rvar_probability);
    conflict = flip_unfixed_variable(table, var);
  }
  return conflict;
}


static void print_model(samp_table_t *table) {
  samp_truth_value_t *assignment;
  atom_table_t *atom_table;
  int32_t i, num_vars;

  atom_table = &table->atom_table;
  assignment = atom_table->assignment[atom_table->current_assignment];
  num_vars = atom_table->num_vars;
  for (i=0; i<num_vars; i++) {
    print_atom(atom_table->atom[i], table);
    if (assigned_true(assignment[i])) {
      printf(" true\n");
    } else {
      printf(" false\n");
    }
  }
}

void update_query_pmodel(samp_table_t *table) {
  query_instance_table_t *qinst_table;
  samp_query_instance_t *qinst;
  samp_truth_value_t *assignment;
  int32_t num_queries, *apmodel, i, j, k;
  bool fval;

  qinst_table = &table->query_instance_table;
  assignment
    = table->atom_table.assignment[table->atom_table.current_assignment];
  num_queries = qinst_table->num_queries;
  apmodel = table->atom_table.pmodel;
  
  for (i = 0; i < num_queries; i++) {
    // Each query instance is an array of array of literals
    qinst = qinst_table->query_inst[i];
    fval = false;
    // First loop through the conjuncts
    for (j = 0; qinst->lit[j] != NULL; j++) {
      // Now the disjuncts
      for (k = 0; qinst->lit[j][k] != -1; k++) {
	// If any literal is true, skip the rest
	if (assigned_true_lit(assignment, qinst->lit[j][k])) {
	  goto cont;
	}
      }
      // None was true, kick out
      goto done;
    cont: continue;
    }
    // Each conjunct was true, so the assignment is a model - increase the
    // pmodel counter
    if (qinst->lit[j] == NULL) {
	qinst->pmodel++;
    }
  done: continue;
  }
}

void update_pmodel(samp_table_t *table){
  samp_truth_value_t *assignment = table->atom_table.assignment[table->atom_table.current_assignment];
  int32_t num_vars = table->atom_table.num_vars;
  int32_t *pmodel = table->atom_table.pmodel;
  int32_t i; 

  table->atom_table.num_samples++;
  for (i = 0; i < num_vars; i++){
    if (assigned_true(assignment[i])){
      //if (i > 0) {
	//printf("Atom %d was assigned true\n", i);
	//fflush(stdout);
      //}
      pmodel[i]++;
    }
  }

  update_query_pmodel(table);

  // HACK: TEMPORARY FOR DEBUUGGING
  if (get_verbosity_level() > 1) {
    print_model(table);
  }
}


int32_t first_sample_sat(samp_table_t *table, double sa_probability,
			 double samp_temperature, double rvar_probability,
			 uint32_t max_flips){
  int32_t conflict;
  conflict = init_sample_sat(table);
  if (conflict == -1) return -1;
  uint32_t num_flips = max_flips;
  while (table->clause_table.num_unsat_clauses > 0 &&
	 num_flips > 0){
    if (sample_sat_body(table, sa_probability, samp_temperature, rvar_probability) == -1)
      return -1;
    num_flips--;
  }
  if (table->clause_table.num_unsat_clauses > 0){
    mcsat_err("Initialization failed to find a model; increase max_flips\n");
    return -1;
  }
  update_pmodel(table);
  return 0;
}


/*
 * This function is not used??
 */
void move_unsat_to_dead_clauses(clause_table_t *clause_table){
  samp_clause_t **link_ptr = &(clause_table->dead_clauses);
  samp_clause_t *link = clause_table->dead_clauses;
  while (link != NULL){
    link_ptr = &(link->link);
    link = link->link;
  }
  *link_ptr = clause_table->unsat_clauses;
  clause_table->unsat_clauses = NULL;
  clause_table->num_unsat_clauses = 0;
}


/*
 * One round of the mc_sat loop:
 * - select a set of live clauses from the current assignment
 * - compute a new assignment by sample_sat
 */
void sample_sat(samp_table_t *table, double sa_probability,
		double samp_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips) {
  int32_t conflict;
  //assert(valid_table(table));
  conflict = reset_sample_sat(table);
  //assert(valid_table(table));
  uint32_t num_flips = max_flips;
  if (conflict != -1) {
    while (table->clause_table.num_unsat_clauses > 0 && num_flips > 0 && conflict == 0) {
      conflict = sample_sat_body(table, sa_probability, samp_temperature, rvar_probability);
      //assert(valid_table(table));
      num_flips--;
    }
    if (max_extra_flips < num_flips){
      num_flips = max_extra_flips;
    }
    while (num_flips > 0 && conflict == 0){
      conflict = sample_sat_body(table, sa_probability, samp_temperature, rvar_probability);
      //assert(valid_table(table));
      num_flips--;
    }
  }
    
  if (conflict != -1 && table->clause_table.num_unsat_clauses == 0){
    update_pmodel(table);
  } else { 
    /*
     * Sample sat did not find a model (within max_flips)
     * restore the earlier assignment
     */
    if (conflict == -1){
      cprintf(2, "Hit a conflict.\n");
    } else {
      cprintf(2, "Failed to find a model. Consider increasing max_flips - see mcsat help.\n");
    }

    // Flip current_assignment (restore the saved assignment)
    assert(table->atom_table.current_assignment == 0 || table->atom_table.current_assignment == 1);
    table->atom_table.current_assignment ^= 1;
    
    empty_clause_lists(table);
    init_clause_lists(&table->clause_table);
    update_pmodel(table);
  }
}



/*
 * Top-level MCSAT call
 *
 * Parameters for sample sat:
 * - sa_probability = probability of a simulated annealing step
 * - samp_temperature = temperature for simulated annealing
 * - rvar_probability = probability used by a Walksat step:
 *   a Walksat step selects an unsat clause and flips one of its variables
 *   - with probability rvar_probability, that variable is chosen randomly
 *   - with probability 1(-rvar_probability), that variable is the one that
 *     results in minimal increase of the number of unsat clauses.
 * - max_flips = bound on the number of sample_sat steps
 * - max_extra_flips = number of additional (simulated annealing) steps to perform
 *   after a satisfying assignment is found
 *
 * Parameter for mc_sat:
 * - max_samples = number of samples generated
 */
void mc_sat(samp_table_t *table, uint32_t max_samples,
	    double sa_probability, double samp_temperature,
	    double rvar_probability, uint32_t max_flips,
	    uint32_t max_extra_flips){
  int32_t conflict;
  uint32_t i;

  conflict = first_sample_sat(table, sa_probability, samp_temperature,
			      rvar_probability, max_flips);
  if (conflict == -1) {
    mcsat_err("Found conflict in initialization.\n");
    return;
  }

  //  print_state(table, 0);
  //assert(valid_table(table));
  for (i = 0; i < max_samples; i++){
    cprintf(2, "---- sample[%"PRIu32"] ---\n", i);
    sample_sat(table, sa_probability, samp_temperature,
	       rvar_probability, max_flips, max_extra_flips);
    //    print_state(table, i+1);
    //assert(valid_table(table));
  }

  //print_atoms(table);
}

// Given a rule, a -1 terminated array of constants corresponding to the
// variables of the rule, and a samp_table, returns the substituted clause.
// Checks that the constants array is the right length, and the sorts match.
// Returns -1 if there is a problem.
int32_t substit_rule(samp_rule_t *rule,
		     substit_entry_t *substs,
		     samp_table_t *table){
  const_table_t *const_table = &table->const_table;
  sort_table_t *sort_table = &table->sort_table;
  atom_table_t *atom_table = &table->atom_table;
  pred_table_t *pred_table = &table->pred_table;
  array_hmap_pair_t *atom_map;
  int32_t i;
  for(i=0; i<rule->num_vars; i++){
    if (substs[i].const_index == -1) {
      mcsat_err("substit: Not enough constants - %"PRId32" given, %"PRId32" required\n",
		i, rule->num_vars);
      return -1;
    }
    int32_t vsort = rule->vars[i]->sort_index;
    int32_t csort = const_sort_index(substs[i].const_index, const_table);
    if (! subsort_p(csort, vsort, sort_table)) {
      mcsat_err("substit: Constant/variable sorts do not match at %"PRId32"\n", i);
      return -1;
    }
  }
  if(substs[i].const_index != -1) {
    mcsat_err("substit: Too many constants - %"PRId32" given, %"PRId32" required\n",
	      i, rule->num_vars);
      return -1;
  }
  // Everything is OK, do the sustitution
  // We just use the clause_buffer - first make sure it's big enough
  clause_buffer_resize(rule->num_lits);
  for(i=0; i<rule->num_lits; i++) {
    rule_literal_t *lit = rule->literals[i];
    int32_t arity = pred_arity(lit->atom->pred, pred_table);
    samp_atom_t *new_atom = atom_copy(lit->atom, arity);
    int32_t j;
    for(j=0; j<arity; j++){
      int32_t argidx = new_atom->args[j];
      if (argidx < 0) {
	// Have a var, index to var (and hence, const) array is -(argidx + 1)
	new_atom->args[j] = substs[-(argidx + 1)].const_index;
      }
    }
    atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
				    arity+1, //+1 for pred
				    (int32_t *) new_atom);
    int32_t added_atom = add_internal_atom(table, new_atom);
    if (added_atom == -1){
      // This shouldn't happen, but if it does, we need to free up space
      safe_free(new_atom);
      mcsat_err("substit: Bad atom\n");
      return -1;
    }
    //if (atom_map != NULL) {
      // already had the atom - free it
      safe_free(new_atom);
      //}
    clause_buffer.data[i] = lit->neg ? neg_lit(added_atom) : pos_lit(added_atom);
  }
  return add_internal_clause(table, clause_buffer.data, rule->num_lits,
			     rule->weight);
}


static clause_buffer_t rule_atom_buffer = {0, NULL}; 

void rule_atom_buffer_resize(int32_t length){
    if (rule_atom_buffer.data == NULL){
    rule_atom_buffer.data = (int32_t *) safe_malloc(INIT_CLAUSE_SIZE * sizeof(int32_t));
    rule_atom_buffer.size = INIT_CLAUSE_SIZE;
  }
  int32_t size = rule_atom_buffer.size;
  if (size < length){
    if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
      out_of_memory();
    }
    size += size/2;
    rule_atom_buffer.data =
      (int32_t  *) safe_realloc(rule_atom_buffer.data, size * sizeof(int32_t));
    rule_atom_buffer.size = size;
  }
}

/*
 * Given an atom (index), checks if the given possible rule instances should
 * be added to the clauses.  Called from all_rule_instances_rec when lazy
 * flag is set.
 */
bool check_clause_instance(samp_table_t *table,
			   samp_rule_t *rule,
			   int32_t atom_index){//index of atom being activated
  //Use a local buffer to build the literals and check
  //if they are active and not fixed true.
  pred_table_t *pred_table = &(table->pred_table);
  //pred_tbl_t *pred_tbl = &(pred_table->pred_tbl);
  atom_table_t *atom_table = &table->atom_table;
  int32_t predicate, arity, i, j, num_non_false;
  samp_atom_t *atom;
  samp_atom_t *rule_atom; 
  array_hmap_pair_t *atom_map;
  samp_truth_value_t *assignment;

  // FIXME: Need to keep track of non-false atoms - if there is only one,
  // then activate it, if necessary, at the end.  Have to do two loops, the
  // first simply keeps track of the number of non-false atoms - if there is
  // more than one, then we return false.  Otherwise we do the second loop,
  // and activate the inactive atom if there is one.
  assignment = table->atom_table.assignment[table->atom_table.current_assignment];
  num_non_false = 0;
  for (i = 0; i < rule->num_lits; i++){//for each literal
    atom = rule->literals[i]->atom;
    predicate = atom->pred; 
    arity = pred_arity(predicate, pred_table);
    rule_atom_buffer_resize(arity);
    rule_atom = (samp_atom_t *) rule_atom_buffer.data;
    rule_atom->pred = predicate; 
    for (j = 0; j < arity; j++){//copy each instantiated argument
      if (atom->args[j] < 0){
	rule_atom->args[j]
	  = substit_buffer.entries[-(atom->args[j]) - 1].const_index;
      } else {
	rule_atom->args[j] = atom->args[j];
      }
    }
    //find the index of the atom
    atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
				    arity + 1,
				    (int32_t *) rule_atom);
    // check for witness predicate - fixed false if NULL atom_map
    if (atom_map == NULL || assigned_true(assignment[atom_index])) {
      if (num_non_false == 0) {
	num_non_false += 1;
      } else {
	return false;
      }
    }
  }
  assert(num_non_false <= 1);
  // Now we do the flipping
  for (i = 0; i < rule->num_lits; i++){//for each literal
    if (i != atom_index){//if literal is not the one being activated
      atom = rule->literals[i]->atom;
      predicate = atom->pred; 
      arity = pred_arity(predicate, pred_table);
      rule_atom_buffer_resize(arity);
      rule_atom = (samp_atom_t *) rule_atom_buffer.data;
      rule_atom->pred = predicate; 
      for (j = 0; j < arity; j++){//copy each instantiated argument
	if (atom->args[j] < 0){
	  rule_atom->args[j]
	    = substit_buffer.entries[-(atom->args[j]) - 1].const_index;
	} else {
	  rule_atom->args[j] = atom->args[j];
	}
      }
      //find the index of the atom
      atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
				      arity + 1,
				      (int32_t *) rule_atom);
      // check for witness predicate - fixed false if NULL atom_map
      if (atom_map == NULL) {
	activate_atom(table, rule_atom);
	return false;//atom is inactive
      }
      if (rule->literals[i]->neg &&
	  assignment[atom_map->val] == v_fixed_false){
	return false;//literal is fixed true
      }
      if (!rule->literals[i]->neg &&
	  assignment[atom_map->val] == v_fixed_true){
	return false;//literal is fixed true
      }
    }
  }
  return true;
}

// all_rule_instances takes a rule and generates all ground instances
// based on the predicate and its sorts

void all_rule_instances_rec(int32_t vidx,
			    samp_rule_t *rule,
			    samp_table_t *table,
			    int32_t atom_index) {
  if(substit_buffer.entries[vidx].fixed) {
    // Simply do the substitution, or go to the next var
    if (vidx == rule->num_vars - 1) {
      substit_buffer.entries[vidx+1].const_index = -1;
      substit_rule(rule, substit_buffer.entries, table);
    } else {
      all_rule_instances_rec(vidx+1, rule, table, atom_index);
    }
  } else {
    sort_table_t *sort_table = &table->sort_table;
    int32_t vsort = rule->vars[vidx]->sort_index;
    sort_entry_t entry = sort_table->entries[vsort];
    int32_t i;
    for(i=0; i<entry.cardinality; i++){
      substit_buffer.entries[vidx].const_index = entry.constants[i];
      if (vidx == rule->num_vars - 1) {
	substit_buffer.entries[vidx+1].const_index = -1;
	if (!lazy_mcsat() || check_clause_instance(table, rule, atom_index)) {
	  substit_rule(rule, substit_buffer.entries, table);
	  //if (lazy_mcsat()) {
	    // Need to add to watched list or unsat clauses
	    
	}
      } else {
	all_rule_instances_rec(vidx+1, rule, table, atom_index);
      }
    }
  }
}

// Fills in the substit_buffer, creates atoms when arity is reached
void all_pred_instances_rec(int32_t vidx, int32_t *psig, int32_t arity,
			    samp_atom_t *atom, samp_table_t *table) {
  sort_table_t *sort_table = &table->sort_table;
  sort_entry_t entry;
  int32_t i;

  if (vidx == arity) {
    add_internal_atom(table, atom);
  } else {
    entry = sort_table->entries[psig[vidx]];
    for (i = 0; i < entry.cardinality; i++) {
      atom->args[vidx] = entry.constants[i];
      all_pred_instances_rec(vidx+1, psig, arity, atom, table);
    }
  }
}

void all_pred_instances(char *pred, samp_table_t *table) {
  pred_table_t *pred_table = &table->pred_table;
  int32_t predval, predidx, arity, *psig;
  samp_atom_t *atom;

  predidx = pred_index(pred, pred_table);
  predval = pred_val_to_index(predidx);
  arity = pred_arity(predval, pred_table);
  psig = pred_signature(predval, pred_table);
  atom_buffer_resize(arity);
  atom = (samp_atom_t *) atom_buffer.data;
  atom->pred = predval;
  all_pred_instances_rec(0, psig, arity, atom, table);
}

// Eager - called by MCSAT when a new rule is added.
void all_rule_instances(int32_t rule_index, samp_table_t *table) {
  rule_table_t *rule_table = &table->rule_table;
  samp_rule_t *rule = rule_table->samp_rules[rule_index];
  substit_buffer_resize(rule->num_vars);
  int32_t i;
  for(i=0; i<substit_buffer.size; i++){
    substit_buffer.entries[i].fixed = false;
  }
  all_rule_instances_rec(0, rule, table, -1);
}

// Note that substit_buffer.entries has been set up already
void fixed_const_rule_instances(samp_rule_t *rule, samp_table_t *table,
				int32_t atom_index) {
  all_rule_instances_rec(0, rule, table, atom_index);
}

// Called by MCSAT when a new constant is added.
// Generates all new instances of rules involving this constant.
void create_new_const_rule_instances(int32_t constidx, samp_table_t *table,
				     int32_t atom_index) {
  const_table_t *const_table = &(table->const_table);
  //sort_table_t *sort_table = &(table->sort_table);
  rule_table_t *rule_table = &(table->rule_table);
  const_entry_t centry = const_table->entries[constidx];
  int32_t csort = centry.sort_index;
  int32_t i, j, k;
  for(i=0; i<rule_table->num_rules; i++){
    samp_rule_t *rule = rule_table->samp_rules[i];
    for (j=0; j<rule->num_vars; j++) {
      if (rule->vars[j]->sort_index == csort) {
	// Set the substit_buffer - no need to resize, as there are no new rules
	substit_buffer.entries[j].const_index = constidx;
	substit_buffer.entries[j].fixed = true;
	// Make sure all other entries are empty
	for (k=0; k<rule->num_vars; k++) {
	  if (k != j) {
	    substit_buffer.entries[j].const_index = -1;
	    substit_buffer.entries[j].fixed = false;
	  }
	}
	fixed_const_rule_instances(rule, table, atom_index);
      }
    }
  }
}

// Queries are not like rules, as rules are simpler (can treat as separate
// clauses), and have a weight.  substit on rules updates the clause_table,
// whereas substit_query updates the query_instance_table.

int32_t add_subst_query_instance(samp_literal_t **litinst, substit_entry_t *substs,
				 samp_query_t *query, samp_table_t *table) {
  query_instance_table_t *query_instance_table;
  query_table_t *query_table;
  samp_query_instance_t *qinst;
  int32_t i, qindex;
  
  query_table = &table->query_table;
  query_instance_table = &table->query_instance_table;
  for (i = 0; i < query_instance_table->num_queries; i++) {
    if (eql_query_instance_lits(litinst,
				query_instance_table->query_inst[i]->lit)) {
	break;
    }
  }
  qindex = i;
  if (i < query_instance_table->num_queries) {
    // Already have this instance - just associate it with the query
    safe_free(litinst);
  } else {
    query_instance_table_resize(query_instance_table);
    qinst = (samp_query_instance_t *) safe_malloc(sizeof(samp_query_instance_t));
    query_instance_table->query_inst[qindex] = qinst;
    query_instance_table->num_queries += 1;
    for (i = 0; i < query_table->num_queries; i++) {
      if (query_table->query[i] == query) {
	break;
      }
    }
    qinst->query_index = i;
    qinst->sampling_num = table->atom_table.num_samples;
    qinst->pmodel = 0;
    // Copy the substs - used to return the OAA formula instances
    qinst->subst = (int32_t *) safe_malloc(query->num_vars * sizeof(int32_t));
    for (i = 0; i < query->num_vars; i++) {
      qinst->subst[i] = substs[i].const_index;
    }
    qinst->lit = litinst;
  }
  return qindex;
}

int32_t substit_query(samp_query_t *query,
		      substit_entry_t *substs,
		      samp_table_t *table) {
  const_table_t *const_table = &table->const_table;
  sort_table_t *sort_table = &table->sort_table;
  pred_table_t *pred_table = &table->pred_table;
  atom_table_t *atom_table = &table->atom_table;
  samp_atom_t *new_atom;
  int32_t i, j, k, vsort, csort, arity, argidx, added_atom;
  rule_literal_t ***lit;
  samp_literal_t **litinst;
  
  for(i = 0; i < query->num_vars; i++) {
    if (substs[i].const_index == -1) {
      mcsat_err("substit: Not enough constants - %"PRId32" given, %"PRId32" required\n",
		i, query->num_vars);
      return -1;
    }
    vsort = query->vars[i]->sort_index;
    csort = const_sort_index(substs[i].const_index, const_table);
    if (! subsort_p(csort, vsort, sort_table)) {
      mcsat_err("substit: Constant/variable sorts do not match at %"PRId32"\n", i);
      return -1;
    }
  }
  if (substs != NULL && substs[i].const_index != -1) {
    mcsat_err("substit: Too many constants - %"PRId32" given, %"PRId32" required\n",
	      i, query->num_vars);
      return -1;
  }
  // Everything is OK, do the sustitution
  lit = query->literals;
  litinst = (samp_literal_t **)
    safe_malloc((query->num_clauses + 1) * sizeof(samp_literal_t *));
  for (i = 0; i < query->num_clauses; i++) {
    for (j = 0; lit[i][j] != NULL; j++) {}
    litinst[i] = (samp_literal_t *)
      safe_malloc((j + 1) * sizeof(samp_literal_t));
    for (j = 0; lit[i][j] != NULL; j++) {
      arity = pred_arity(lit[i][j]->atom->pred, pred_table);
      new_atom = atom_copy(lit[i][j]->atom, arity);
      for (k = 0; k < arity; k++) {
	argidx = new_atom->args[k];
	if (argidx < 0) {
	  // Have a var, index to var (and hence, const) array is -(argidx + 1)
	  new_atom->args[k] = substs[-(argidx + 1)].const_index;
	}
      }
      if (lazy_mcsat()) {
	array_hmap_pair_t *atom_map;
	atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
					arity+1, //+1 for pred
					(int32_t *) new_atom);
	if (atom_map != NULL) {
	  added_atom = atom_map->val;
	} else {
	  added_atom = -1;
	}
      } else {
	added_atom = add_internal_atom(table, new_atom);
      }
      if (added_atom == -1){
	// This shouldn't happen, but if it does, we need to free up space
	free_samp_atom(new_atom);
	mcsat_err("substit: Bad atom\n");
	return -1;
      }
      free_samp_atom(new_atom);
      litinst[i][j] = lit[i][j]->neg ? neg_lit(added_atom) : pos_lit(added_atom);
    }
    litinst[i][j] = -1; // end-marker - NULL doesn't work
  }
  litinst[i] = NULL;
  return add_subst_query_instance(litinst, substs, query, table);
}

bool check_query_instance(samp_query_t *query, samp_table_t *table) {
  // Similar to (and based on) check_clause_instance
  pred_table_t *pred_table = &(table->pred_table);
  //pred_tbl_t *pred_tbl = &(pred_table->pred_tbl);
  atom_table_t *atom_table = &(table->atom_table);
  int32_t predicate, arity, i, j, k;
  samp_atom_t *atom;
  samp_atom_t *rule_atom; 
  array_hmap_pair_t *atom_map;
  
  for (i = 0; i < query->num_clauses; i++) { //for each clause
    for (j = 0; query->literals[i][j] != NULL; j ++) { //for each literal
      atom = query->literals[i][j]->atom;
      predicate = atom->pred; 
      arity = pred_arity(predicate, pred_table);
      rule_atom_buffer_resize(arity);
      rule_atom = (samp_atom_t *) rule_atom_buffer.data;
      rule_atom->pred = predicate; 
      for (k = 0; k < arity; k++){//copy each instantiated argument
	if (atom->args[k] < 0){
	  rule_atom->args[k]
	    = substit_buffer.entries[-(atom->args[k]) - 1].const_index;
	} else {
	  rule_atom->args[k] = atom->args[k];
	}
      }
      //find the index of the atom
      atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
				      arity + 1,
				      (int32_t *) rule_atom);
      // check for witness predicate - fixed false if NULL atom_map
      if (atom_map == NULL) return false;//atom is inactive
      if (query->literals[i][j]->neg &&
	  (samp_truth_value_t) atom_table->assignment[atom_map->val]
	  == v_fixed_false){
	return false;//literal is fixed true
      }
      if (!query->literals[i][j]->neg &&
	  (samp_truth_value_t) atom_table->assignment[atom_map->val]
	  == v_fixed_true){
	return false;//literal is fixed true
      }
    }
  }
  return true;
}

void all_query_instances_rec(int32_t vidx, samp_query_t *query,
			     samp_table_t *table, int32_t atom_index) {
  sort_table_t *sort_table;
  sort_entry_t entry;
  int32_t i, vsort;
  
  if (substit_buffer.entries[vidx].fixed) {
    // Simply do the substitution, or go to the next var
    if (vidx == query->num_vars - 1) {
      substit_buffer.entries[vidx+1].const_index = -1;
      substit_query(query, substit_buffer.entries, table);
    } else {
      all_query_instances_rec(vidx+1, query, table, atom_index);
    }
  } else {
    sort_table = &table->sort_table;
    vsort = query->vars[vidx]->sort_index;
    entry = sort_table->entries[vsort];
    for(i=0; i<entry.cardinality; i++){
      substit_buffer.entries[vidx].const_index = entry.constants[i];
      if (vidx == query->num_vars - 1) {
	substit_buffer.entries[vidx+1].const_index = -1;
	if (!lazy_mcsat() || check_query_instance(query, table)) {
	  substit_query(query, substit_buffer.entries, table);
	}
      } else {
	all_query_instances_rec(vidx+1, query, table, atom_index);
      }
    }
  }
}

// This is used in the simple case where the samp_query has no variables
// So we simply convert it to a query_instance and add it to the tables.
int32_t samp_query_to_query_instance(samp_query_t *query, samp_table_t *table) {
  pred_table_t *pred_table = &(table->pred_table);
  samp_atom_t *new_atom;
  int32_t i, j, k, arity, argidx, added_atom;
  rule_literal_t ***lit;
  samp_literal_t **litinst;
  
  lit = query->literals;
  litinst = (samp_literal_t **)
    safe_malloc((query->num_clauses + 1) * sizeof(samp_literal_t *));
  for (i = 0; i < query->num_clauses; i++) {
    for (j = 0; lit[i][j] != NULL; j++) {}
    litinst[i] = (samp_literal_t *)
      safe_malloc((j + 1) * sizeof(samp_literal_t));
    for (j = 0; lit[i][j] != NULL; j++) {
      arity = pred_arity(lit[i][j]->atom->pred, pred_table);
      new_atom = atom_copy(lit[i][j]->atom, arity);
      for (k = 0; k < arity; k++) {
	argidx = new_atom->args[k];
      }
      added_atom = add_internal_atom(table, new_atom);
      if (added_atom == -1){
	// This shouldn't happen, but if it does, we need to free up space
	mcsat_err("samp_query_to_query_instance: Bad atom\n");
	return -1;
      }
      litinst[i][j] = lit[i][j]->neg ? neg_lit(added_atom) : pos_lit(added_atom);
    }
    litinst[i][j] = -1; // end-marker - NULL doesn't work
  }
  litinst[i] = NULL;
  return add_subst_query_instance(litinst, NULL, query, table);
}

// Similar to all_rule_instances, but updates query_instance_table
// instead of clause_table
void all_query_instances(samp_query_t *query, samp_table_t *table) {
  int32_t i;
  
  if (query->num_vars == 0) {
    // Already instantiated, but needs to be converted from rule_literals to
    // samp_clauses.  Do this here since we don't know if variables are
    // involved earlier.
    samp_query_to_query_instance(query, table);
  } else {
    substit_buffer_resize(query->num_vars);
    for (i=0; i < substit_buffer.size; i++) {
      substit_buffer.entries[i].fixed = false;
    }
    all_query_instances_rec(0, query, table, -1);
  }
}
  

// Note that substit_buffer.entries has been set up already
void fixed_const_query_instances(samp_query_t *query,
				 samp_table_t *table,
				 int32_t atom_index) {
  all_query_instances_rec(0, query, table, atom_index);
}

void create_new_const_query_instances(int32_t constidx, samp_table_t *table,
				      int32_t atom_index) {
  const_table_t *const_table = &(table->const_table);
  query_table_t *query_table = &(table->query_table);
  samp_query_t *query;
  const_entry_t centry;
  int32_t i, j, k, csort;

  centry = const_table->entries[constidx];
  csort = centry.sort_index;
  for(i=0; i<query_table->num_queries; i++){
    query = query_table->query[i];
    for (j=0; j<query->num_vars; j++) {
      if (query->vars[j]->sort_index == csort) {
	// Set the substit_buffer - first resize if necessary
	substit_buffer_resize(query->num_vars);
	substit_buffer.entries[j].const_index = constidx;
	substit_buffer.entries[j].fixed = true;
	// Make sure all other entries are empty
	for (k=0; k<query->num_vars; k++) {
	  if (k != j) {
	    substit_buffer.entries[j].const_index = -1;
	    substit_buffer.entries[j].fixed = false;
	  }
	}
	fixed_const_query_instances(query, table, atom_index);
      }
    }
  }
}

/*
 * Given a ground atom and a literal from a rule, this returns true if the
 * literal matches - ignores sign, and finds consistent binding for the rule
 * variables - we don't want p(a, b) to match p(V, V).
 * Side effect: substit_buffer has the (partial) matching substitution
 * Of course, this is only useful if this function returns true.
 */
bool match_atom_in_rule_atom(samp_atom_t *atom, rule_literal_t *lit, int32_t arity) {
  // Don't know the rule this literal came from, so we don't actually know
  // the number of variables - we just use the whole substit buffer, which
  // is assumed to be large enough.
  int32_t i, varidx;
  
  // First check that the preds match
  if (atom->pred != lit->atom->pred) {
    return false;
  }
  // Initialize the substit_buffer
  for (i = 0; i < substit_buffer.size; i++) {
    substit_buffer.entries[i].const_index = -1;
    substit_buffer.entries[i].fixed = false;
  }
  // Now go through comparing the args of the atoms
  for (i = 0; i < arity; i++) {
    if (lit->atom->args[i] >= 0) {
      // Constants must be equal
      if (atom->args[i] != lit->atom->args[i]) {
	return false;
      }
    } else {
      // It's a variable
      varidx = -(lit->atom->args[i]) - 1;
      assert(substit_buffer.size > varidx); // Just in case
      if (substit_buffer.entries[varidx].const_index == -1) {
	// Variable not yet set, simply set it
	substit_buffer.entries[varidx].const_index = atom->args[i];
	substit_buffer.entries[varidx].fixed = true;
      } else {
	// Already set, make sure it's the same value
	if (substit_buffer.entries[varidx].const_index != atom->args[i]) {
	  return false;
	}
      }
    }
  }
  return true;
}

// match_atom_in_rule_atom(int32_t atom_index, rule_literal_t *lit,
// 			     int32_t arity, samp_table_t *table,
// 			     bool *rule_inst_true) {
//   // Don't know the rule this literal came from, so we don't actually know
//   // the number of variables - we just use the whole substit buffer, which
//   // is assumed to be large enough.
//   atom_table_t *atom_table = &table->atom_table;
//   samp_atom_t *atom;
//   samp_truth_value_t *assignment;
//   int32_t i, varidx;

//   atom = atom_table->atom[atom_index];
//   printf("Comparing ");
//   print_atom(atom, table);
//   printf(" to ");
//   print_atom(lit->atom, table);
//   printf("\n");
//   // Make sure the preds match
//   if (atom->pred != lit->atom->pred) {
//     cprintf(0, "match_atom_in_rule_atom: preds don't match\n");
//     return false;
//   }
//   // Initialize the substit_buffer
//   for (i = 0; i < substit_buffer.size; i++) {
//     substit_buffer.entries[i].const_index = -1;
//     substit_buffer.entries[i].fixed = false;
//   }
//   // Now go through comparing the args of the atoms
//   for (i = 0; i < arity; i++) {
//     if (lit->atom->args[i] >= 0) {
//       // Constants must be equal
//       if (atom->args[i] != lit->atom->args[i]) {
// 	cprintf(0, "match_atom_in_rule_atom: constant args don't match\n");
// 	return false;
//       }
//     } else {
//       // It's a variable
//       varidx = -(lit->atom->args[i]) - 1;
//       assert(substit_buffer.size > varidx); // Just in case
//       if (substit_buffer.entries[varidx].const_index == -1) {
// 	// Variable not yet set, simply set it
// 	substit_buffer.entries[varidx].const_index = atom->args[i];
// 	substit_buffer.entries[varidx].fixed = true;
//       } else {
// 	// Already set, make sure it's the same value
// 	if (substit_buffer.entries[varidx].const_index != atom->args[i]) {
// 	  cprintf(0, "match_atom_in_rule_atom: var already set nonmatch\n");
// 	  return false;
// 	}
//       }
//     }
//   }
//   return true;
// }

void activate_rules(int32_t atom_index, samp_table_t *table) {
  atom_table_t *atom_table = &table->atom_table;
  pred_table_t *pred_table = &table->pred_table;
  rule_table_t *rule_table = &table->rule_table;
  pred_tbl_t *pred_tbl = &pred_table->pred_tbl;
  samp_atom_t *atom;
  samp_rule_t *rule_entry;
  int32_t i, j, predicate, arity, num_rules, *rules;
  bool rule_inst_true;

  atom = atom_table->atom[atom_index];
  predicate = atom->pred;
  arity = pred_arity(predicate, pred_table);
  num_rules = pred_tbl->entries[predicate].num_rules;
  rules = pred_tbl->entries[predicate].rules;
  for (i = 0; i < num_rules; i++) {
    rule_entry = rule_table->samp_rules[rules[i]];
    rule_inst_true = false;
    for (j = 0; j < rule_entry->num_lits; j++) {
      cprintf(2, "Checking whether to activate rule %"PRId32", literal %"PRId32"\n", rules[i], j);
      if (match_atom_in_rule_atom(atom, rule_entry->literals[j], arity)) {
	//then substit_buffer contains the matching substitution
	fixed_const_rule_instances(rule_entry, table, atom_index);
      }
    }
  }
}

int32_t activate_atom(samp_table_t *table, samp_atom_t *atom){
  //Each rule involving the predicate in the atom is instantiated
  //in all possible extensions of the given atom.  Only the
  //fixed falsifiable instances are retained.

  int32_t atom_index;

  //assert(valid_table(table));
  atom_index = add_internal_atom(table, atom);
  //assert(valid_table(table));
  assert(table->atom_table.num_vars == atom_index + 1);
  while (atom_index < table->atom_table.num_vars) {
    activate_rules(atom_index, table);
    atom_index += 1;
  }
  //assert(valid_table(table));
  return 0;
}


  

/* query_match compares the atom from the samplesat table to the pattern
   atom patom.  They match if the predicates match, and if the variables
   represented by bindings (initially all -1) can be consistently bound.
   Note that this modifies the bindings.
 */
// static bool query_match(samp_atom_t *atom, samp_atom_t *patom,
// 			int32_t *bindings, samp_table_t *table) {
//   pred_table_t *pred_table = &(table->pred_table);
//   int32_t i;
//   int32_t vidx;

//   if (atom->pred != patom->pred) {
//     return false;
//   }
//   i = 0;
//   for (i = 0; i < pred_arity(atom->pred, pred_table); i++) {
//     if (atom->args[i] != patom->args[i]) {
//       if (patom->args[i] >= 0) {
// 	// It's a different constant - no match
// 	return false;
//       } else {
// 	// See if the variable is bound - the index is -(arg + 1)
// 	vidx = -(patom->args[i] + 1);
// 	if (bindings[vidx] >= 0) {
// 	  if (bindings[vidx] != atom->args[i]) {
// 	    return false;
// 	  }
// 	} else {
// 	  bindings[vidx] = atom->args[i];
// 	}
//       }
//     }
//   }
//   return true;
// }

static inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// Based on Bruno's sort_int_array - works on two arrays in parallel
static void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// insertion sort
static void isort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
  uint32_t i, j;
  int32_t x, y;
  double u, v;

  for (i=1; i<n; i++) {
    x = a[i]; u = p[i];
    j = 0;
    while (p[j] > u) j ++;
    while (j < i) {
      y = a[j];  v = p[j];
      a[j] = x;  p[j] = u;
      x = y;     u = v;
      j ++;
    }
    a[j] = x;  p[j] = u;
  }
}

static inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
  if (n <= 10) {
    isort_query_atoms_and_probs(a, p, n);
  } else {
    qsort_query_atoms_and_probs(a, p, n);
  }
}

// quick sort: requires n > 1
static void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
  uint32_t i, j;
  int32_t x, y;
  double u, v;

  // u = random pivot
  i = random_uint(n);
  x = a[i];     u = p[i];

  // swap x and a[0], u and p[0]
  a[i] = a[0];  p[i] = p[0];
  a[0] = x;     p[0] = u;

  i = 0;
  j = n;

  do { j--; } while (p[j] < u);
  do { i++; } while (i <= j && p[i] > u);

  while (i < j) {
    y = a[i];    v = p[i];
    a[i] = a[j]; p[i] = p[j];
    a[j] = y;    p[j] = v;

    do { j--; } while (p[j] < u);
    do { i++; } while (p[i] > u);
  }

  // pivot goes into a[j]
  a[0] = a[j];  p[0] = p[j];
  a[j] = x;     p[j] = u;

  // sort a[0...j-1] and a[j+1 .. n-1]
  sort_query_atoms_and_probs(a, p, j);
  j++;
  sort_query_atoms_and_probs(a + j, p + j, n - j);
}
