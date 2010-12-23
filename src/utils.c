#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include "memalloc.h"
#include "prng.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "utils.h"
  
/*  These copy operations assume that size input to safe_malloc can't 
 *  overflow since there are existing structures of the given size. 
 */

int32_t imax(int32_t i, int32_t j) {return i<j ? j : i;}
int32_t imin(int32_t i, int32_t j) {return i>j ? j : i;}

char * str_copy(char *name){
  int32_t len = strlen(name);
  char * new_name = (char *) safe_malloc(++len * sizeof(char));
  memcpy(new_name, name, len);
  return new_name;
}

samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity){
  arity++;
  int32_t size = arity * sizeof(int32_t);
  samp_atom_t * new_atom = (samp_atom_t *) safe_malloc(size);
  memcpy(new_atom, atom, size);
  return new_atom;
}

int32_t *intarray_copy(int32_t *signature, int32_t length){
  int32_t size = length * sizeof(int32_t);
  int32_t *new_signature = (int32_t *) safe_malloc(size);
  memcpy(new_signature, signature, size);
  return new_signature;
}

/*asserts a db atom to the atoms table and sets its truth value as v_db_true
  A db atom has an evidence predicate which must be negative. 
 */

pred_entry_t *pred_entry(pred_table_t *pred_table, int32_t predicate){
  if (predicate <= 0){
    return &(pred_table->evpred_tbl.entries[-predicate]);
  } else {
    return &(pred_table->pred_tbl.entries[predicate]); 
  }
}

clause_buffer_t clause_buffer = {0, NULL};

void clause_buffer_resize (int32_t length){
  if (clause_buffer.data == NULL){
    clause_buffer.data = (int32_t *) safe_malloc(INIT_CLAUSE_SIZE * sizeof(int32_t));
    clause_buffer.size = INIT_CLAUSE_SIZE;
  }
  int32_t size = clause_buffer.size;
  if (size < length){
    if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
      out_of_memory();
    }
    size += size/2;
    clause_buffer.data =
      (int32_t  *) safe_realloc(clause_buffer.data, size * sizeof(int32_t));
    clause_buffer.size = size;
  }
}

bool assigned_undef(samp_truth_value_t value){
  return (value == v_undef);
}

bool assigned_true(samp_truth_value_t value){
  return (value == v_true ||
	  value == v_db_true ||
	  value == v_fixed_true);
}

bool assigned_false(samp_truth_value_t value){
  return (value == v_false ||
	  value == v_db_false ||
	  value == v_fixed_false);
}

bool assigned_true_lit(samp_truth_value_t *assignment,
		       samp_literal_t lit){
  if (is_pos(lit)){
    return assigned_true(assignment[var_of(lit)]);
  } else {
    return assigned_false(assignment[var_of(lit)]);
  }
}

bool assigned_false_lit(samp_truth_value_t *assignment,
		       samp_literal_t lit){
  if (is_pos(lit)){
    return assigned_false(assignment[var_of(lit)]);
  } else {
    return assigned_true(assignment[var_of(lit)]);
  }
}

bool assigned_fixed_true_lit(samp_truth_value_t *assignment,
		       samp_literal_t lit){
  samp_truth_value_t tval = assignment[var_of(lit)];
  if (is_pos(lit)){
    return (fixed_tval(tval) && assigned_true(tval));
  } else {
    return (fixed_tval(tval) && assigned_false(tval));
  }
}

bool assigned_fixed_false_lit(samp_truth_value_t *assignment,
		       samp_literal_t lit){
  samp_truth_value_t tval = assignment[var_of(lit)];
  if (is_pos(lit)){
    return (fixed_tval(tval) && assigned_false(tval));
  } else {
    return (fixed_tval(tval) && assigned_true(tval));
  }
}


/** Evaluates a clause to false (-1) or to the literal index evaluating to true.
 */
int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause){
  int32_t i;
  for (i = 0; i < clause->numlits; i++){
    if (assigned_true_lit(assignment, clause->disjunct[i]))
      return i;
  }
  return -1;
}

/** Evaluates a clause to -1 if all literals are false, and i if the
   i'th literal is true, in the given assignment. 
*/
int32_t eval_neg_clause(samp_truth_value_t *assignment, samp_clause_t *clause){
  int32_t i;
  for (i = 0; i < clause->numlits; i++){
    if (assigned_true_lit(assignment, clause->disjunct[i]))
      return i;
  }
  return -1;
}

substit_buffer_t substit_buffer = {0, NULL};

// Fill this in later
void substit_buffer_resize(int32_t length) {
  if (substit_buffer.entries == NULL) {
    substit_buffer.entries = (substit_entry_t *)
      safe_malloc(INIT_SUBSTIT_TABLE_SIZE * sizeof(substit_entry_t));
    substit_buffer.size = INIT_SUBSTIT_TABLE_SIZE;
  }
  int32_t size = substit_buffer.size;
  if (size < length){
    if (MAXSIZE(sizeof(substit_entry_t), 0) - size <= size/2){
      out_of_memory();
    }
    size += size/2;
    substit_buffer.entries = (substit_entry_t *)
      safe_realloc(substit_buffer.entries, size * sizeof(substit_entry_t));
    substit_buffer.size = size;
  }
}

char *const_sort_name(int32_t const_idx, samp_table_t *table) {
  sort_table_t *sort_table = &table->sort_table;
  const_table_t *const_table = &table->const_table;
  int32_t const_sig;

  const_sig = const_table->entries[const_idx].sort_index;
  return sort_table->entries[const_sig].name;
}

void free_strings (char **string) {
  int32_t i;

  i = 0;
  if (string != NULL) {
    while (string[i] != NULL) {
      if (strcmp(string[i], "") != 0) {
	safe_free(string[i]);
      }
      i++;
    }
  }
}

bool subsort_p(int32_t sig1, int32_t sig2, sort_table_t *sort_table) {
  int32_t i;
  sort_entry_t *ent1;
  if (sig1 == sig2) {
    return true;
  }
  ent1 = &sort_table->entries[sig1];
  if (ent1->supersorts != NULL) {
    for (i = 0; ent1->supersorts[i] != -1; i++) {
      if (ent1->supersorts[i] == sig2) {
	return true;
      }
    }
  }
  return false;
}

// Return a (not necessarily unique) least common supersort, or -1 if there is none
int32_t least_common_supersort(int32_t sig1, int32_t sig2, sort_table_t *sort_table) {
  int32_t i, j, lcs;
  sort_entry_t *ent1, *ent2;
  
  if (sig1 == sig2) {
    return sig1;
  }
  // First check if one is a subsort of the other
  if (subsort_p(sig1, sig2, sort_table)) {
    return sig2;
  } else if (subsort_p(sig2, sig1, sort_table)) {
    return sig1;
  } else {
    ent1 = &sort_table->entries[sig1];
    ent2 = &sort_table->entries[sig2];
    // Now the tricky one
    if (ent1->supersorts == NULL || ent2->supersorts == NULL) {
      return -1;
    }
    lcs = -1;
    for (i = 0; ent1->supersorts[i] != -1; i++) {
      for (j = 0; ent2->supersorts[j] != -1; j++) {
	if (ent1->supersorts[i] == ent2->supersorts[j]) {
	  if (lcs == -1 || subsort_p(ent1->supersorts[i], lcs, sort_table)) {
	    lcs = ent1->supersorts[i];
	  }
	}
      }
    }
    return lcs;
  }
}

// Return a (not necessarily unique) greatest common subsort, or -1 if there is none
int32_t greatest_common_subsort(int32_t sig1, int32_t sig2, sort_table_t *sort_table) {
  int32_t i, j, gcs;
  sort_entry_t *ent1, *ent2;

  if (sig1 == sig2) {
    return sig1;
  }
  // First check if one is a subsort of the other
  if (subsort_p(sig1, sig2, sort_table)) {
    return sig1;
  } else if (subsort_p(sig2, sig1, sort_table)) {
    return sig2;
  } else {
    ent1 = &sort_table->entries[sig1];
    ent2 = &sort_table->entries[sig2];
    // Now the tricky one
    if (ent1->subsorts == NULL || ent2->subsorts == NULL) {
      return -1;
    }
    gcs = -1;
    for (i = 0; ent1->subsorts[i] != -1; i++) {
      for (j = 0; ent2->subsorts[j] != -1; j++) {
	if (ent1->subsorts[i] == ent2->subsorts[j]) {
	  if (gcs == -1 || subsort_p(gcs, ent1->subsorts[i], sort_table)) {
	    gcs = ent1->subsorts[i];
	  }
	}
      }
    }
    return gcs;
  }
}
      
samp_atom_t *rule_atom_to_samp_atom(rule_atom_t *ratom, pred_table_t *pred_table) {
  int32_t i;
  int32_t arity = ratom->builtinop == 0
    ? pred_arity(ratom->pred, pred_table)
    : builtin_arity(ratom->builtinop);
  samp_atom_t *satom = (samp_atom_t *) safe_malloc((arity+1) * sizeof(int32_t));
  satom->pred = ratom->pred;
  for (i = 0; i < arity; i++) {
    satom->args[i] = ratom->args[i].value;
  }
  return satom;
}
