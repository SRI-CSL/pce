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
  char * new_name = safe_malloc(++len * sizeof(char));
  memcpy(new_name, name, len);
  return new_name;
}

// Conditional print - only prints if level <= verbosity_level
void cprintf(int32_t level, const char *fmt, ...){
  if(level <= verbosity_level){
    va_list argp;
    va_start(argp, fmt);
    vprintf(fmt, argp);
    va_end(argp);
  }
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
  int32_t *new_signature = safe_malloc(size);
  memcpy(new_signature, signature, size);
  return new_signature;
}

void init_sort_table(sort_table_t *sort_table){
  int32_t size = INIT_SORT_TABLE_SIZE;
  if (size >= MAX_SIZE(sizeof(sort_entry_t), 0)){
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
  if (n >= MAX_SIZE(sizeof(sort_entry_t), 0)){
    out_of_memory();
  }
  sort_table->entries = (sort_entry_t *) safe_realloc(sort_table->entries,
					     n * sizeof(sort_entry_t));
  sort_table->size = n; 
}

void add_sort(sort_table_t *sort_table, char *name){
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

/* The same functionality is repeated for constants
 */

void init_const_table(const_table_t *const_table){
  int32_t size = INIT_CONST_TABLE_SIZE;
  if (size >= MAX_SIZE(sizeof(const_entry_t), 0)){
    out_of_memory();
  }
  const_table->size = size;
  const_table->num_consts = 0;
  const_table->entries = (const_entry_t*)
    safe_malloc(size*sizeof(const_entry_t));
  init_stbl(&(const_table->const_name_index), 0);
}

static void const_table_resize(const_table_t *const_table, uint32_t n){
  if (n >= MAX_SIZE(sizeof(const_entry_t), 0)){
    out_of_memory();
  }
  const_table->entries = (const_entry_t *) safe_realloc(const_table->entries,
					     n * sizeof(const_entry_t));
  const_table->size = n; 
}

static void add_const_to_sort(int32_t const_index,
			      int32_t sort_index,
			      sort_table_t *sort_table){
  sort_entry_t *entry = &(sort_table->entries[sort_index]);
  if (entry->size == entry->cardinality){
    if (MAX_SIZE(sizeof(int32_t), 0) - entry->size < entry->size/2){
      out_of_memory();
    }
    entry->size += entry->size/2;
    entry->constants = (int32_t *) safe_realloc(entry->constants,
						entry->size * sizeof(int32_t));
    }
  entry->constants[entry->cardinality++] = const_index;
}

int32_t add_const(const_table_t *const_table,
		  char *name,
		  sort_table_t *sort_table,
		  char *sort_name){
  int32_t sort_index = sort_name_index(sort_name, sort_table);
  if (sort_index == -1){
    printf("Sort name %s has not been declared.\n", sort_name);
    return -1;
  }
  int32_t index = stbl_find(&(const_table->const_name_index), name);
  if (index == -1){//const is not in the symbol table
    int32_t n = const_table->num_consts;
    if (n >= const_table->size){
      const_table_resize(const_table, n + (n/2));
    }
    name = str_copy(name);
    const_table->entries[n].name = name;
    const_table->entries[n].sort_index = sort_index;
    add_const_to_sort(n, sort_index, sort_table);
    const_table->num_consts++;
    stbl_add(&(const_table->const_name_index), name, n);
    return 0;
  } else {
    printf("Constant %s already exists\n", name);
    return -1;
  }
}

int32_t const_index(char *name,
		    const_table_t *const_table){
  return stbl_find(&(const_table->const_name_index), name);
}

char *const_name(int32_t const_index,
		  const_table_t *const_table){
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
  if (size >= MAX_SIZE(sizeof(var_entry_t), 0)){
    out_of_memory();
  }
  var_table->size = size;
  var_table->num_vars = 0;
  var_table->entries = (var_entry_t*)
    safe_malloc(size*sizeof(var_entry_t));
  init_stbl(&(var_table->var_name_index), 0);
}

static void var_table_resize(var_table_t *var_table, uint32_t n){
  if (n >= MAX_SIZE(sizeof(var_entry_t), 0)){
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
  if (size >= MAX_SIZE(sizeof(pred_entry_t), 0)){
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
  if (MAX_SIZE(sizeof(pred_entry_t), 0) - size <= (size/2)){
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
  if (index == -1){//pred is not in the symbol table  (Change to stbl_rec_t *)
    if (evidence){
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
    pred_tbl->num_preds++;
    stbl_add(&(pred_table->pred_name_index), name, index);//note index is never -1
    //printf("\nAdded predicate index %d with name %s, hashindex %d, arity %d, and signature:",
    //   n, name, index, arity);
    //printf("\nhashindex[%s] = %d",
    //   pred_tbl->entries[n].name,
    //   pred_index(pred_tbl->entries[n].name, pred_table));
    //printf("\n(");
    //int32_t i;
    //for (i=0; i < arity; i++){
    //printf("%d", signature[i]);
    //}
    //printf(")");
    return 0;
  } else {
    return -1;
  }
}

void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table){
  char *buffer;
  int32_t i, j; 
   printf("num of evpreds: %d\n", pred_table->evpred_tbl.num_preds);
   for (i = 0; i < pred_table->evpred_tbl.num_preds; i++){
     printf("evpred[%d] = ", i);
      buffer = pred_table->evpred_tbl.entries[i].name;
      printf("index(%d); ", pred_index(buffer, pred_table));
      printf( "%s(", buffer);
      for (j = 0; j < pred_table->evpred_tbl.entries[i].arity; j++){
	if (j != 0) printf(", ");
	printf("%s", sort_table->entries[pred_table->evpred_tbl.entries[i].signature[j]].name);
      }
      printf(")\n");
  }
   printf("num of preds: %d\n", pred_table->pred_tbl.num_preds);
   for (i = 0; i < pred_table->pred_tbl.num_preds; i++){
     printf("\n pred[%d] = ", i);
     buffer = pred_table->pred_tbl.entries[i].name;
     printf("index(%d); ", pred_index(buffer, pred_table));
     printf("%s(", buffer);
    for (j = 0; j < pred_table->pred_tbl.entries[i].arity; j++){
      if (j != 0) printf(", ");
      printf("%s", sort_table->entries[pred_table->pred_tbl.entries[i].signature[j]].name);
    }
    printf(")\n");
  }
}

void init_atom_table(atom_table_t *table){
  table->size = INIT_ATOM_TABLE_SIZE;
  table->num_vars = 0; //atoms are positive
  table->num_unfixed_vars = 0;
  if (table->size >= MAX_SIZE(sizeof(atom_entry_t), 0)){
    out_of_memory();
  }
  table->atom = (samp_atom_t **) safe_malloc(table->size * sizeof(samp_atom_t *));
  table->assignment = (samp_truth_value_t *)
    safe_malloc(table->size * sizeof(samp_truth_value_t));
  table->pmodel = (double *) safe_malloc(table->size * sizeof(int32_t));
  uint32_t i;
  for (i = 0; i < table->size; i++)
    table->pmodel[i] = 0;//was -1
  table->num_samples = 0;
  init_array_hmap(&(table->atom_var_hash), ARRAY_HMAP_DEFAULT_SIZE);
  //  table->entries[0].atom = (samp_atom_t *) safe_malloc(sizeof(samp_atom_t));
  //  table->entries[0].atom->pred = 0;
}

/*
 * When atom_table is resized, the watched literals must also be resized. 
 */
void atom_table_resize(atom_table_t *atom_table, clause_table_t *clause_table){
  int32_t size = atom_table->size;
  int32_t num_vars = atom_table->num_vars;
  if (num_vars < size) return;
  if (MAX_SIZE(sizeof(atom_entry_t), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  atom_table->atom = (samp_atom_t **) safe_realloc(atom_table->atom,
					     size * sizeof(samp_atom_t *));
  atom_table->assignment = (samp_truth_value_t *) safe_realloc(atom_table->assignment,
					     size * sizeof(samp_truth_value_t));
  atom_table->pmodel = (int32_t *) safe_realloc(atom_table->pmodel,
					       size * sizeof(int32_t));
  atom_table->size = size;

  if (MAX_SIZE(sizeof(samp_clause_t *), 0) - size <= size){
    out_of_memory();
  }
  clause_table->watched = (samp_clause_t **) safe_realloc(clause_table->watched,
							  2*size*sizeof(samp_clause_t *));
}


void init_clause_table(clause_table_t *table){
  table->size = INIT_CLAUSE_TABLE_SIZE;
  table->num_clauses = 0;
    if (table->size >= MAX_SIZE(sizeof(samp_clause_t *), 0)){
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

void clause_table_resize(clause_table_t *clause_table){
  int32_t size = clause_table->size;
  int32_t num_clauses = clause_table->num_clauses;
  if (num_clauses < size) return;
  if (MAX_SIZE(sizeof(samp_clause_t *), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  clause_table->samp_clauses = (samp_clause_t **)
    safe_realloc(clause_table->samp_clauses,
		 size * sizeof(samp_clause_t *));
  clause_table->size = size; 
}


void init_samp_table(samp_table_t *table){
  init_sort_table(&(table->sort_table));
  init_const_table(&(table->const_table));
  init_var_table(&(table->var_table));
  init_pred_table(&(table->pred_table));
  init_atom_table(&(table->atom_table));
  init_clause_table(&(table->clause_table));
  init_rule_table(&(table->rule_table));
  init_integer_stack(&(table->fixable_stack), 0);
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

void print_atom(samp_atom_t *atom,
		samp_table_t *table) {
  pred_table_t *pred_table = &(table->pred_table);
  const_table_t *const_table = &(table->const_table);
  uint32_t i;
  printf("%s", pred_name(atom->pred, pred_table));
  for (i = 0; i < pred_arity(atom->pred, pred_table); i++){
    i == 0 ? printf("(") : printf(", ");
    printf("%s", const_name(atom->args[i], const_table));
  }
  printf(")");
}

char *samp_truth_value_string(samp_truth_value_t val){
  return
    val == v_undef ? "U" :
    val == v_false ? "F" :
    val == v_true ?  "T" :
    val == v_fixed_false ? "fx F" :
    val == v_fixed_true ? "fx T" :
    val == v_db_false ? "db F" :
    val == v_db_true ? "db T" : "ERROR";
}

void print_atoms(samp_table_t *table){
  atom_table_t *atom_table = &(table->atom_table);
  uint32_t nvars = atom_table->num_vars;
  char d[10];
  uint32_t nwdth = sprintf(d, "%d", nvars);
  uint32_t i;
  int32_t num_samples = atom_table->num_samples;
  printf("--------------------------------------------------------------------------------\n");
  printf("| %*s | tval | prob   | %-*s |\n", nwdth, "i", 57-nwdth, "atom");
  printf("--------------------------------------------------------------------------------\n");
  for (i = 0; i < nvars; i++){
    samp_truth_value_t tv = atom_table->assignment[i];
    printf("| %-*u | %-4s | % 5.3f | ", nwdth, i, samp_truth_value_string(tv),
	   (double) (atom_table->pmodel[i]/(double) num_samples));
    print_atom(atom_table->atom[i], table);
    printf("\n");
  }
  printf("--------------------------------------------------------------------------------\n");
}

clause_buffer_t clause_buffer = {0, NULL};

void clause_buffer_resize (int32_t length){
  if (clause_buffer.data == NULL){
    clause_buffer.data = (int32_t *) safe_malloc(INIT_CLAUSE_SIZE * sizeof(int32_t));
    clause_buffer.size = INIT_CLAUSE_SIZE;
  }
  int32_t size = clause_buffer.size;
  if (size < length){
    if (MAX_SIZE(sizeof(int32_t), 0) - size <= size/2){
      out_of_memory();
    }
    size += size/2;
    clause_buffer.data =
      (int32_t  *) safe_realloc(clause_buffer.data, size * sizeof(int32_t));
    clause_buffer.size = size;
  }
}

void print_clause(samp_clause_t *clause, samp_table_t *table){
  atom_table_t *atom_table = &(table->atom_table);
  int32_t i;
  for (i = 0; i<clause->numlits; i++){
    if (i != 0) printf(" | ");
    int32_t samp_atom = var_of(clause->disjunct[i]);
    if (is_neg(clause->disjunct[i])) printf("~");
    print_atom(atom_table->atom[samp_atom], table);
  }
}

void print_clauses(samp_table_t *table){
  clause_table_t *clause_table = &(table->clause_table);
  int32_t nclauses = clause_table->num_clauses;
  char d[20];
  uint32_t nwdth = sprintf(d, "%d", nclauses);
  uint32_t i;
  printf("\n-------------------------------------------------------------------------------\n");
  printf("| %-*s | weight    | %-*s|\n", nwdth, "i", 62-nwdth, "clause");
  printf("-------------------------------------------------------------------------------\n");
  for (i = 0; i < nclauses; i++){
    double wt = clause_table->samp_clauses[i]->weight;
    if (wt == DBL_MAX) {
      printf("| %*"PRIu32" |    MAX    | ", nwdth, i);
    } else {
      printf("| %*"PRIu32" | % 9.3f | ", nwdth, i, wt);
    }
    print_clause(clause_table->samp_clauses[i], table);
    printf("\n");
  }
  printf("-------------------------------------------------------------------------------\n");
}

void print_clause_list(samp_clause_t *link, samp_table_t *table){
  while (link != NULL){
    print_clause(link, table);
    link = link->link;
  }
  printf("\n");
 }

void print_clause_table(samp_table_t *table, int32_t num_vars){
  clause_table_t *clause_table = &(table->clause_table);
  print_clauses(table);
  printf("\n");
  printf("Sat clauses: \n");
  samp_clause_t *link = clause_table->sat_clauses;
  print_clause_list(link, table);
  printf("Unsat clauses: \n");
  link = clause_table->unsat_clauses;
  print_clause_list(link, table);  
  printf("Watched clauses: \n");
  int32_t i, lit;
  for (i = 0; i < num_vars; i++){
    lit = pos_lit(i);
    printf("lit[%d]: ", lit);
    link = clause_table->watched[lit];
    print_clause_list(link, table);
    lit = neg_lit(i);
    printf("lit[%d]: ", lit);
    link = clause_table->watched[lit];
    print_clause_list(link, table);
  }
  printf("Dead clauses:\n");
  link = clause_table->dead_clauses;
  print_clause_list(link, table);
  printf("Negative/Unit clauses:\n");
  link = clause_table->negative_or_unit_clauses;
  print_clause_list(link, table);
  printf("Dead negative/unit clauses:\n");
  link = clause_table->dead_negative_or_unit_clauses;
  print_clause_list(link, table);
}


void print_state(samp_table_t *table){
  atom_table_t *atom_table = &(table->atom_table);
  printf("==============================================================\n");
  print_atoms(table);
  printf("==============================================================\n");
  print_clause_table(table, atom_table->num_vars);
}

void init_rule_table(rule_table_t *table){
  table->size = INIT_RULE_TABLE_SIZE;
  table->num_rules = 0;
  if (table->size >= MAX_SIZE(sizeof(samp_rule_t *), 0)){
    out_of_memory();
  }
  table->samp_rules =
    (samp_rule_t **) safe_malloc(table->size * sizeof(samp_rule_t *));
}

void rule_table_resize(rule_table_t *rule_table){
  int32_t size = rule_table->size;
  int32_t num_rules = rule_table->num_rules;
  if (num_rules < size) return;
  if (MAX_SIZE(sizeof(samp_rule_t *), 0) - size <= (size/2)){
    out_of_memory();
  }
  size += size/2;
  rule_table->samp_rules = (samp_rule_t **)
    safe_realloc(rule_table->samp_rules,
		 size * sizeof(samp_rule_t *));
  rule_table->size = size; 
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

/* Checks the validity of the table for assertions within other functions.
   valid_sort_table checks that the sort size is nonnegative, num_sorts is
   at most size, and each sort name is hashed to the right index.
 */
bool valid_sort_table(sort_table_t *sort_table){
  if (sort_table->size < 0 || sort_table->num_sorts > sort_table->size)
    return 0;
  uint32_t i = 0;
  while (i < sort_table->num_sorts &&
	 i == stbl_find(&(sort_table->sort_name_index),
			sort_table->entries[i].name))
    i++;
  if (i < sort_table->num_sorts) return 0;
  return 1;
}

/* Checks that the const names are hashed to the right index.
 */
bool valid_const_table(const_table_t *const_table, sort_table_t *sort_table){
  if (const_table->size < 0 || const_table->num_consts > const_table->size)
    return 0;
  uint32_t i = 0;
  while (i < const_table->num_consts &&
	 i == stbl_find(&(const_table->const_name_index),
			const_table->entries[i].name) &&
	 const_table->entries[i].sort_index < sort_table->num_sorts)
    i++;
  if (i < const_table->num_consts) return 0;
  return 1;
}

/* Checks that evpred and pred names are hashed to the right index, and
   have a valid signature. 
 */
  
bool valid_pred_table(pred_table_t *pred_table,
		      sort_table_t *sort_table,
		      atom_table_t *atom_table){
  pred_tbl_t *evpred_tbl = &(pred_table->evpred_tbl);
  if (evpred_tbl->size < 0 ||
      evpred_tbl->num_preds > evpred_tbl->size)
    return 0;
  uint32_t i = 0;
  uint32_t j;
  pred_entry_t *entry;
  while (i < evpred_tbl->num_preds){
    entry = &(evpred_tbl->entries[i]);
    if (-i != pred_val_to_index(pred_index(entry->name, pred_table)))
      return 0;
    if (entry->arity < 0) return 0;
    for (j = 0; j < entry->arity; j ++){
      if (entry->signature[j] < 0 ||
	  entry->signature[j] >= sort_table->num_sorts)
	return 0;
    }
    if (entry->size_atoms < 0 || entry->num_atoms > entry-> size_atoms)
      return 0;
    for (j = 0; j < entry->num_atoms; j++){
      if (entry->atoms[j] < 0 ||
	  entry->atoms[j] >= atom_table->num_vars ||
	  atom_table->atom[entry->atoms[j]]->pred != -i)
	return 0;
    }
    i++;
  }
    //check pred_tbl
  pred_tbl_t *pred_tbl = &(pred_table->pred_tbl);
  if (pred_tbl->size < 0 ||
      pred_tbl->num_preds > pred_tbl->size)
    return 0;
  i = 0;
  while (i < pred_tbl->num_preds){
    entry = &(pred_tbl->entries[i]);
    if (i != pred_val_to_index(pred_index(entry->name, pred_table)))
      return 0;
    if (entry->arity < 0) return 0;
    for (j = 0; j < entry->arity; j ++){
      if (entry->signature[j] < 0 ||
	  entry->signature[j] >= sort_table->num_sorts)
	return 0;
    }
    if (entry->size_atoms < 0 || entry->num_atoms > entry-> size_atoms)
      return 0;
    for (j = 0; j < entry->num_atoms; j++){
      if (entry->atoms[j] < 0 ||
	  entry->atoms[j] >= atom_table->num_vars ||
	  atom_table->atom[entry->atoms[j]]->pred != i)
	return 0;
    }
    i++;
  }
    return 1;
}

/* Checks that each atom is well-formed. 
 */
bool valid_atom_table(atom_table_t *atom_table,
		      pred_table_t *pred_table,
		      const_table_t *const_table,
		      sort_table_t *sort_table){
  if (atom_table->size < 0 ||
      atom_table->num_vars > atom_table->size)
    return 0;
  uint32_t i = 0;
  uint32_t j;
  int32_t pred, arity;
  int32_t *sig;
  while (i < atom_table->num_vars){
    pred = atom_table->atom[i]->pred;
    arity  = pred_arity(pred, pred_table);
    sig = pred_signature(pred, pred_table);
    for (j = 0; j < arity; j++){
      if (const_table->entries[atom_table->atom[i]->args[j]].sort_index !=
	  sig[j])
	return 0;
    }
    array_hmap_pair_t *hmap_pair;
    hmap_pair = array_size_hmap_find(&(atom_table->atom_var_hash),
				     arity+1,
				     (int32_t *) atom_table->atom[i]);
    if (hmap_pair == NULL ||
	hmap_pair->val != i)
      return 0;
    i++;
  }
  return 1;
}


/* Evaluates a clause to false (-1) or to the literal index evaluating to true.
 */
int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause){
  int32_t i;
  for (i = 0; i < clause->numlits; i++){
    if (assigned_true_lit(assignment, clause->disjunct[i]))
      return i;
  }
  return -1;
}

/* Evaluates a clause to -1 if all literals are false, and i if the
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

bool valid_clause_table(clause_table_t *clause_table,
			atom_table_t *atom_table){
  samp_truth_value_t *assignment = atom_table->assignment;
  //check that every clause in the unsat list is unsat
  samp_clause_t *link;
  int32_t lit;
  uint32_t i, j;
  if (clause_table->size < 0) return 0;
  if (clause_table->num_clauses < 0 || clause_table->num_clauses > clause_table->size)
    return 0;
  link = clause_table->unsat_clauses;
  i = 0;
  while (link !=NULL){
    if (eval_clause(assignment, link) != -1)
      return 0;
    i++;
    link = link->link;
  }
  if (i != clause_table->num_unsat_clauses) return 0;

  //check negative_or_unit_clauses
  link = clause_table->negative_or_unit_clauses;
  while (link != NULL){
    if (link->weight >= 0 &&
	link->numlits != 1) return 0;
    link = link->link;
  }

  //check dead_negative_or_unit_clauses
  link = clause_table->dead_negative_or_unit_clauses;
  while (link != NULL){
    if (link->weight >= 0 &&
	link->numlits != 1) return 0;
    link = link->link;
  }

  
  //check that every watched clause is satisfied and has the watched literal
  //as its first disjunct.
  for (i = 0; i < atom_table->num_vars; i++){
    lit = pos_lit(i);
    link = clause_table->watched[lit];
    if (link != NULL && !assigned_true(assignment[i]))
      return 0;
    while (link != NULL){
      if (link->disjunct[0] != lit) return 0;
      link = link->link;
    }
    lit = neg_lit(i);
    link = clause_table->watched[lit];
    if (link != NULL && !assigned_false(assignment[i]))
      return 0;
    while (link != NULL){
      if (link->disjunct[0] != lit) return 0;
      link = link->link;
    }
  }

  //check the sat_clauses to see if the first disjunct is fixed true
  link = clause_table->sat_clauses;
  while (link != NULL){
    if (!assigned_fixed_true_lit(assignment, link->disjunct[0]))
      return 0;
    link = link->link;
  }
  samp_clause_t *clause;
  //check all the clauses to see if they are properly indexed
  for (i = 0; i < clause_table->num_clauses; i++){
    clause = clause_table->samp_clauses[i];
    if (clause->numlits < 0) return 0;
    for (j = 0; j < clause->numlits; j++){
      if (var_of(clause->disjunct[j]) < 0 ||
	  var_of(clause->disjunct[j]) >= atom_table->num_vars)
	return 0;
    }
  }
  return 1;
}

bool valid_table(samp_table_t *table){
  sort_table_t *sort_table = &(table->sort_table); 
  const_table_t *const_table = &(table->const_table);
  atom_table_t *atom_table = &(table->atom_table);
  pred_table_t *pred_table = &(table->pred_table);
  clause_table_t *clause_table = &(table->clause_table);

  if (!valid_sort_table(sort_table)) return 0;
  if (!valid_const_table(const_table, sort_table)) return 0;
  if (!valid_pred_table(pred_table, sort_table, atom_table)) return 0;
  if (!valid_atom_table(atom_table, pred_table, const_table, sort_table)) return 0;
  if (!valid_clause_table(clause_table, atom_table)) return 0;
  return 1;
}

substit_buffer_t substit_buffer = {0, NULL};

// Fill this in later
void substit_buffer_resize(int32_t length) {
  if (substit_buffer.entries == NULL){
    substit_buffer.entries = (substit_entry_t *)
      safe_malloc(INIT_SUBSTIT_TABLE_SIZE * sizeof(substit_entry_t));
    substit_buffer.size = INIT_SUBSTIT_TABLE_SIZE;
  }
  int32_t size = substit_buffer.size;
  if (size < length){
    if (MAX_SIZE(sizeof(substit_entry_t), 0) - size <= size/2){
      out_of_memory();
    }
    size += size/2;
    substit_buffer.entries = (substit_entry_t *)
      safe_realloc(substit_buffer.entries, size * sizeof(substit_entry_t));
    substit_buffer.size = size;
  }
}
