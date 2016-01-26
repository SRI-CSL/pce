#include <float.h>
#include <string.h>

#include "memalloc.h"
#include "utils.h"
#include "parser.h"
#include "print.h"
#include "tables.h"
#include "input.h"
#include "clause_list.h"

void init_sort_table(sort_table_t *sort_table){
	int32_t size = INIT_SORT_TABLE_SIZE;
	if (size >= MAXSIZE(sizeof(sort_entry_t), 0)){
		out_of_memory();
	}
	sort_table->size = size;
	sort_table->num_sorts = 0;
	sort_table->entries = (sort_entry_t*)safe_malloc(
			size*sizeof(sort_entry_t));
	/* this is not really needed */
	//for (i=0; i<size; i++){
	//	sort_table->entries[i].cardinality = 0;
	//	sort_table->entries[i].name = NULL;
	//}
	init_stbl(&(sort_table->sort_name_index), 0);
}

/*
 * Deep copy: Those functions that begin with 'copy_' will generally
 * take a 'to' and a 'from' argument, and assume that 'to' has been
 * allocated by the caller.  Those functions that begin with 'clone_'
 * will only take a 'from' argument, and will allocate enough space
 * for a new struct, fill it, and return that.  In the latter case,
 * the struct is of variable length, so that the last slot is an array
 * of length dictated by an earlier slot in the struct, so it's a bit
 * more convenient to allocate on the fly.
 *
 * So far, none of these functions has a true, reliable free
 * mechanism, so until we fix that, we will have to accept the memory
 * leak.
 */


void copy_sort_entry(sort_entry_t *to, sort_entry_t *from) {

  int i;

  to->size = from->size;
  to->cardinality = from->cardinality;
  to->name = str_copy(from->name);

  to->constants = (int32_t *) safe_malloc(from->cardinality * sizeof(int32_t));
  memcpy( to->constants, from->constants, from->cardinality * sizeof(int32_t) );

  if (from->ints == NULL) to->ints = NULL;
  else {
    to->ints = (int32_t *) safe_malloc(from->cardinality * sizeof(int32_t));
    memcpy( to->ints, from->ints, from->cardinality * sizeof(int32_t) );
  }

  to->lower_bound = from->lower_bound;
  to->upper_bound = from->upper_bound;

  /* As with ints above, am guessing here and we will need to more
   * thoroughly test when these things are non-NULL...
   */
  if (from->subsorts == NULL) to->subsorts = NULL;
  else {
    /* Not terribly safe: */
    i = 0;
    while (from->subsorts[i] != -1) i++;
    i++;

    to->subsorts = (int32_t *) safe_malloc(i * sizeof(int32_t));
    memcpy(to->subsorts, from->subsorts, i*sizeof(int32_t));
  }

  if (from->supersorts == NULL) to->supersorts = NULL;
  else {
    /* Not terribly safe: */
    i = 0;
    while (from->supersorts[i] != -1) i++;
    i++;

    to->supersorts = (int32_t *) safe_malloc(i * sizeof(int32_t));
    memcpy(to->supersorts, from->supersorts, i*sizeof(int32_t));
  }
}


void copy_sort_table(sort_table_t *to, sort_table_t *from) {
  int32_t i, size;
  size = to->size = from->size;
  to->num_sorts = from->num_sorts;
  to->entries = (sort_entry_t*) safe_malloc(size * sizeof(sort_entry_t));

  /* Deep-copy the sort entries: */
  for (i = 0; i < from->num_sorts; i++)
    copy_sort_entry( &(to->entries[i]), &(from->entries[i]) );

  /* 
   * If memory is being shared (e.g. among threads), it's probably
   * safe to point to the same name_index, but this will not work if
   * we ever want to pickle the top-level samp_table_t:
   */
  copy_stbl( &to->sort_name_index, &from->sort_name_index );
}


/* How are sort_table_t objects destroyed / freed? */

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
	if (index == -1){ /* sort is not in the symbol table */
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
	sort_entry_t *subentry, *supentry;
	int32_t i, j;
	bool foundit;

	subentry = &sort_table->entries[subidx];
	supentry = &sort_table->entries[supidx];
	for (i = 0; i < subentry->cardinality; i++) {
		foundit = false;
		for (j = 0; j < supentry->cardinality; j++) {
			if (subentry->constants == NULL) {
				if (subentry->ints[i] == supentry->ints[j]) {
					foundit = true;
					break;
				}
			} else {
				if (subentry->constants[i] == supentry->constants[j]) {
					foundit = true;
					break;
				}
			}
		}
		if (! foundit) {
			// Note that this also adds to the supersorts of supentry
			if (subentry->constants == NULL) {
				add_int_const(subentry->ints[i], supentry, sort_table);
			} else {
				add_const_to_sort(subentry->constants[i], supidx, sort_table);
			}
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

int32_t get_sort_const(sort_entry_t *sort_entry, int32_t index) {
	if (sort_entry->constants == NULL) {
		if (sort_entry->ints == NULL) {
			return sort_entry->lower_bound + index;
		}
		else {
			return sort_entry->ints[index];
		}
	}
	else {
		return sort_entry->constants[index];
	}
}

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


/*
 * const table copier:
 */

void copy_const_entry(const_entry_t *to, const_entry_t *from) {
  if (from->name == NULL) to->name = NULL;
  else to->name = str_copy(from->name);
  to->sort_index = from->sort_index;
}

void copy_const_table(const_table_t *to, const_table_t *from) {
  int32_t i, size;
  size = to->size = from->size;
  to->num_consts = from->num_consts;
  to->entries = (const_entry_t*) safe_malloc(size*sizeof(const_entry_t));

  for (i = 0; i < size; i++)
    copy_const_entry( &(to->entries[i]), &(from->entries[i]) );

  /* If we are to pickle the tables, then this will need to be copied.
   * Just copy the pointer for now:   */
  copy_stbl( &to->const_name_index, &from->const_name_index );
}

static void const_table_resize(const_table_t *const_table, uint32_t n){
	if (n >= MAXSIZE(sizeof(const_entry_t), 0)){
		out_of_memory();
	}
	const_table->entries = (const_entry_t *) safe_realloc(const_table->entries,
			n * sizeof(const_entry_t));
	const_table->size = n; 
}

/*
 * Resize the constant array or the int array of a sort to
 * accommodate a new constant or int.
 */
void sort_entry_resize(sort_entry_t *sort_entry) {
	if (sort_entry->size == sort_entry->cardinality) {
		if (MAXSIZE(sizeof(int32_t), 0) - sort_entry->size < sort_entry->size/2){
			out_of_memory();
		}
		sort_entry->size += sort_entry->size/2;
		if (sort_entry->constants != NULL) {
			sort_entry->constants = (int32_t *) safe_realloc(sort_entry->constants,
					sort_entry->size * sizeof(int32_t));
		}
		if (sort_entry->ints != NULL) {
			sort_entry->ints = (int32_t *) safe_realloc(sort_entry->ints,
					sort_entry->size * sizeof(int32_t));
		}
	}
}

/* 
 * Adds a constant to the corresponding entry in sort_table
 */
void add_const_to_sort(int32_t const_index,
		int32_t sort_index,
		sort_table_t *sort_table) {
	int32_t i, j;
	bool foundit;
	sort_entry_t *entry, *supentry;

	entry = &sort_table->entries[sort_index];
	assert(entry->constants != NULL);
	sort_entry_resize(entry);
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

/*
 * Internal form of add_const, checks if const already exists, and verifies
 * its sort if it does.  Returns 0 if it is newly added, 1 if it is already
 * there, and -1 if there is an error (e.g., sort mismatch).
 */
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

/*
 * Adds a constant to the const_table.  If the constant already exists,
 * simply prints a message, but does not check that the signature is correct.
 */
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

/*
 * var table copier:
 */
void copy_var_entry(var_entry_t *to, var_entry_t *from) {
  if (from->name == NULL) to->name = NULL;
  else to->name = str_copy(from->name);
  to->sort_index = from->sort_index;
}

void copy_var_table(var_table_t *to, var_table_t *from) {
  int32_t i, size;
  size = to->size = from->size;
  to->num_vars = from->num_vars;
  to->entries = (var_entry_t*) safe_malloc(size*sizeof(var_entry_t));
  for (i = 0; i < size; i++)
    copy_var_entry( &(to->entries[i]), &(from->entries[i]) );

  /* If we are to pickle the tables, then this will need to be copied.
   * Just copy the pointer for now:   */
  copy_stbl( &to->var_name_index, &from->var_name_index );
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

// FIXME why? the true evidence predicate occupies the 0 slot
static char *truepred = "true"; 

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



/*
 * Copy the predicate entry:
 */

void copy_pred_entry(pred_entry_t *to, pred_entry_t *from) {
  /* Copy the signature array: */
  to->arity = from->arity;
  to->signature = (int32_t *) safe_malloc( to->arity * sizeof(int32_t) );
  memcpy(to->signature, from->signature,  to->arity * sizeof(int32_t) );

  to->name = str_copy(from->name);

  /* Copying the atom index array: */
  to->size_atoms = from->size_atoms;
  to->num_atoms = from->num_atoms;
  to->atoms = (int32_t *) safe_malloc(to->size_atoms * sizeof(int32_t));
  memcpy(to->atoms, from->atoms, to->size_atoms * sizeof(int32_t));

  /* Copying the rule index array: */
  to->size_rules = from->size_rules;
  to->num_rules = from->num_rules;
  to->rules = (int32_t *) safe_malloc(to->size_rules * sizeof(int32_t));
  memcpy(to->rules, from->rules, to->size_rules * sizeof(int32_t));
}

/*
 * Predicates are split into direct (ev) and indirect (non-evidence),
 * but their structure is the same, so copy here:
 */

void copy_pred_tbl(pred_tbl_t *to, pred_tbl_t *from) {
  int i, size;
  size = to->size = from->size;
  to->num_preds = from->num_preds;
  to->entries = (pred_entry_t *) safe_malloc(size * sizeof(pred_entry_t));
  for (i = 0; i < to->num_preds; i++)
    copy_pred_entry( &(to->entries[i]), &(from->entries[i]) );
}


/*
 * Deep copy the predicate table:
 */
void copy_pred_table(pred_table_t *to, pred_table_t *from) {
  copy_pred_tbl( &(to->evpred_tbl), &(from->evpred_tbl) );
  copy_pred_tbl( &(to->pred_tbl), &(from->pred_tbl) );

  /* Just copy the pointer for the symbol table: */
  copy_stbl( &to->pred_name_index, &from->pred_name_index );
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

/* Prepares for adding a new atom for a pred entry*/
void pred_atom_table_resize(pred_entry_t *pred_entry) {
	int32_t size;
	if (pred_entry->num_atoms >= pred_entry->size_atoms) {
		if (pred_entry->size_atoms == 0) {
			pred_entry->atoms = (int32_t *) safe_malloc(
					INIT_ATOM_PRED_SIZE * sizeof(int32_t));
			pred_entry->size_atoms = INIT_ATOM_PRED_SIZE;
		} else {
			size = pred_entry->size_atoms;
			if (MAXSIZE(sizeof(int32_t), 0) - size <= size / 2) {
				out_of_memory();
			}
			size += size / 2;
			pred_entry->atoms = (int32_t *) safe_realloc(pred_entry->atoms,
					size * sizeof(int32_t));
			pred_entry->size_atoms = size;
		}
	}
}

/* Adds a new atom for a pred entry */
void add_atom_to_pred(pred_table_t *pred_table, int32_t predicate,
		int32_t current_atom_index) {
	pred_entry_t *entry = get_pred_entry(pred_table, predicate);
	pred_atom_table_resize(entry);
	entry->atoms[entry->num_atoms++] = current_atom_index;
}

/* Prepares for adding a rule for a pred entry */
void pred_rule_table_resize(pred_entry_t *pred_entry) {
	int32_t size;
	if (pred_entry->num_rules >= pred_entry->size_rules) {
		if (pred_entry->size_rules == 0) {
			pred_entry->rules = (int32_t *) safe_malloc(
					INIT_RULE_PRED_SIZE * sizeof(int32_t));
			pred_entry->size_rules = INIT_RULE_PRED_SIZE;
		} else {
			size = pred_entry->size_rules;
			if (MAXSIZE(sizeof(int32_t), 0) - size <= size / 2) {
				out_of_memory();
			}
			size += size / 2;
			pred_entry->rules = (int32_t *) safe_realloc(pred_entry->rules,
					size * sizeof(int32_t));
			pred_entry->size_rules = size;
		}
	}
}

/* Adds a rule for a pred entry */
void add_rule_to_pred(pred_table_t *pred_table, int32_t predicate,
		int32_t current_rule_index) {
	pred_entry_t *entry = get_pred_entry(pred_table, predicate);
	int32_t i;

	/* if alreay exists, return */
	for (i = 0; i < entry->num_rules; i++) {
		if (entry->rules[i] == current_rule_index) {
			return;
		}
	}

	pred_rule_table_resize(entry);
	entry->rules[entry->num_rules++] = current_rule_index;
}

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

/* Whether atom_id is an evidence atom */
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
	} else {
		return pred_table->pred_tbl.entries[pred].name;
	}
}

/* FIXME 
 * 0: false; 1: true; */
inline int32_t pred_default_value(pred_entry_t *pred) {
	return 0;
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
	table->active = (bool *) safe_malloc(table->size * sizeof(bool));
	table->assignments[0] = (samp_truth_value_t *)
		safe_malloc(table->size * sizeof(samp_truth_value_t));
	table->assignments[1] = (samp_truth_value_t *)
		safe_malloc(table->size * sizeof(samp_truth_value_t));
	table->assignment_index = 0;
	table->assignment = table->assignments[table->assignment_index];
	table->pmodel = (int32_t *) safe_malloc(table->size * sizeof(int32_t));
	table->sampling_nums = (int32_t *) safe_malloc(table->size * sizeof(int32_t));

	for (i = 0; i < table->size; i++) {
		table->pmodel[i] = 0;//was -1
	}
	table->num_samples = 0;
	init_array_hmap(&table->atom_var_hash, ARRAY_HMAP_DEFAULT_SIZE);
	//  table->entries[0].atom = (samp_atom_t *) safe_malloc(sizeof(samp_atom_t));
	//  table->entries[0].atom->pred = 0;
}


/*
 * Deep copy of the atom table: This one concerns me.  It smells to me
 * like the main state object for MCMC.  Are we doing the right thing
 * here by copying everything?
 *
 * Need to pass down the pred_table to be able to look up predicates
 * and check their arity:
 */

/* Functionally equivalent to atom_copy( samp_atom_t *atom, int32_t arity),
 * but with a slightly different signature & internals:
 */

samp_atom_t *clone_atom( samp_atom_t *from, pred_table_t *pred_table) {
  int32_t arity, len;
  samp_atom_t *to;
  arity = pred_arity(from->pred, pred_table);
  
  len = (arity+1) * sizeof(int32_t);
  to = (samp_atom_t *) safe_malloc( len );
  memcpy(to, from, len);
  return to;
}

/*
 * Unlike other copies, this one needs the pred table:
 */
void copy_atom_table(atom_table_t *to, atom_table_t *from, samp_table_t *st) {
  pred_table_t *pt = &st->pred_table;
  uint32_t i, size;

  size = to->size = from->size;

  //  to->size = INIT_ATOM_TO_SIZE;
  to->num_vars = from->num_vars; //atoms are positive
  to->num_unfixed_vars = from->num_unfixed_vars;
  if (to->size >= MAXSIZE(sizeof(samp_atom_t *), 0)){
    out_of_memory();
  }

  /* Something special needs to happen here.  In reality, all of the
   * atoms have lengths that are dictated by the arity of the
   * predicate, whose index sits in the first word of the atom struct,
   * so we will need to walk and allocate atoms according to arity,
   * then do a copy: */
  
  /* Deeper copy needed?? */
  to->atom = (samp_atom_t **) safe_malloc(size * sizeof(samp_atom_t *));

  for (i = 0; i < to->num_vars; i++)
    to->atom[i] = clone_atom( from->atom[i], pt );

  /* Looks ok - just an array of booleans: */
  to->active = (bool *) safe_malloc(size * sizeof(bool));
  memcpy(to->active, from->active, size * sizeof(bool));

  /* An array of samp_truth_value_t; these are enums, so memcpy should be ok: */
  to->assignments[0] = (samp_truth_value_t *) safe_malloc(size * sizeof(samp_truth_value_t));
  memcpy(to->assignments[0], from->assignments[0], size * sizeof(samp_truth_value_t));

  /* An array of samp_truth_value_t; these are enums, so memcpy should be ok: */
  to->assignments[1] = (samp_truth_value_t *) safe_malloc(size * sizeof(samp_truth_value_t));
  memcpy(to->assignments[1], from->assignments[1], size * sizeof(samp_truth_value_t));

  /* Should be ok - probably overwritten during MCMC */
  to->assignment_index = from->assignment_index;
  to->assignment = from->assignments[from->assignment_index];

  /* Ok - an array of int32s: */
  to->pmodel = (int32_t *) safe_malloc(size * sizeof(int32_t));
  memcpy(to->pmodel, from->pmodel, size * sizeof(int32_t));

  /* Looks ok - an array of sampling counts: */
  to->sampling_nums = (int32_t *) safe_malloc(size * sizeof(int32_t));
  memcpy(to->sampling_nums, from->sampling_nums, size * sizeof(int32_t));

  /* What's the "right number" to use here??  This should probably
   * start fresh at 0 for every new copy, but here we will copy the
   * value in case the thread needs to know it:
   */
  to->num_samples = from->num_samples;

  /* Copy the atom hash table: */
  copy_array_hmap(&(to->atom_var_hash),  &(from->atom_var_hash) );
}


/*
 * It's not clear whether we want to do this, but for threading or
 * "cloud" operation, one possibility is to allocate copies of the
 * samp_table_t object and reset the counts before each usage.  This
 * function performs ONLY that task.
 */
void clear_atom_counts( atom_table_t *tbl ) {
  int i, nvars;
  tbl->num_samples = 0;
  nvars = tbl->num_vars;
  for (i = 0; i < nvars; i++) {
    tbl->pmodel[i] = 0;
    tbl->sampling_nums[i] = 0;
  }
}

void merge_atom_tables(atom_table_t *to, atom_table_t *from) {
  /* Tread carefully.  How much checking can / should we do here?
   * This function will sweep through the pmodel and sampling_nums
   * arrays and update 'to' based on the contents of 'from'.  The
   * point is to be able to merge atom tables after spawning multiple
   * threads.
   */
  int32_t i;
  uint32_t nvars = from->num_vars;
  if ( nvars != to->num_vars ) {
    printf("merge_atom_tables: Tables 'to' (%llx) and 'from' (%llx) differ in their num_vars (%d vs. %d)\n",
           (unsigned long long) to, (unsigned long long) from, to->num_vars, from->num_vars);
    printf("                   Cowardly refusing to merge atom tables.\n");
  } else {
    /* Here, we assume that clear_atom_counts was called BEFORE the
     * mcmc run, and that what appears here are differential
     * counts. */
    to->num_samples += from->num_samples;
    for (i = 0; i < nvars; i++) {
      to->pmodel[i] += from->pmodel[i];
      to->sampling_nums[i] += from->sampling_nums[i];
    }
  }
 }



/*
 * Resizes the atom table to fit the current atom_table->num_vars. When
 * atom_table is resized, the assignments and the watched literals must also be
 * resized. 
 */
void atom_table_resize(atom_table_t *atom_table, rule_inst_table_t *rule_inst_table) {
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
	atom_table->active = (bool *)
		safe_realloc(atom_table->active, size * sizeof(bool));
	atom_table->sampling_nums = (int32_t *)
		safe_realloc(atom_table->sampling_nums, size * sizeof(int32_t));
	atom_table->assignments[0] = (samp_truth_value_t *) 
		safe_realloc(atom_table->assignments[0], size * sizeof(samp_truth_value_t));
	atom_table->assignments[1] = (samp_truth_value_t *) 
		safe_realloc(atom_table->assignments[1], size * sizeof(samp_truth_value_t));
	atom_table->assignment = atom_table->assignments[atom_table->assignment_index];
	atom_table->pmodel = (int32_t *)
		safe_realloc(atom_table->pmodel, size * sizeof(int32_t));

	if (MAXSIZE(sizeof(samp_clause_t *), 0) - size <= size){
		out_of_memory();
	}
	rule_inst_table->watched = (samp_clause_list_t *) safe_realloc(
			rule_inst_table->watched, 2 * size * sizeof(samp_clause_list_t));

	for (i = atom_table->size; i < size; i++) {
		atom_table->pmodel[i] = 0;//was -1
	}
	atom_table->size = size;
}

void init_rule_inst_table(rule_inst_table_t *table){
	table->size = INIT_RULE_INST_TABLE_SIZE;
	table->num_rule_insts = 0;
	if (table->size >= MAXSIZE(sizeof(rule_inst_t *), 0)){
		out_of_memory();
	}
	table->rule_insts = (rule_inst_t **) safe_malloc(table->size * sizeof(rule_inst_t *));
	table->assignment = (samp_truth_value_t *) safe_malloc(
			table->size * sizeof(samp_truth_value_t));
	table->watched = (samp_clause_list_t *) safe_malloc(
			2 * INIT_ATOM_TABLE_SIZE * sizeof(samp_clause_list_t));
	table->rule_watched = (samp_clause_list_t *) safe_malloc(
			table->size * sizeof(samp_clause_list_t));
	init_clause_list(&table->sat_clauses);
	init_clause_list(&table->unsat_clauses);
	init_clause_list(&table->live_clauses);
	init_hmap(&table->unsat_soft_rules, HMAP_DEFAULT_SIZE);
}

/*
 * Deep copy of the rule instance table:
 */

samp_clause_t *clone_samp_clause( samp_clause_t *from ) {
  int32_t i, n, len;
  samp_clause_t *to;

  if (!from) return NULL;

  n = from->num_lits;
  len = sizeof(samp_clause_t) + (n+1) * sizeof(samp_literal_t);
  to = (samp_clause_t *) safe_malloc( len );

  to->rule_index = from->rule_index;
  to->num_lits = from->num_lits;

  /* samp_literal_t is a scalar (int), so just copy these: */
  for (i = 0; i < n; i++)
    to->disjunct[i] = from->disjunct[i];

  /* Must fill this in after the call! */
  to->link = clone_samp_clause( from->link );

  return to;
}

/*
 * Copy the linked list of clauses:
 */

void copy_samp_clause_list( samp_clause_list_t *to, samp_clause_list_t *from ) {
  int32_t i, length;
  //  samp_clause_t *to_next, *from_next;
  samp_clause_t  *tail;
  length = to->length = from->length;

  if (!from->head) {
    to->head = NULL;
    to->tail = NULL;
  } else {
    // from_next = from->head;

    /* clone_samp_clause will build out a copy of the entire list: */
    to->head = clone_samp_clause(from->head);
    tail = to->head;
    i = 0;
    while (tail->link) {
      tail = tail->link;
      i++;
    }
    /*      
    for (i = 0; i < length; i++) {
      from_next = from_next->link;
      if (from_next)
        tail = to_next->link = clone_samp_clause(from_next);
    }
    */
    to->tail = tail;
  }
  //  printf("Valid clause list returns %d\n", valid_clause_list(to));
}



rule_inst_t  *clone_rule_inst( rule_inst_t *from ) {
  int i, n, len;
  rule_inst_t *to;

  n = from->num_clauses;
  len = sizeof(double) + sizeof(int32_t) + (n+1)*sizeof(samp_clause_t*);
  to = (rule_inst_t *) safe_malloc(len);

  to->weight = from->weight;
  to->num_clauses = n;
  for (i = 0; i < n; i++)
    to->conjunct[i] = clone_samp_clause(from->conjunct[i]);

  return to;
}


void copy_rule_inst_table(rule_inst_table_t *to, rule_inst_table_t *from, samp_table_t *tbl) {
  int32_t i, size, n_insts, nvars;
  rule_inst_t *tmp;

  size = to->size = from->size;
  n_insts = to->num_rule_insts = from->num_rule_insts;

  nvars = (tbl->atom_table).num_vars;

  if (size >= MAXSIZE(sizeof(rule_inst_t *), 0)){
    out_of_memory();
  }

  to->soft_rules_included = from->soft_rules_included;
  to->unsat_weight = from->unsat_weight;

  to->rule_insts = (rule_inst_t **) safe_malloc(size * sizeof(rule_inst_t *));
  for (i = 0; i < to->num_rule_insts; i++) {
    tmp = clone_rule_inst(from->rule_insts[i]);
    to->rule_insts[i] = tmp;
  }

  /* Should be ok, since samp_truth_value_t is enum: */
  to->assignment = (samp_truth_value_t *) safe_malloc(size * sizeof(samp_truth_value_t));
  memcpy(to->assignment, from->assignment, size * sizeof(samp_truth_value_t));

  /* watched needs to be as large as 2*nvars: */
  to->watched = (samp_clause_list_t *) safe_malloc( 2 * nvars * sizeof(samp_clause_list_t) );

  for (i = 0; i < 2*nvars; i++)
    copy_samp_clause_list( &(to->watched[i]), &(from->watched[i]) );

  //  to->watched = clone_samp_clause_list( from->watched );
  to->rule_watched = (samp_clause_list_t *) safe_malloc(size * sizeof(samp_clause_list_t));
  for (i = 0; i < n_insts; i++) {
    copy_samp_clause_list( &(to->rule_watched[i]), &(from->rule_watched[i]) );
  }

  /* TO DO:  CHECK THESE AND DO THE RIGHT THING HERE: */
  
  copy_samp_clause_list(&to->sat_clauses, &from->sat_clauses);
  copy_samp_clause_list(&to->unsat_clauses, &from->unsat_clauses);
  copy_samp_clause_list(&to->live_clauses, &from->live_clauses);
  copy_hmap(&to->unsat_soft_rules, &from->unsat_soft_rules);
  //  init_hmap(&to->unsat_soft_rules, HMAP_DEFAULT_SIZE);
}



/*
 * Check whether there's room for one more clause in rule_inst_table.
 * - if not, make the table larger
 */
void rule_inst_table_resize(rule_inst_table_t *rule_inst_table){
	int32_t size = rule_inst_table->size;
	int32_t num_rinsts = rule_inst_table->num_rule_insts;
	if (num_rinsts + 1 < size) return;
	if (MAXSIZE(sizeof(rule_inst_t *), 0) - size <= (size/2)){
		out_of_memory();
	}
	size += size/2;
	rule_inst_table->rule_insts = (rule_inst_t **) safe_realloc(
			rule_inst_table->rule_insts, size * sizeof(rule_inst_t *));
	rule_inst_table->assignment = (samp_truth_value_t *) safe_realloc(
			rule_inst_table->assignment, size * sizeof(samp_truth_value_t));
	rule_inst_table->rule_watched = (samp_clause_list_t *) safe_realloc(
			rule_inst_table->rule_watched, size * sizeof(samp_clause_list_t));
	rule_inst_table->size = size; 
	//if (MAXSIZE(sizeof(samp_clause_t *), sizeof(rule_inst_t)) < num_clauses) {
	//	out_of_memory();
	//}
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

void copy_rule_atom( rule_atom_t *to, rule_atom_t *from, pred_table_t *pt) {
  int i, arity;
  to->pred = from->pred;
  arity = pred_arity(to->pred, pt);
  to->builtinop = from->builtinop;
  to->args = (rule_atom_arg_t *) safe_malloc( arity * sizeof(rule_atom_arg_t) );
  for (i = 0; i < arity; i++) {
    to->args[i].kind = from->args[i].kind;
    to->args[i].value = from->args[i].value;
  }
}

void copy_rule_literal( rule_literal_t *to, rule_literal_t *from, pred_table_t *pt ) {
  to->neg = from->neg;
  to->grounded = from->grounded;
  to->atom = (rule_atom_t *) safe_malloc(sizeof(rule_atom_t));
  copy_rule_atom( to->atom, from->atom, pt );
}


rule_clause_t *clone_rule_clause( rule_clause_t *from, pred_table_t *pt ) {
  int32_t i, n;
  rule_clause_t *to;

  n = from->num_lits;
  to = (rule_clause_t *) safe_malloc(sizeof(rule_clause_t) + (n+1) * sizeof(rule_literal_t *));
  to->num_lits = n;
  to->grounded = from->grounded;
  for (i = 0; i < n; i++) {
    to->literals[i] = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
    copy_rule_literal( to->literals[i], from->literals[i], pt );
  }
  return to;
}


samp_rule_t *clone_samp_rule( samp_rule_t *from, pred_table_t *pt  ) {
  int32_t i, full_len, size;
  samp_rule_t *to;

  full_len = sizeof(samp_rule_t) + (from->num_clauses+1) * sizeof(rule_clause_t *);

  to = (samp_rule_t *) safe_malloc( full_len );

  to->weight = from->weight;
  to->num_vars = from->num_vars;

  size = to->num_vars;
  to->vars = (var_entry_t **) safe_malloc(size*sizeof(var_entry_t));
  for (i = 0; i < to->num_vars; i++) {
    to->vars[i] = (var_entry_t *) safe_malloc( sizeof(var_entry_t) );
    copy_var_entry( to->vars[i], from->vars[i] );
  }

  to->num_clauses = from->num_clauses;
  for (i = 0; i < to->num_clauses; i++)
    to->clauses[i] = clone_rule_clause( from->clauses[i], pt );

  return to;
}


void copy_rule_table( rule_table_t *to, rule_table_t *from,  samp_table_t *st) {
  int32_t i, size, num_rules;
  pred_table_t *pt = &st->pred_table;

  size = to->size = from->size;
  num_rules = to->num_rules = from->num_rules;

  to->samp_rules = (samp_rule_t **) safe_malloc( size * sizeof(samp_rule_t *) );
  for (i = 0; i < num_rules; i++)
    to->samp_rules[i] = clone_samp_rule( from->samp_rules[i], pt );
}


/*
 * Resize the rule table to be at least 1 more than num_rules
 */
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

void init_query_table(query_table_t *table) {
	table->size = INIT_QUERY_TABLE_SIZE;
	table->num_queries = 0;
	if (table->size >= MAXSIZE(sizeof(samp_query_t *), 0)){
		out_of_memory();
	}
	table->query =
		(samp_query_t **) safe_malloc(table->size * sizeof(samp_query_t *));
}

void copy_samp_query( samp_query_t *to, samp_query_t *from, pred_table_t *pt ) {
  int i, j, m, n;
  rule_literal_t ***lits;

  to->source_index = from->source_index;
  m = to->num_clauses = from->num_clauses;
  n = to->num_vars = from->num_vars;
  to->vars = (var_entry_t **) safe_malloc( m * sizeof(var_entry_t *) );
  for (i = 0; i < m; i++) {
    to->vars[i] = (var_entry_t *) safe_malloc( sizeof(var_entry_t) );
    copy_var_entry( to->vars[i], from->vars[i] );
  }
  
  n = 0;

  lits = (rule_literal_t ***) safe_malloc(m * sizeof(rule_literal_t **));

  for (i = 0; i < m; i++) {
    if (from->literals[i] == NULL) lits[i] = NULL;
    else {
      for (n = 0; from->literals[i][n] != NULL; n++);
      lits[i] = (rule_literal_t **) safe_malloc(n * sizeof(rule_literal_t *));
      for (j = 0; j < n; j++) {
        lits[i][j] = (rule_literal_t *) safe_malloc( sizeof(rule_literal_t) );
        copy_rule_literal( lits[i][j], from->literals[i][j], pt );
      }
    }
  }

  to->literals = lits;
}


void copy_query_table( query_table_t *to, query_table_t *from, samp_table_t *tbl ) {
  int32_t i, n;
  pred_table_t *pt = &tbl->pred_table;
  to->size = from->size;
  n = to->num_queries = from->num_queries;
  if (from->query == NULL) to->query = NULL;
  else {
    to->query = (samp_query_t**) safe_malloc(n * sizeof(samp_query_t *));
    for (i = 0; i < n; i++) {
      to->query[i] = (samp_query_t*) safe_malloc(sizeof(samp_query_t));
      copy_samp_query( to->query[i], from->query[i], pt );
    }
  }
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


void copy_ivector( ivector_t *to, ivector_t *from ) {
  int n;
  n = to->capacity = from->capacity;
  to->size = from->size;
  to->data = (int32_t *) safe_malloc( n * sizeof(int32_t) );
  memcpy( &to->data, &from->data, n*sizeof(int32_t) );
}


void copy_samp_query_instance( samp_query_instance_t *to, samp_query_instance_t *from, query_table_t *qt ) {
  int32_t i, j, n, nvars;
  samp_query_t *query;

  copy_ivector( &(to->query_indices), &(from->query_indices) );
  to->sampling_num = from->sampling_num;
  to->pmodel = from->pmodel;

  //  printf("------\n");
  /* Hack */
  for (i = 0; i < to->query_indices.size; i++) {
    //    printf("query_index: %d, ", to->query_indices.data[i]);
    j = to->query_indices.data[i];
    if (j < qt->num_queries) {
      query = qt->query[j];
      nvars = query->num_vars;
      //      printf("nvars = %d\n", nvars);
    }
  }

  to->subst = (int32_t *) safe_malloc( nvars * sizeof(int32_t) );
  memcpy( &to->subst, &from->subst, nvars * sizeof(int32_t) );

  to->constp = (bool *) safe_malloc( nvars * sizeof(bool) );
  memcpy( &to->constp, &from->constp, nvars * sizeof(bool) );

  for (n = 0; from->lit[n] != NULL; n++);
  to->lit = (samp_literal_t **) safe_malloc( (n+1) * sizeof(samp_literal_t*));
  for (i = 0; i < n; i++) {
    for (j = 0; from->lit[i][j] != -1; j++);
    to->lit[i] = (samp_literal_t *) safe_malloc((j+1) * sizeof(samp_literal_t));
    memcpy( to->lit[i], from->lit[i], (j+1) * sizeof(samp_literal_t) );  // copy the trailing '-1'
  }
}


void copy_query_instance_table( query_instance_table_t *to, query_instance_table_t *from, samp_table_t *stbl ) {
  int i;
  query_table_t *qt = &(stbl->query_table);
  samp_query_t *query;

  to->size = from->size;
  to->num_queries = from->num_queries;
  to->query_inst = (samp_query_instance_t **) safe_malloc( to->size * sizeof(samp_query_instance_t *));
  for (i = 0; i < to->num_queries; i++) {
    to->query_inst[i] = (samp_query_instance_t *) safe_malloc( sizeof(samp_query_instance_t) );
    // query = qt->query[i];
    copy_samp_query_instance( to->query_inst[i], from->query_inst[i], qt); // query->num_vars );
  }
}


void merge_query_instance_tables(query_instance_table_t *to, query_instance_table_t *from) {
  /* Tread carefully.  How much checking can / should we do here?
   * This function will sweep through the pmodel and sampling_nums
   * arrays and update 'to' based on the contents of 'from'.  The
   * point is to be able to merge atom tables after spawning multiple
   * threads.
   */
  int32_t i;
  int32_t nq = from->num_queries;
  samp_query_instance_t *qifrom, *qito;

  if ( nq != to->num_queries ) {
    printf("merge_query_instance_tables: Tables 'to' (%llx) and 'from' (%llx) differ in their num_queries (%d vs. %d)\n",
           (unsigned long long) to, (unsigned long long) from, to->num_queries, from->num_queries);
    printf("                   Cowardly refusing to merge query instance tables.\n");
  } else {
    /* Here, we assume that clear_atom_counts was called BEFORE the
     * mcmc run, and that what appears here are differential
     * counts. */
    for (i = 0; i < from->num_queries; i++) {
      qifrom = from->query_inst[i];
      qito = to->query_inst[i];
      qito->pmodel += qifrom->pmodel;
      qito->sampling_num += qifrom->sampling_num; // Almost certainly wrong.
    }
  }
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

void copy_source_table( source_table_t *to, source_table_t *from ) {

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

void add_source_to_clause(char *source, int32_t rinst_index, double weight,
		samp_table_t *table) {
	source_table_t *source_table = &table->source_table;
	source_entry_t *source_entry;
	int32_t num_rinsts, srcidx;

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
		source_entry->rule_insts = (int32_t *) safe_malloc(2 * sizeof(int32_t));
		source_entry->weight = (double *) safe_malloc(2 * sizeof(double));
		source_entry->rule_insts[0] = rinst_index;
		source_entry->rule_insts[1] = -1;
		source_entry->weight[0] = weight;
		source_entry->weight[1] = -1;
	} else {
		source_entry = source_table->entry[srcidx];
		if (source_entry->rule_insts == NULL) {
			source_entry->rule_insts = (int32_t *) safe_malloc(2 * sizeof(int32_t));
			source_entry->weight = (double *) safe_malloc(2 * sizeof(double));
			source_entry->rule_insts[0] = rinst_index;
			source_entry->rule_insts[1] = -1;
			source_entry->weight[0] = rinst_index;
			source_entry->weight[1] = -1;
		} else {
			for (num_rinsts = 0; source_entry->assertion[num_rinsts] != -1; num_rinsts++) {
			}
			source_entry->rule_insts = (int32_t *) safe_realloc(
					source_entry->rule_insts, (num_rinsts + 2) * sizeof(int32_t));
			source_entry->weight = (double *) safe_realloc(
					source_entry->weight, (num_rinsts + 2) * sizeof(double));
			source_entry->rule_insts[num_rinsts] = rinst_index;
			source_entry->rule_insts[num_rinsts + 1] = -1;
			source_entry->weight[num_rinsts] = weight;
			source_entry->weight[num_rinsts + 1] = -1;
		}
	}
}

void add_source_to_assertion(char *source, int32_t atom_index,
		samp_table_t *table) {
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
		source_entry->rule_insts = NULL;
		source_entry->weight = NULL;
	} else {
		source_entry = source_table->entry[srcidx];
		if (source_entry->assertion == NULL) {
			source_entry->assertion = (int32_t *) safe_malloc(2 * sizeof(int32_t));
			source_entry->assertion[0] = atom_index;
			source_entry->assertion[1] = -1;
		} else {
			for (numassn = 0; source_entry->assertion[numassn] != -1; numassn++) {
			}
			source_entry->assertion = (int32_t *) safe_realloc(
					source_entry->assertion, (numassn + 2) * sizeof(int32_t));
			source_entry->assertion[numassn] = atom_index;
			source_entry->assertion[numassn + 1] = -1;
		}
	}
}

/* TODO what does this do */
void retract_source(char *source, samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	source_table_t *source_table = &table->source_table;
	source_entry_t *source_entry;
	int32_t i, j, srcidx, sidx, rinst_idx;
	rule_inst_t *rinst;
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
	if (source_entry->rule_insts != NULL) {
		for (i = 0; source_entry->rule_insts[i] != -1; i++) {
			rinst_idx = source_entry->rule_insts[i];
			rinst = rule_inst_table->rule_insts[rinst_idx];
			if (source_entry->weight[i] == DBL_MAX) {
				// Need to go through all other sources and add them up
				wt = 0.0;
				for (sidx = 0; sidx < source_table->num_entries; sidx++) {
					if (sidx != srcidx && source_table->entry[sidx]->rule_insts != NULL) {
						for (j = 0; source_table->entry[sidx]->rule_insts[j] != -1; j++) {
							if (source_table->entry[sidx]->rule_insts[j] == rinst_idx) {
								wt += source_table->entry[sidx]->weight[j];
								// Assume clause occurs only once for given source,
								// as we can simply sum up all such occurrences.
								break;
							}
						}
					}
				}
				rinst->weight = wt;
			} else if (rinst->weight != DBL_MAX) {
				// Subtract the weight of this source from the clause
				rinst->weight -= source_entry->weight[i];
			} // Nothing to do if current weight is DBL_MAX
		}
	}
}

void init_samp_table(samp_table_t *table) {
	init_sort_table(&table->sort_table);
	init_const_table(&table->const_table);
	init_var_table(&table->var_table);
	init_pred_table(&table->pred_table);
	init_atom_table(&table->atom_table);
	init_rule_inst_table(&table->rule_inst_table);
	init_rule_table(&table->rule_table);
	init_query_table(&table->query_table);
	init_query_instance_table(&table->query_instance_table);
	init_source_table(&table->source_table);
	//init_integer_stack(&table->fixable_stack, 0);
}

/* 
 * Checks the validity of the table for assertions within other functions.
 * valid_sort_table checks that the sort size is nonnegative, num_sorts is
 * at most size, and each sort name is hashed to the right index.
 */
bool valid_sort_table(sort_table_t *sort_table){
#ifdef VALIDATE
	assert(sort_table->size >= 0);
	assert(sort_table->num_sorts <= sort_table->size);
	if (sort_table->size < 0 || sort_table->num_sorts > sort_table->size)
		return false;
	uint32_t i = 0;
	while (i < sort_table->num_sorts &&
			i == stbl_find(&(sort_table->sort_name_index),
				sort_table->entries[i].name))
		i++;
	assert(i >= sort_table->num_sorts);
	if (i < sort_table->num_sorts) return false;
#endif
	return true;
}

/* Checks that the const names are hashed to the right index. */
bool valid_const_table(const_table_t *const_table, sort_table_t *sort_table){
#ifdef VALIDATE
	assert(const_table->size >= 0); 
	assert(const_table->num_consts <= const_table->size);
	if (const_table->size < 0 || const_table->num_consts > const_table->size)
		return false;
	uint32_t i = 0;
	while (i < const_table->num_consts &&
			i == stbl_find(&(const_table->const_name_index),
				const_table->entries[i].name) &&
			const_table->entries[i].sort_index < sort_table->num_sorts)
		i++;
	assert(i >= const_table->num_consts);
	if (i < const_table->num_consts) return false;
#endif
	return true;
}

/* 
 * Checks that evpred and pred names are hashed to the right index, and
 * have a valid signature. 
 */
bool valid_pred_table(pred_table_t *pred_table,
		sort_table_t *sort_table,
		atom_table_t *atom_table){
#ifdef VALIDATE
	pred_tbl_t *evpred_tbl = &(pred_table->evpred_tbl);
	pred_entry_t *entry;
	uint32_t i, j;

	assert(evpred_tbl->size >= 0);
	assert(evpred_tbl->num_preds <= evpred_tbl->size);
	if (evpred_tbl->size < 0 || evpred_tbl->num_preds > evpred_tbl->size) {
		printf("Invalid pred size for evpred_tbl\n");
		return false;
	}
	i = 0;
	while (i < evpred_tbl->num_preds){
		entry = &(evpred_tbl->entries[i]);
		assert(-i == pred_val_to_index(pred_index(entry->name, pred_table)));
		assert(entry->arity >= 0);
		if (-i != pred_val_to_index(pred_index(entry->name, pred_table))) {
			printf("Invalid pred_val_to_index for evpred_tbl\n");
			return false;
		}
		if (entry->arity < 0) {
			printf("Invalid arity for evpred_tbl\n");
			return false;
		}
		for (j = 0; j < entry->arity; j ++){
			assert(entry->signature[j] >= 0);
			assert(entry->signature[j] < sort_table->num_sorts);
			if (entry->signature[j] < 0 ||
					entry->signature[j] >= sort_table->num_sorts) {
				printf("Invalid signature sort for evpred_tbl\n");
				return false;
			}
		}
		assert(entry->size_atoms >= 0);
		assert(entry->num_atoms <= entry->size_atoms);
		if (entry->size_atoms < 0 || entry->num_atoms > entry->size_atoms) {
			printf("Invalid atoms size for evpred_tbl\n");
			return false;
		}
		for (j = 0; j < entry->num_atoms; j++){
			assert(entry->atoms[j] >= 0);
			assert(entry->atoms[j] < atom_table->num_vars);
			assert(atom_table->atom[entry->atoms[j]]->pred == -i);
			if (entry->atoms[j] < 0 ||
					entry->atoms[j] >= atom_table->num_vars ||
					atom_table->atom[entry->atoms[j]]->pred != -i) {
				printf("Invalid atom pred for evpred_tbl\n");
				return false;
			}
		}
		i++;
	}

	/* check pred_tbl */
	pred_tbl_t *pred_tbl = &(pred_table->pred_tbl);
	assert(pred_tbl->size >= 0);
	assert(pred_tbl->num_preds <= pred_tbl->size);
	if (pred_tbl->size < 0 || pred_tbl->num_preds > pred_tbl->size) {
		printf("Invalid pred_tbl size\n");
		return false;
	}
	i = 0;
	while (i < pred_tbl->num_preds){
		entry = &(pred_tbl->entries[i]);
		assert(i == pred_val_to_index(pred_index(entry->name, pred_table)));
		assert(entry->arity >= 0);
		if (i != pred_val_to_index(pred_index(entry->name, pred_table))) {
			printf("Invalid pred_val_to_index for pred_tbl\n");
			return false;
		}
		if (entry->arity < 0) return false;
		for (j = 0; j < entry->arity; j ++){
			assert(entry->signature[j] >= 0);
			assert(entry->signature[j] < sort_table->num_sorts);
			if (entry->signature[j] < 0 ||
					entry->signature[j] >= sort_table->num_sorts) {
				printf("Invalid signature for pred_tbl\n");
				return false;
			}
		}
		assert(entry->size_atoms >= 0);
		assert(entry->num_atoms <= entry->size_atoms);
		if (entry->size_atoms < 0 || entry->num_atoms > entry->size_atoms) {
			printf("Invalid atom for pred_tbl\n");
			return false;
		}
		for (j = 0; j < entry->num_atoms; j++){
			assert(entry->atoms[j] >= 0);
			assert(entry->atoms[j] < atom_table->num_vars);
			assert(atom_table->atom[entry->atoms[j]]->pred == i);
			if (entry->atoms[j] < 0 ||
					entry->atoms[j] >= atom_table->num_vars ||
					atom_table->atom[entry->atoms[j]]->pred != i) {
				printf("Invalid atom for pred_tbl\n");
				return false;
			}
		}
		i++;
	}
#endif
	return true;
}

/* 
 * Checks that each atom is well-formed. 
 */
bool valid_atom_table(atom_table_t *atom_table, pred_table_t *pred_table,
		const_table_t *const_table, sort_table_t *sort_table){
#ifdef VALIDATE
	assert(atom_table->size >= 0);
	assert(atom_table->num_vars <= atom_table->size);
	if (atom_table->size < 0 ||
			atom_table->num_vars > atom_table->size) {
		printf("Invalid atom table size\n");
		return false;
	}

	uint32_t i, j, k;
	int32_t pred, arity;
	int32_t num_unfixed = 0;
	int32_t *sig;
	sort_entry_t *sort_entry;
	int32_t arg;
	bool int_exists;

	for (i = 0; i < atom_table->num_vars; i++) {
		pred = atom_table->atom[i]->pred;
		arity = pred_arity(pred, pred_table);
		sig = pred_signature(pred, pred_table);
		if (unfixed_tval(atom_table->assignment[i])){
			num_unfixed++;
		}
		for (j = 0; j < arity; j++){
			sort_entry = &sort_table->entries[sig[j]];
			arg = atom_table->atom[i]->args[j];
			if (sort_entry->constants == NULL) {
				if (sort_entry->ints == NULL) {
					assert(arg >= sort_entry->lower_bound);
					assert(arg <= sort_entry->upper_bound);
					if (arg < sort_entry->lower_bound
							|| arg > sort_entry->upper_bound) {
						printf("Integer out of boundary\n");
					}
				} else {
					/* Enumerate integers, check if in the set */
					int_exists = false;
					for (k = 0; k < sort_entry->cardinality; k++) {
						if (arg == sort_entry->ints[k])
							int_exists = true;
					}
					assert(int_exists);
					if (!int_exists) {
						printf("Enumerate integer not in set\n");
					}
				}
			} else {
				/* constants have unique sorts, lookup directly */
				assert(const_table->entries[arg].sort_index == sig[j]);
				if (const_table->entries[arg].sort_index != sig[j]) {
					printf("Invalid atom sort\n");
					return false;
				}
			}
		}
		array_hmap_pair_t *hmap_pair;
		hmap_pair = array_size_hmap_find(&(atom_table->atom_var_hash),
				arity+1,
				(int32_t *) atom_table->atom[i]);
		assert(hmap_pair != NULL);
		assert(hmap_pair->val == i);
		if (hmap_pair == NULL ||
				hmap_pair->val != i) {
			printf("Invalid atom hmap_pair\n");
			return false;
		}
	}
	assert(num_unfixed == atom_table->num_unfixed_vars);
	if (num_unfixed != atom_table->num_unfixed_vars) {
		printf("Invalid atom num unfixed vars\n");
		return false;
	}
#endif
	return true;
}

/*
 * Returns if a clause contains the literal lit
 */
static bool clause_contains_lit(samp_clause_t *clause, samp_literal_t lit) {
	int32_t i;
	bool lit_exists = false;
	for (i = 0; i < clause->num_lits; i++) {
		if (clause->disjunct[i] == lit)
			lit_exists = true;
	}
	if (!lit_exists)
		return false;
	return true;
}

static bool valid_watched_lit(rule_inst_table_t *rule_inst_table, samp_literal_t lit,
		atom_table_t *atom_table) {
#ifdef VALIDATE
	valid_clause_list(&rule_inst_table->watched[lit]);

	bool lit_true = (is_pos(lit) && assigned_true(atom_table->assignment[var_of(lit)]))
	             || (is_neg(lit) && assigned_false(atom_table->assignment[var_of(lit)]));
	assert(is_empty_clause_list(&rule_inst_table->watched[lit]) || lit_true);
	if (!is_empty_clause_list(&rule_inst_table->watched[lit]) && !lit_true) {
		return false;
	}

	samp_clause_t *ptr;
	samp_clause_t *cls;
	for (ptr = rule_inst_table->watched[lit].head;
			ptr != rule_inst_table->watched[lit].tail;
			ptr = next_clause_ptr(ptr)) {
		cls = ptr->link;
		assert(clause_contains_lit(cls, lit));
		if (!clause_contains_lit(cls, lit))
			return false;
	}
#endif
	return true;
}

/*
 * Returns if a clause contains a literal that is fixed to true
 */
static bool clause_contains_fixed_true_lit(samp_clause_t *clause,
		samp_truth_value_t *assignment) {
	int32_t i;
	for (i = 0; i < clause->num_lits; i++) {
		if (assigned_fixed_true_lit(assignment, clause->disjunct[i]))
			return true;
	}
	return false;
}

/*
 * Validate all the lists in the clause table
 */
bool valid_rule_inst_table(rule_inst_table_t *rule_inst_table, atom_table_t *atom_table) {
#ifdef VALIDATE
	samp_clause_t *ptr;
	samp_clause_t *cls;
	int32_t lit, i;
	assert(rule_inst_table->size >= 0);
	assert(rule_inst_table->num_rule_insts >= 0);
	assert(rule_inst_table->num_rule_insts <= rule_inst_table->size);
	if (rule_inst_table->size < 0) return false;
	if (rule_inst_table->num_rule_insts < 0 
			|| rule_inst_table->num_rule_insts > rule_inst_table->size)
		return false;

	/* check that every clause in the unsat list is unsat */
	valid_clause_list(&rule_inst_table->unsat_clauses);
	for (ptr = rule_inst_table->unsat_clauses.head;
			ptr != rule_inst_table->unsat_clauses.tail;
			ptr = next_clause_ptr(ptr)) {
		cls = ptr->link;
		assert(eval_clause(atom_table->assignment, cls) == -1);
		if (eval_clause(atom_table->assignment, cls) != -1)
			return false;
	}

	///* check negative_or_unit_clauses */
	//valid_clause_list(&rule_inst_table->negative_or_unit_clauses);
	//for (ptr = rule_inst_table->negative_or_unit_clauses.head;
	//		ptr != rule_inst_table->negative_or_unit_clauses.tail;
	//		ptr = next_clause_ptr(ptr)) {
	//	cls = ptr->link;
	//	assert(cls->weight < 0 || cls->numlits == 1);
	//	if (cls->weight >= 0 && cls->numlits != 1) 
	//		return false;
	//}

	///* check dead_negative_or_unit_clauses */
	//valid_clause_list(&rule_inst_table->dead_negative_or_unit_clauses);
	//for (ptr = rule_inst_table->dead_negative_or_unit_clauses.head;
	//		ptr != rule_inst_table->dead_negative_or_unit_clauses.tail;
	//		ptr = next_clause_ptr(ptr)) {
	//	cls = ptr->link;
	//	assert(cls->weight < 0 || cls->numlits == 1);
	//	if (cls->weight >= 0 && cls->numlits != 1) 
	//		return false;
	//}

	/* check that every watched clause is satisfied */
	for (i = 0; i < atom_table->num_vars; i++) {
		lit = pos_lit(i);
		valid_watched_lit(rule_inst_table, lit, atom_table);

		lit = neg_lit(i);
		valid_watched_lit(rule_inst_table, lit, atom_table);
	}

	/* check the sat_clauses to see if the first disjunct is fixed true */
	valid_clause_list(&rule_inst_table->sat_clauses);
	for (ptr = rule_inst_table->sat_clauses.head;
			ptr != rule_inst_table->sat_clauses.tail;
			ptr = next_clause_ptr(ptr)) {
		cls = ptr->link;
		assert(clause_contains_fixed_true_lit(cls, atom_table->assignment));
		if (!clause_contains_fixed_true_lit(cls, atom_table->assignment))
			return false;
	}
	
	/* check the live_clauses */
	valid_clause_list(&rule_inst_table->sat_clauses);

	/* check all the clauses to see if they are properly indexed */
	rule_inst_t *rinst;
	for (i = 0; i < rule_inst_table->num_rule_insts; i++){
		rinst = rule_inst_table->rule_insts[i];
		assert(rinst->num_clauses >= 0);
		if (rinst->num_clauses < 0) 
			return false;
		//for (j = 0; j < clause->numlits; j++){
		//	assert(var_of(clause->disjunct[j]) >= 0);
		//	assert(var_of(clause->disjunct[j]) < atom_table->num_vars);
		//	if (var_of(clause->disjunct[j]) < 0 ||
		//			var_of(clause->disjunct[j]) >= atom_table->num_vars)
		//		return false;
		//}
	}
#endif
	return true;
}

bool valid_table(samp_table_t *table){
#if VALIDATE
	sort_table_t *sort_table = &(table->sort_table); 
	const_table_t *const_table = &(table->const_table);
	atom_table_t *atom_table = &(table->atom_table);
	pred_table_t *pred_table = &(table->pred_table);
	rule_inst_table_t *rule_inst_table = &(table->rule_inst_table);

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
	if (!valid_rule_inst_table(rule_inst_table, atom_table)) {
		printf("Invalid rule_inst_table\n");
		return false;
	}
#endif
	return true;
}



/*
 * Deep copy of the sample table, mainly for allowing us to run
 * multithreaded mcmc.  Corresponding functions are being written for
 * each other table type and relevant slot types.  The idea is that
 * each MCMC thread will need its own samp_table_t table.  Rather than
 * use mutexes and a single table, it should be faster to allow
 * multiple threads to run asynchronously, each with its own copy of
 * the tables.  Deep copy could also serve as a prelude to pickling,
 * especially if we want to share computation across a network...
 */

samp_table_t *clone_samp_table(samp_table_t *table) {
  samp_table_t *clone;

  clone = (samp_table_t *) safe_malloc( sizeof(samp_table_t) );

  copy_sort_table( &clone->sort_table, &(table->sort_table) );
  copy_const_table( &clone->const_table, &(table->const_table) );
  copy_var_table( &clone->var_table, &(table->var_table) );
  copy_pred_table( &clone->pred_table, &(table->pred_table) );
  copy_atom_table( &clone->atom_table, &(table->atom_table), clone );

  copy_rule_table(&clone->rule_table, &(table->rule_table), clone );
  copy_rule_inst_table(&(clone->rule_inst_table), &(table->rule_inst_table), clone );

  copy_query_table(&clone->query_table, &(table->query_table), clone );
  copy_query_instance_table(&clone->query_instance_table, &(table->query_instance_table), clone );

  /* Still need to implement this! */
  //  copy_source_table(&clone->source_table, &(table->source_table) );

  /* Let's do this no matter what - it's a one-shot check of the copy
     before we spawn threads, so it won't incur enough overhead to be
     a problem: */
  assert(valid_table(clone));
  return clone;
}

