#include <string.h>
#include "memalloc.h"
#include "utils.h"
#include "input.h"
#include "tables.h"

void init_sort_table(sort_table_t *sort_table){
  int32_t size = INIT_SORT_TABLE_SIZE;
  if (size >= MAXSIZE(sizeof(sort_entry_t), 0)){
    out_of_memory();
  }
  sort_table->size = size;
  sort_table->num_sorts = 0;
  sort_table->entries = (sort_entry_t*)
    safe_malloc(size*sizeof(sort_entry_t));
  /*  for (i=0; i<size; i++){//this is not really needed
    sort_table->entries[i].cardinality = 0;
    sort_table->entries[i].name = NULL;
  }
    */
  init_stbl(&(sort_table->sort_name_index), 0);
}

void reset_sort_table(sort_table_t *sort_table){
  sort_table->num_sorts = 0;
  reset_stbl(&(sort_table->sort_name_index));
}

static void sort_table_resize(sort_table_t *sort_table, uint32_t n){
  if (n >= MAXSIZE(sizeof(sort_entry_t), 0)){
    out_of_memory();
  }
  sort_table->entries = (sort_entry_t *) safe_realloc(sort_table->entries,
					     n * sizeof(sort_entry_t));
  sort_table->size = n; 
}

void add_sort(sort_table_t *sort_table, char *name) {
  int32_t index = stbl_find(&(sort_table->sort_name_index), name);
  if (index == -1){//sort is not in the symbol table
    int32_t n = sort_table->num_sorts;
    if (n >= sort_table->size){
      sort_table_resize(sort_table, n + (n/2));
    }
    name = str_copy(name);
    sort_table->entries[n].name = name;
    sort_table->entries[n].size = INIT_SORT_CONST_SIZE;
    sort_table->entries[n].cardinality = 0;
    sort_table->entries[n].constants =
      (int32_t *) safe_malloc(INIT_SORT_CONST_SIZE * sizeof(int32_t));
    sort_table->entries[n].subsorts = NULL;
    sort_table->entries[n].supersorts = NULL;
    sort_table->num_sorts++;
    stbl_add(&(sort_table->sort_name_index), name, n);
  }
}

int32_t sort_name_index(char *name,
			 sort_table_t *sort_table){
  return stbl_find(&(sort_table->sort_name_index), name);
}

int32_t *sort_signature(char **in_signature,
			int32_t arity,
			sort_table_t *sort_table){
  int32_t i;
  int32_t sort_index; 
  int32_t * signature = (int32_t *) safe_malloc(arity * sizeof(int32_t));
  for (i=0; i<arity; i++){
    sort_index = sort_name_index(in_signature[i], sort_table);
    if (sort_index == -1){
      safe_free(signature);
      return NULL;
    } else {
      signature[i] = sort_index;
    }
  }
  return signature;
}

void add_new_supersort(sort_entry_t *entry, int32_t supidx) {
  int32_t size;
  
  if (entry->supersorts == NULL) {
    entry->supersorts = (int32_t *) safe_malloc(2 * sizeof(int32_t));
    entry->supersorts[0] = supidx;
    entry->supersorts[1] = -1;
  } else {
    for (size = 0; entry->supersorts[size] != -1; size++) { }
    entry->supersorts = (int32_t *)
      safe_realloc(entry->supersorts, (size+2) * sizeof(int32_t));
    entry->supersorts[size] = supidx;
    entry->supersorts[size+1] = -1;
  }
}

void add_new_subsort(sort_entry_t *entry, int32_t subidx) {
  int32_t size;
  
  if (entry->subsorts == NULL) {
    entry->subsorts = (int32_t *) safe_malloc(2 * sizeof(int32_t));
    entry->subsorts[0] = subidx;
    entry->subsorts[1] = -1;
  } else {
    for (size = 0; entry->subsorts[size] != -1; size++) { }
    entry->subsorts = (int32_t *)
      safe_realloc(entry->subsorts, (size+2) * sizeof(int32_t));
    entry->subsorts[size] = subidx;
    entry->subsorts[size+1] = -1;
  }
}

void add_subsort_consts_to_supersort(int32_t subidx, int32_t supidx,
				     sort_table_t *sort_table) {
  sort_entry_t subentry, supentry;
  int32_t i, j;
  bool foundit;

  subentry = sort_table->entries[subidx];
  supentry = sort_table->entries[supidx];
  for (i = 0; i < subentry.cardinality; i++) {
    foundit = false;
    for (j = 0; j < supentry.cardinality; j++) {
      if (subentry.constants[i] == supentry.constants[j]) {
	foundit = true;
	break;
      }
    }
    if (! foundit) {
      // Note that this alao adds to the supersorts of supentry
      add_const_to_sort(subentry.constants[i], supidx, sort_table);
    }
  }
}

void add_subsort(sort_table_t *sort_table, char *subsort, char *supersort) {
  int32_t i, j, subidx, supidx;
  sort_entry_t *subentry, *supentry;
  bool foundit;

  add_sort(sort_table, subsort); // Does nothing if already there
  add_sort(sort_table, supersort);
  
  subidx = stbl_find(&(sort_table->sort_name_index), subsort);
  supidx = stbl_find(&(sort_table->sort_name_index), supersort);
  subentry = &sort_table->entries[subidx];
  supentry = &sort_table->entries[supidx];

  // Check if this is unnecessary
  if (subidx == supidx) {
    printf("Declaring %s as a subsort of itself has no effect\n", subsort);
    return;
  }
  // Note that we only need to test one direction
  if (supentry->subsorts != NULL) {
    for (i = 0; supentry->subsorts[i] != -1; i++) {
      if (supentry->subsorts[i] == subidx) {
	printf("%s is already a subsort of %s; this delaration has no effect\n",
	       subsort, supersort);
	return;
      }
    }
  }
  // Now check for cirularities
  if (supentry->supersorts != NULL) {
    for (i = 0; supentry->supersorts[i] != -1; i++) {
      if (supentry->supersorts[i] == subidx) {
	printf("%s is a already supersort of %s; cannot be made a subsort\n",
	       subsort, supersort);
	return;
      }
    }
  }
  // Need to check both ways
  if (subentry->subsorts != NULL) {
    for (i = 0; subentry->subsorts[i] != -1; i++) {
      if (subentry->subsorts[i] == supidx) {
	printf("%s is a already subsort of %s; cannot be made a supersort\n",
	       supersort, subsort);
	return;
      }
    }
  }
  // Everything's OK, update the subentry supersorts and the supentry subsorts
  if (supentry->supersorts != NULL) {
    for (i = 0; supentry->supersorts[i] != -1; i++) {
      foundit = false;
      if (subentry->supersorts != NULL) {
	for (j = 0; subentry->supersorts[j] != -1; j++) {
	  if (supentry->supersorts[i] == subentry->supersorts[j]) {
	    foundit = true;
	    break;
	  }
	}
      }
      if (!foundit) {
	add_new_supersort(subentry, supentry->supersorts[i]);
      }
    }
  }
  add_new_supersort(subentry, supidx);
  if (subentry->subsorts != NULL) {
    for (i = 0; subentry->subsorts[i] != -1; i++) {
      foundit = false;
      if (supentry->subsorts != NULL) {
	for (j = 0; supentry->subsorts[j] != -1; j++) {
	  if (subentry->subsorts[i] == supentry->subsorts[j]) {
	    foundit = true;
	    break;
	  }
	}
      }
      if (!foundit) {
	add_new_subsort(supentry, subentry->subsorts[i]);
      }
    }
  }
  add_new_subsort(supentry, subidx);
  assert(supentry->subsorts != NULL);
  assert(subentry->supersorts != NULL);
  // Finally, add constants of the subsort to the supersort
  add_subsort_consts_to_supersort(subidx, supidx, sort_table);
}

/* The same functionality is repeated for constants
 */

void init_const_table(const_table_t *const_table){
  int32_t size = INIT_CONST_TABLE_SIZE;
  if (size >= MAXSIZE(sizeof(const_entry_t), 0)){
    out_of_memory();
  }
  const_table->size = size;
  const_table->num_consts = 0;
  const_table->entries = (const_entry_t*)
    safe_malloc(size*sizeof(const_entry_t));
  init_stbl(&(const_table->const_name_index), 0);
}

static void const_table_resize(const_table_t *const_table, uint32_t n){
  if (n >= MAXSIZE(sizeof(const_entry_t), 0)){
    out_of_memory();
  }
  const_table->entries = (const_entry_t *) safe_realloc(const_table->entries,
					     n * sizeof(const_entry_t));
  const_table->size = n; 
}

void add_const_to_sort(int32_t const_index,
			      int32_t sort_index,
			      sort_table_t *sort_table){
  int32_t i, j;
  bool foundit;
  sort_entry_t *entry, *supentry;
  
  entry = &sort_table->entries[sort_index];  
  if (entry->size == entry->cardinality){
    if (MAXSIZE(sizeof(int32_t), 0) - entry->size < entry->size/2){
      out_of_memory();
    }
    entry->size += entry->size/2;
    entry->constants = (int32_t *) safe_realloc(entry->constants,
						entry->size * sizeof(int32_t));
    }
  entry->constants[entry->cardinality++] = const_index;
  // Added it to this sort, now do its supersorts
  if (entry->supersorts != NULL) {    
    for (i = 0; entry->supersorts[i] != -1; i++) {
      supentry = &sort_table->entries[entry->supersorts[i]];
      foundit = false;
      for (j = 0; j < supentry->cardinality; j++) {
	if (supentry->constants[j] == const_index) {
	  foundit = true;
	  break;
	}
      }
      if (!foundit) {
	add_const_to_sort(const_index, entry->supersorts[i], sort_table);
      }
    }
  }
}

// Internal form of add_const, checks if const already exists, and verifies
// its sort if it does.  Returns 0 if it is newly added, 1 if it is already
// there, and -1 if there is an error (e.g., sort mismatch).
int32_t add_const_internal (char *name, int32_t sort_index,
			    samp_table_t *table) {
  const_table_t *const_table = &(table->const_table);
  sort_table_t *sort_table = &(table->sort_table);
  int32_t const_index, num_consts;

  if (sort_index < 0 || sort_index >= sort_table->num_sorts) {
    printf("Invalid sort_index");
    return -1;
  }
  const_index = stbl_find(&(const_table->const_name_index), name);
  if (const_index == -1){
    //const is not in the symbol table
    num_consts = const_table->num_consts;
    if (num_consts >= const_table->size) {
      const_table_resize(const_table, num_consts + (num_consts/2));
    }
    name = str_copy(name);
    const_table->entries[num_consts].name = name;
    const_table->entries[num_consts].sort_index = sort_index;
    add_const_to_sort(num_consts, sort_index, sort_table);
    const_table->num_consts++;
    stbl_add(&(const_table->const_name_index), name, num_consts);
    return 0;
  } else {
    // const is in the symbol table - check the sorts
    if (sort_index == const_table->entries[const_index].sort_index) {
      return 1;
    } else {
      printf("Const %s of sort %s may not be redeclared to sort %s\n",
	     name,
	     sort_table->entries[const_table->entries[const_index].sort_index].name,
	     sort_table->entries[sort_index].name);
      return -1;
    }
  }
}

// Adds a constant to the const_table.  If the constant already exists,
// simply prints a message, but does not check that the signature is correct.
int32_t add_const(char *name, char *sort_name, samp_table_t *table) {
  sort_table_t *sort_table = &table->sort_table;
  int32_t res;
  
  int32_t sort_index = sort_name_index(sort_name, sort_table);
  if (sort_index == -1){
    printf("Sort name %s has not been declared.\n", sort_name);
    return -1;
  }
  
  //int32_t index = stbl_find(&(const_table->const_name_index), name);
  res = add_const_internal(name, sort_index, table);
  // res is 0 if new, 1 if exists, -1 if there and different sort
  if (res == 0) {
    return 0;
  } else {
    if (res == 1) {
      printf("Constant %s already exists\n", name);
    }
    return res;
  }
}

int32_t const_index(char *name,
		    const_table_t *const_table){
  return stbl_find(&(const_table->const_name_index), name);
}

char *const_name(int32_t const_index, const_table_t *const_table) {
  return const_table->entries[const_index].name;
}

int32_t const_sort_index(int32_t const_index,
			 const_table_t *const_table){
  return const_table->entries[const_index].sort_index;
}

/* The same functionality is repeated for variables
 */

void init_var_table(var_table_t *var_table){
  int32_t size = INIT_VAR_TABLE_SIZE;
  if (size >= MAXSIZE(sizeof(var_entry_t), 0)){
    out_of_memory();
  }
  var_table->size = size;
  var_table->num_vars = 0;
  var_table->entries = (var_entry_t*)
    safe_malloc(size*sizeof(var_entry_t));
  init_stbl(&(var_table->var_name_index), 0);
}

static void var_table_resize(var_table_t *var_table, uint32_t n){
  if (n >= MAXSIZE(sizeof(var_entry_t), 0)){
    out_of_memory();
  }
  var_table->entries = (var_entry_t *) safe_realloc(var_table->entries,
					     n * sizeof(var_entry_t));
  var_table->size = n; 
}

int32_t add_var(var_table_t *var_table,
	       char *name,
	       sort_table_t *sort_table,
	       char * sort_name){
  int32_t sort_index = sort_name_index(sort_name, sort_table);
  if (sort_index == -1){
    printf("\nSort name %s has not been declared.", sort_name);
    return -1;
  }
  int32_t index = stbl_find(&(var_table->var_name_index), name);
  if (index == -1){//var is not in the symbol table
    int32_t n = var_table->num_vars;
    if (n >= var_table->size){
      var_table_resize(var_table, n + (n/2));
    }
    name = str_copy(name);
    var_table->entries[n].name = name;
    var_table->entries[n].sort_index = sort_index;
    var_table->num_vars++;
    stbl_add(&(var_table->var_name_index), name, n);
    return 0;
  } else {
    printf("\nVariable %s already exists", name);
    return -1;
  }
}

int32_t var_index(char *name,
		    var_table_t *var_table){
  return stbl_find(&(var_table->var_name_index), name);
}

char *var_name(int32_t var_index,
		  var_table_t *var_table){
  return var_table->entries[var_index].name;
}

/* The same routines for adding predicate symbols
 */
bool pred_epred(int32_t predicate){
  return (predicate <= 0);
}

int32_t pred_val_to_index(int32_t val){
  if (val & 1){
    return ((val-1)/2);
  } else {
    return -(val/2);
  }
}

bool atom_eatom(int32_t atom_id, pred_table_t *pred_table, atom_table_t *atom_table){
  return pred_epred(atom_table->atom[atom_id]->pred);
}


int32_t pred_arity(int32_t predicate, pred_table_t *pred_table){
  int32_t arity;
  if (predicate <= 0){
    arity = pred_table->evpred_tbl.entries[-predicate].arity;
      } else {
    arity = pred_table->pred_tbl.entries[predicate].arity;
  }
  return arity;
}

int32_t *pred_signature(int32_t predicate, pred_table_t *pred_table){
  if (predicate <= 0){
    return pred_table->evpred_tbl.entries[-predicate].signature;
  } else {
    return pred_table->pred_tbl.entries[predicate].signature;
  }
}

int32_t pred_index(char * predicate, pred_table_t *pred_table){
  return stbl_find(&(pred_table->pred_name_index), predicate);
}

char * pred_name(int32_t pred, pred_table_t *pred_table){
  if (pred <= 0){
    return pred_table->evpred_tbl.entries[-pred].name;
  } else
    return pred_table->pred_tbl.entries[pred].name;
}

static char * truepred = "true"; //the true evidence predicate occupies the 0 slot

void init_pred_table(pred_table_t *pred_table){
  int32_t size = INIT_PRED_TABLE_SIZE;
  if (size >= MAXSIZE(sizeof(pred_entry_t), 0)){
    out_of_memory();
  }
  pred_table->evpred_tbl.size = size;
  pred_table->evpred_tbl.num_preds = 1;
  pred_table->evpred_tbl.entries = (pred_entry_t*)
    safe_malloc(size*sizeof(pred_entry_t));
  pred_table->pred_tbl.size = size;
  pred_table->pred_tbl.num_preds = 1;
  pred_table->pred_tbl.entries = (pred_entry_t*)
    safe_malloc(size*sizeof(pred_entry_t));
  init_stbl(&(pred_table->pred_name_index), size);
  stbl_add(&(pred_table->pred_name_index), truepred, 0);
  pred_table->pred_tbl.entries[0].arity = 0;
  pred_table->pred_tbl.entries[0].name = truepred;
  pred_table->pred_tbl.entries[0].signature = NULL;
  pred_table->pred_tbl.entries[0].size_atoms = 0;
  pred_table->pred_tbl.entries[0].num_atoms = 0;
  pred_table->pred_tbl.entries[0].atoms = NULL;
  pred_table->evpred_tbl.entries[0].arity = 0;
  pred_table->evpred_tbl.entries[0].name = truepred;
  pred_table->evpred_tbl.entries[0].signature = NULL;
  pred_table->evpred_tbl.entries[0].size_atoms = 0;
  pred_table->evpred_tbl.entries[0].num_atoms = 0;
  pred_table->evpred_tbl.entries[0].atoms = NULL;

}

static void pred_tbl_resize(pred_tbl_t *pred_tbl){//call this extend, not resize
  int32_t size = pred_tbl->size;
  int32_t num_preds = pred_tbl->num_preds;
  if (num_preds < size) return;
  if (MAXSIZE(sizeof(pred_entry_t), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  pred_tbl->entries = (pred_entry_t *) safe_realloc(pred_tbl->entries,
					     size * sizeof(pred_entry_t));
  pred_tbl->size = size; 
}

int32_t add_pred(pred_table_t *pred_table,
		 char *name,
		 bool evidence,
		 int32_t arity,
		 sort_table_t *sort_table,
		 char **in_signature){
  int32_t * signature = sort_signature(in_signature, arity, sort_table);
  if (signature == NULL && in_signature != NULL){
    printf("\nInput signature contains undeclared sort.");
    return -1;
  }
  int32_t index = stbl_find(&(pred_table->pred_name_index), name);
  pred_tbl_t *pred_tbl;
  if (strlen(name) == 0){
    printf("\nEmpty predicate name is not allowed.");
    return -1;
  }
  if (index == -1) { //pred is not in the symbol table  (Change to stbl_rec_t *)
    if (evidence) {
      pred_tbl = &(pred_table->evpred_tbl);
      index = 2 * pred_tbl->num_preds;
    } else {
      pred_tbl = &(pred_table->pred_tbl);
      index = (2 * pred_tbl->num_preds) + 1;
    }

    pred_tbl_resize(pred_tbl);

    int32_t n = pred_tbl->num_preds;
    name = str_copy(name);
    pred_tbl->entries[n].name = name;
    pred_tbl->entries[n].signature = signature;
    pred_tbl->entries[n].arity = arity;
    pred_tbl->entries[n].size_atoms = 0;
    pred_tbl->entries[n].num_atoms = 0;
    pred_tbl->entries[n].atoms = NULL;
    pred_tbl->entries[n].size_rules = 0;
    pred_tbl->entries[n].num_rules = 0;
    pred_tbl->entries[n].rules = NULL;
    pred_tbl->num_preds++;
    stbl_add(&(pred_table->pred_name_index), name, index); //note index is never -1
    //printf("\nAdded predicate index %"PRId32" with name %s, hashindex %"PRId32", arity %"PRId32", and signature:",
    //   n, name, index, arity);
    //printf("\nhashindex[%s] = %"PRId32"",
    //   pred_tbl->entries[n].name,
    //   pred_index(pred_tbl->entries[n].name, pred_table));
    //printf("\n(");
    //int32_t i;
    //for (i=0; i < arity; i++){
    //printf("%"PRId32"", signature[i]);
    //}
    //printf(")");
    return 0;
  } else {
    return -1;
  }
}

void init_atom_table(atom_table_t *table) {
  uint32_t i;

  table->size = INIT_ATOM_TABLE_SIZE;
  table->num_vars = 0; //atoms are positive
  table->num_unfixed_vars = 0;
  if (table->size >= MAXSIZE(sizeof(samp_atom_t *), 0)){
    out_of_memory();
  }
  table->atom = (samp_atom_t **) safe_malloc(table->size * sizeof(samp_atom_t *));
  table->assignment[0] = (samp_truth_value_t *)
    safe_malloc(table->size * sizeof(samp_truth_value_t));
  table->assignment[1] = (samp_truth_value_t *)
    safe_malloc(table->size * sizeof(samp_truth_value_t));
  table->current_assignment = 0;
  table->pmodel = (int32_t *) safe_malloc(table->size * sizeof(int32_t));
  table->sampling_nums = (int32_t *) safe_malloc(table->size * sizeof(int32_t));
  
  for (i = 0; i < table->size; i++) {
    table->pmodel[i] = 0;//was -1
  }
  table->num_samples = 0;
  init_array_hmap(&(table->atom_var_hash), ARRAY_HMAP_DEFAULT_SIZE);
  //  table->entries[0].atom = (samp_atom_t *) safe_malloc(sizeof(samp_atom_t));
  //  table->entries[0].atom->pred = 0;
}


/*
 * When atom_table is resized, the watched literals must also be resized. 
 */
void atom_table_resize(atom_table_t *atom_table, clause_table_t *clause_table){
  int32_t size, num_vars, i;

  num_vars = atom_table->num_vars;
  size = atom_table->size;
  if (num_vars < size) return;

  if (MAXSIZE(sizeof(samp_atom_t *), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  atom_table->atom = (samp_atom_t **)
    safe_realloc(atom_table->atom, size * sizeof(samp_atom_t *));
  atom_table->sampling_nums = (int32_t *)
    safe_realloc(atom_table->sampling_nums, size * sizeof(int32_t));
  atom_table->assignment[0] = (samp_truth_value_t *) 
    safe_realloc(atom_table->assignment[0], size * sizeof(samp_truth_value_t));
  atom_table->assignment[1] = (samp_truth_value_t *) 
    safe_realloc(atom_table->assignment[1], size * sizeof(samp_truth_value_t));
  atom_table->pmodel = (int32_t *)
    safe_realloc(atom_table->pmodel, size * sizeof(int32_t));

  if (MAXSIZE(sizeof(samp_clause_t *), 0) - size <= size){
    out_of_memory();
  }
  clause_table->watched = (samp_clause_t **) safe_realloc(clause_table->watched, 2*size*sizeof(samp_clause_t *));

  for (i = atom_table->size; i < size; i++) {
    atom_table->pmodel[i] = 0;//was -1
  }
  atom_table->size = size;
}


void init_clause_table(clause_table_t *table){
  table->size = INIT_CLAUSE_TABLE_SIZE;
  table->num_clauses = 0;
    if (table->size >= MAXSIZE(sizeof(samp_clause_t *), 0)){
    out_of_memory();
  }
  table->samp_clauses =
    (samp_clause_t **) safe_malloc(table->size * sizeof(samp_clause_t *));
  init_array_hmap(&(table->clause_hash), ARRAY_HMAP_DEFAULT_SIZE);
  table->watched = (samp_clause_t **)
    safe_malloc(sizeof(int32_t) +
		(2 * INIT_ATOM_TABLE_SIZE * sizeof(intptr_t)));
  table->dead_clauses = NULL;
  table->unsat_clauses = NULL;
  table->sat_clauses = NULL;
  table->negative_or_unit_clauses = NULL;
  table->dead_negative_or_unit_clauses = NULL; 
  table->num_unsat_clauses = 0;
}

/*
 * Check whether there's room for one more clause in clause_table.
 * - if not, make the table larger
 */
void clause_table_resize(clause_table_t *clause_table){
  int32_t size = clause_table->size;
  int32_t num_clauses = clause_table->num_clauses;
  if (num_clauses + 1 < size) return;
  if (MAXSIZE(sizeof(samp_clause_t *), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  clause_table->samp_clauses = 
    (samp_clause_t **) safe_realloc(clause_table->samp_clauses, size * sizeof(samp_clause_t *));
  clause_table->size = size; 
}

void init_query_table(query_table_t *table) {
  table->size = INIT_QUERY_TABLE_SIZE;
  table->num_queries = 0;
    if (table->size >= MAXSIZE(sizeof(samp_query_t *), 0)){
    out_of_memory();
  }
  table->query =
    (samp_query_t **) safe_malloc(table->size * sizeof(samp_query_t *));
}

void query_table_resize(query_table_t *table) {
  int32_t size = table->size;
  int32_t num_queries = table->num_queries;
  if (num_queries + 1 < size) return;
  if (MAXSIZE(sizeof(samp_query_t *), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  table->query = (samp_query_t **)
    safe_realloc(table->query, size * sizeof(samp_query_t *));
  table->size = size; 
}

void init_query_instance_table(query_instance_table_t *table) {
  table->size = INIT_QUERY_INSTANCE_TABLE_SIZE;
  table->num_queries = 0;
  if (table->size >= MAXSIZE(sizeof(samp_query_instance_t *), 0)) {
    out_of_memory();
  }
  table->query_inst = (samp_query_instance_t **)
    safe_malloc(table->size * sizeof(samp_query_instance_t *));
}

void query_instance_table_resize(query_instance_table_t *table) {
  int32_t size = table->size;
  int32_t num_queries = table->num_queries;
  if (num_queries + 1 < size) return;
  if (MAXSIZE(sizeof(samp_query_instance_t *), 0) - size <= (size/2)) {
    out_of_memory();
  }
  size += size/2;
  table->query_inst = (samp_query_instance_t **)
    safe_realloc(table->query_inst, size * sizeof(samp_query_instance_t *));
  table->size = size; 
}

void reset_query_instance_table(query_instance_table_t *table) {
  int32_t i;
  for (i = 0; i < table->num_queries; i++) {
    free_samp_query_instance(table->query_inst[i]);
  }
  table->num_queries = 0;
}

void init_source_table(source_table_t *table) {
  table->size = INIT_SOURCE_TABLE_SIZE;
  table->num_entries = 0;
  if (table->size >= MAXSIZE(sizeof(source_entry_t *), 0)) {
    out_of_memory();
  }
  table->entry = (source_entry_t **)
    safe_malloc(table->size * sizeof(source_entry_t *));
}

void source_table_extend(source_table_t *table) {
  int32_t size = table->size;
  int32_t num_entries = table->num_entries;
  if (num_entries + 1 < size) return;
  if (MAXSIZE(sizeof(source_entry_t *), 0) - size <= (size/2)) {
    out_of_memory();
  }
  size += size/2;
  table->entry = (source_entry_t **)
    safe_realloc(table->entry, size * sizeof(source_entry_t *));
  table->size = size; 
}

void reset_source_table(source_table_t *table) {
  int32_t i;
  for (i = 0; i < table->num_entries; i++) {
    safe_free(table->entry[i]);
  }
  table->num_entries = 0;
}

void init_samp_table(samp_table_t *table) {
  init_sort_table(&table->sort_table);
  init_const_table(&table->const_table);
  init_var_table(&table->var_table);
  init_pred_table(&table->pred_table);
  init_atom_table(&table->atom_table);
  init_clause_table(&table->clause_table);
  init_rule_table(&table->rule_table);
  init_query_table(&table->query_table);
  init_query_instance_table(&table->query_instance_table);
  init_source_table(&table->source_table);
  init_integer_stack(&table->fixable_stack, 0);
}

void init_rule_table(rule_table_t *table){
  table->size = INIT_RULE_TABLE_SIZE;
  table->num_rules = 0;
  if (table->size >= MAXSIZE(sizeof(samp_rule_t *), 0)){
    out_of_memory();
  }
  table->samp_rules =
    (samp_rule_t **) safe_malloc(table->size * sizeof(samp_rule_t *));
}

void rule_table_resize(rule_table_t *rule_table){
  int32_t size = rule_table->size;
  int32_t num_rules = rule_table->num_rules;
  if (num_rules < size) return;
  if (MAXSIZE(sizeof(samp_rule_t *), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  rule_table->samp_rules = (samp_rule_t **)
    safe_realloc(rule_table->samp_rules,
		 size * sizeof(samp_rule_t *));
  rule_table->size = size; 
}


/* Checks the validity of the table for assertions within other functions.
   valid_sort_table checks that the sort size is nonnegative, num_sorts is
   at most size, and each sort name is hashed to the right index.
 */
bool valid_sort_table(sort_table_t *sort_table){
  if (sort_table->size < 0 || sort_table->num_sorts > sort_table->size)
    return false;
  uint32_t i = 0;
  while (i < sort_table->num_sorts &&
	 i == stbl_find(&(sort_table->sort_name_index),
			sort_table->entries[i].name))
    i++;
  if (i < sort_table->num_sorts) return false;
  return true;
}

/* Checks that the const names are hashed to the right index.
 */
bool valid_const_table(const_table_t *const_table, sort_table_t *sort_table){
  if (const_table->size < 0 || const_table->num_consts > const_table->size)
    return false;
  uint32_t i = 0;
  while (i < const_table->num_consts &&
	 i == stbl_find(&(const_table->const_name_index),
			const_table->entries[i].name) &&
	 const_table->entries[i].sort_index < sort_table->num_sorts)
    i++;
  if (i < const_table->num_consts) return false;
  return true;
}

/* Checks that evpred and pred names are hashed to the right index, and
   have a valid signature. 
 */
  
bool valid_pred_table(pred_table_t *pred_table,
		      sort_table_t *sort_table,
		      atom_table_t *atom_table){
  pred_tbl_t *evpred_tbl = &(pred_table->evpred_tbl);
  pred_entry_t *entry;
  uint32_t i, j;
  
  if (evpred_tbl->size < 0 || evpred_tbl->num_preds > evpred_tbl->size) {
    printf("Invalid pred size for evpred_tbl\n");
    return false;
  }
  i = 0;
  while (i < evpred_tbl->num_preds){
    entry = &(evpred_tbl->entries[i]);
    if (-i != pred_val_to_index(pred_index(entry->name, pred_table))) {
      printf("Invalid pred_val_to_index for evpred_tbl\n");
      return false;
    }
    if (entry->arity < 0) {
      printf("Invalid arity for evpred_tbl\n");
      return false;
    }
    for (j = 0; j < entry->arity; j ++){
      if (entry->signature[j] < 0 ||
	  entry->signature[j] >= sort_table->num_sorts) {
	printf("Invalid signature sort for evpred_tbl\n");
	return false;
      }
    }
    if (entry->size_atoms < 0 || entry->num_atoms > entry-> size_atoms) {
      printf("Invalid atoms size for evpred_tbl\n");
      return false;
    }
    for (j = 0; j < entry->num_atoms; j++){
      if (entry->atoms[j] < 0 ||
	  entry->atoms[j] >= atom_table->num_vars ||
	  atom_table->atom[entry->atoms[j]]->pred != -i) {
	printf("Invalid atom pred for evpred_tbl\n");
	return false;
      }
    }
    i++;
  }
  //check pred_tbl
  pred_tbl_t *pred_tbl = &(pred_table->pred_tbl);
  if (pred_tbl->size < 0 || pred_tbl->num_preds > pred_tbl->size) {
    printf("Invalid pred_tbl size\n");
    return false;
  }
  i = 0;
  while (i < pred_tbl->num_preds){
    entry = &(pred_tbl->entries[i]);
    if (i != pred_val_to_index(pred_index(entry->name, pred_table))) {
      printf("Invalid pred_val_to_index for pred_tbl\n");
      return false;
    }
    if (entry->arity < 0) return false;
    for (j = 0; j < entry->arity; j ++){
      if (entry->signature[j] < 0 ||
	  entry->signature[j] >= sort_table->num_sorts) {
	printf("Invalid signature for pred_tbl\n");
	return false;
      }
    }
    if (entry->size_atoms < 0 || entry->num_atoms > entry-> size_atoms) {
      printf("Invalid atom for pred_tbl\n");
      return false;
    }
    for (j = 0; j < entry->num_atoms; j++){
      if (entry->atoms[j] < 0 ||
	  entry->atoms[j] >= atom_table->num_vars ||
	  atom_table->atom[entry->atoms[j]]->pred != i) {
	printf("Invalid atom for pred_tbl\n");
	return false;
      }
    }
    i++;
  }
    return true;
}

/* Checks that each atom is well-formed. 
 */
bool valid_atom_table(atom_table_t *atom_table,
		      pred_table_t *pred_table,
		      const_table_t *const_table,
		      sort_table_t *sort_table){
  if (atom_table->size < 0 ||
      atom_table->num_vars > atom_table->size) {
    printf("Invalid atom table size\n");
    return false;
  }
  uint32_t i = 0;
  uint32_t j;
  int32_t pred, arity;
  int32_t num_unfixed = 0;
  int32_t *sig;
  while (i < atom_table->num_vars){
    pred = atom_table->atom[i]->pred;
    arity  = pred_arity(pred, pred_table);
    sig = pred_signature(pred, pred_table);
    if (!fixed_tval(atom_table->assignment[atom_table->current_assignment][i])){
      num_unfixed++;
    }
    for (j = 0; j < arity; j++){
      if (const_table->entries[atom_table->atom[i]->args[j]].sort_index !=
	  sig[j]) {
	printf("Invalid atom sort\n");
	return false;
      }
    }
    array_hmap_pair_t *hmap_pair;
    hmap_pair = array_size_hmap_find(&(atom_table->atom_var_hash),
				     arity+1,
				     (int32_t *) atom_table->atom[i]);
    if (hmap_pair == NULL ||
	hmap_pair->val != i) {
      printf("Invalid atom hmap_pair\n");
      return false;
    }
    i++;
  }
  if (num_unfixed != atom_table->num_unfixed_vars) {
    printf("Invalid atom num unfixed vars\n");
    return false;
  }
  return true;
}

bool valid_clause_table(clause_table_t *clause_table,
			atom_table_t *atom_table){
  samp_truth_value_t *assignment = atom_table->assignment[atom_table->current_assignment];
  //check that every clause in the unsat list is unsat
  samp_clause_t *link;
  int32_t lit;
  uint32_t i, j;
  if (clause_table->size < 0) return false;
  if (clause_table->num_clauses < 0 || clause_table->num_clauses > clause_table->size)
    return false;
  link = clause_table->unsat_clauses;
  i = 0;
  while (link !=NULL){
    if (eval_clause(assignment, link) != -1)
      return false;
    i++;
    link = link->link;
  }
  if (i != clause_table->num_unsat_clauses) return false;

  //check negative_or_unit_clauses
  link = clause_table->negative_or_unit_clauses;
  while (link != NULL){
    if (link->weight >= 0 &&
	link->numlits != 1) return false;
    link = link->link;
  }

  //check dead_negative_or_unit_clauses
  link = clause_table->dead_negative_or_unit_clauses;
  while (link != NULL){
    if (link->weight >= 0 &&
	link->numlits != 1) return false;
    link = link->link;
  }

  
  //check that every watched clause is satisfied and has the watched literal
  //as its first disjunct.
  for (i = 0; i < atom_table->num_vars; i++){
    lit = pos_lit(i);
    link = clause_table->watched[lit];
    if (link != NULL && !assigned_true(assignment[i]))
      return false;
    while (link != NULL){
      if (link->disjunct[0] != lit) return false;
      link = link->link;
    }
    lit = neg_lit(i);
    link = clause_table->watched[lit];
    if (link != NULL && !assigned_false(assignment[i]))
      return false;
    while (link != NULL){
      if (link->disjunct[0] != lit) return false;
      link = link->link;
    }
  }

  //check the sat_clauses to see if the first disjunct is fixed true
  link = clause_table->sat_clauses;
  while (link != NULL){
    if (!assigned_fixed_true_lit(assignment, link->disjunct[0]))
      return false;
    link = link->link;
  }
  samp_clause_t *clause;
  //check all the clauses to see if they are properly indexed
  for (i = 0; i < clause_table->num_clauses; i++){
    clause = clause_table->samp_clauses[i];
    if (clause->numlits < 0) return false;
    for (j = 0; j < clause->numlits; j++){
      if (var_of(clause->disjunct[j]) < 0 ||
	  var_of(clause->disjunct[j]) >= atom_table->num_vars)
	return false;
    }
  }
  return true;
}

bool valid_table(samp_table_t *table){
  sort_table_t *sort_table = &(table->sort_table); 
  const_table_t *const_table = &(table->const_table);
  atom_table_t *atom_table = &(table->atom_table);
  pred_table_t *pred_table = &(table->pred_table);
  clause_table_t *clause_table = &(table->clause_table);

  if (!valid_sort_table(sort_table)) {
    printf("Invalid sort_table\n");
    return false;
  }
  if (!valid_const_table(const_table, sort_table)) {
    printf("Invalid const_table\n");
    return false;
  }
  if (!valid_pred_table(pred_table, sort_table, atom_table)) {
    printf("Invalid pred_table\n");
    return false;
  }
  if (!valid_atom_table(atom_table, pred_table, const_table, sort_table)) {
    printf("Invalid atom table\n");
    return false;
  }
  if (!valid_clause_table(clause_table, atom_table)) {
    printf("Invalid clause_table\n");
    return false;
  }
  return true;
}
