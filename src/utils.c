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
#include "SFMT.h"
#include "buffer.h"

int32_t imax(int32_t i, int32_t j) {return i<j ? j : i;}
int32_t imin(int32_t i, int32_t j) {return i>j ? j : i;}

/*  
 * These copy operations assume that size input to safe_malloc can't 
 * overflow since there are existing structures of the given size. 
 */
char *str_copy(char *name){
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

/* converts string to integer */
int32_t str2int(char *cnst) {
	int32_t i;
	long int lcnst;

	for (i = (cnst[0] == '+' || cnst[0] == '-') ? 1 : 0; cnst[i] != '\0'; i++) {
		if (!isdigit(cnst[i])) {
			mcsat_err("%s is not a valid integer\n", cnst);
		}
	}
	lcnst = strtol(cnst, NULL, 10);
	if (lcnst > INT32_MAX) {
		mcsat_err("Integer %s is too big, above %"PRId32"", cnst, INT32_MAX);
	}
	if (lcnst < INT32_MIN) {
		mcsat_err("Integer %s is too small, below %"PRId32"", cnst, INT32_MIN);
	}
	return (int32_t) lcnst;
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

/*
 * Evaluates a clause to false (-1) or to the literal index evaluating to true.
 */
int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause){
	int32_t i;
	for (i = 0; i < clause->numlits; i++){
		if (assigned_true_lit(assignment, clause->disjunct[i]))
			return i;
	}
	return -1;
}

/* retrieves a predicate structure by the index */
pred_entry_t *get_pred_entry(pred_table_t *pred_table, int32_t predicate){
	if (predicate <= 0){
		return &(pred_table->evpred_tbl.entries[-predicate]);
	} else {
		return &(pred_table->pred_tbl.entries[predicate]); 
	}
}

/* returns the sort name of a constant */
char *const_sort_name(int32_t const_idx, samp_table_t *table) {
	sort_table_t *sort_table = &table->sort_table;
	const_table_t *const_table = &table->const_table;
	int32_t const_sig;

	const_sig = const_table->entries[const_idx].sort_index;
	return sort_table->entries[const_sig].name;
}

/* frees an array of strings */
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

extern substit_buffer_t substit_buffer;

/* 
 * Converts a rule_atom to a samp_atom. If ratom is a builtinop, return NULL.
 */
samp_atom_t *rule_atom_to_samp_atom(rule_atom_t *ratom, substit_entry_t *substs, 
		pred_table_t *pred_table) {
	if (ratom->builtinop > 0) {
		return NULL;
	}
	int32_t i;
	int32_t arity = pred_arity(ratom->pred, pred_table);
	samp_atom_t *satom = (samp_atom_t *) safe_malloc((arity+1) * sizeof(int32_t));
	satom->pred = ratom->pred;
	for (i = 0; i < arity; i++) {
		if (ratom->args[i].kind == variable) {
			satom->args[i] = substit_buffer.entries[ratom->args[i].value];
		} else {
			satom->args[i] = ratom->args[i].value;
		}
	}
	return satom;
}

/* Returns the index of an atom, if not exist, returns -1 */
int32_t samp_atom_index(samp_atom_t *atom, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t arity = atom_arity(atom, pred_table);
	array_hmap_pair_t *atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
			arity + 1, (int32_t *) atom);
	if (atom_map == NULL) {
		return -1;
	}
}

/* Converts a rule_literal to a samp_literal */
samp_literal_t rule_lit_to_samp_lit(rule_literal_t *rlit, substit_entry_t *substs,
		samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	bool neg = rlit->neg;
	rule_atom_t *ratom = rlit->atom;
	samp_atom_t *satom = rule_atom_to_samp_atom(ratom, substs, pred_table);
	int32_t atom_index = samp_atom_index(satom, table);
	return neg? pos_lit(atom_index) : neg_lit(atom_index);
}

// insertion sort
void isort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
	uint32_t i, j;
	int32_t x, y;
	double u, v;

	for (i = 1; i < n; i++) {
		x = a[i];
		u = p[i];
		j = 0;
		while (p[j] > u)
			j++;
		while (j < i) {
			y = a[j];
			v = p[j];
			a[j] = x;
			p[j] = u;
			x = y;
			u = v;
			j++;
		}
		a[j] = x;
		p[j] = u;
	}
}

inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
	if (n <= 10) {
		isort_query_atoms_and_probs(a, p, n);
	} else {
		qsort_query_atoms_and_probs(a, p, n);
	}
}

// quick sort: requires n > 1
void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
	uint32_t i, j;
	int32_t x, y;
	double u, v;

	// u = random pivot
	// i = random_uint(n);
	i = genrand_uint(n);
	x = a[i];
	u = p[i];

	// swap x and a[0], u and p[0]
	a[i] = a[0];
	p[i] = p[0];
	a[0] = x;
	p[0] = u;

	i = 0;
	j = n;

	do {
		j--;
	} while (p[j] < u);
	do {
		i++;
	} while (i <= j && p[i] > u);

	while (i < j) {
		y = a[i];
		v = p[i];
		a[i] = a[j];
		p[i] = p[j];
		a[j] = y;
		p[j] = v;

		do {
			j--;
		} while (p[j] < u);
		do {
			i++;
		} while (p[i] > u);
	}

	// pivot goes into a[j]
	a[0] = a[j];
	p[0] = p[j];
	a[j] = x;
	p[j] = u;

	// sort a[0...j-1] and a[j+1 .. n-1]
	sort_query_atoms_and_probs(a, p, j);
	j++;
	sort_query_atoms_and_probs(a + j, p + j, n - j);
}

