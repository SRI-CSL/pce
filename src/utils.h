#ifndef __UTILS_H
#define __UTILS_H 1

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "SFMT.h"
#include "math.h"
#include "tables.h"

#ifdef _WIN32
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#endif

/*
 * Missing comment: what does this do?
 * FIXME: MAX_SIZE is used already. We need a different name
 */
#define MAXSIZE(size, offset) ((UINT32_MAX - (offset))/(size))

#ifdef MINGW
/*
 * Need some version of random()
 * rand() exists on mingw but random() does not
 */
static inline int random(void) {
	return rand();
}
#endif

/*
 * Random number generator:
 * - returns a floating point number in the interval [0.0, 1.0)
 */
static inline double choose() {
	//return ((double) random()) / ((double) RAND_MAX + 1.0);
	//return ((double) gen_rand64()) / ((double) UINT64_MAX + 1.0);
	return genrand_real2();
}

static inline uint32_t genrand_uint(uint32_t n) {
	return (uint32_t) floor(genrand_real2() * n);
}

/* max of two integers */
static inline int32_t imax(int32_t i, int32_t j) {
	return i<j ? j : i;
}

/* min of two integers */
static inline int32_t imin(int32_t i, int32_t j) {
	return i>j ? j : i;
}

/*
 * Return a freshly allocated copy of string s.
 * - the result must be deleted by calling safe_free later
 */
extern char *str_copy(char *s);
extern void free_strings (char **string);

/* Similarly, returns a copy of an atom */
extern samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity);

/* Similarly, returns a copy of an integer array */
extern int32_t *intarray_copy(int32_t *signature, int32_t length);

/* Converts a string to an integer */
extern int32_t str2int(char *cnst);

static inline bool assigned_undef(samp_truth_value_t value){
	return (value == v_undef);
}

static inline bool assigned_true(samp_truth_value_t value){
	return (value == v_true ||
			value == v_db_true ||
			value == v_fixed_true);
}

static inline bool assigned_false(samp_truth_value_t value){
	return (value == v_false ||
			value == v_db_false ||
			value == v_fixed_false);
}

/* Returns if the value is fixed in input database */
static inline bool db_tval(samp_truth_value_t v){
	return (v == v_db_true || v == v_db_false);
}

/* Returns if the value is fixed by unit propagation */
static inline bool fixed_tval(samp_truth_value_t v){
	return (v == v_fixed_true || v == v_fixed_false);
}

/* Returns if the value is unfixed during walksat */
static inline bool unfixed_tval(samp_truth_value_t v){
	return (v == v_true || v == v_false);
}

/* Changes a value from fixed to unfixed, but keep the truth value */
static inline samp_truth_value_t unfix_tval(samp_truth_value_t v) {
	if (v == v_fixed_true) {
		return v_true;
	}
	else if (v == v_fixed_false) {
		return v_false;
	}
	else {
		return v;
	}
}

/* Negates a value */
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

/* String of a truth value */
static inline char *string_of_tval(samp_truth_value_t tval) {
	switch (tval) {
	case v_undef:
		return "undef";
	case v_false:
		return "false";
	case v_true:
		return "true";
	case v_fixed_false:
		return "fixed false";
	case v_fixed_true:
		return "fixed true";
	case v_db_false:
		return "db false";
	case v_db_true:
		return "db true";
	default:
		return NULL;
	}
}

extern bool assigned_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_fixed_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_fixed_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);

/* Returns the name of a constant */
extern char *const_sort_name(int32_t const_idx, samp_table_t *table);

extern bool subsort_p(int32_t sig1, int32_t sig2, sort_table_t *sort_table);

extern int32_t least_common_supersort(int32_t sig1, int32_t sig2, sort_table_t *sort_table);
extern int32_t greatest_common_subsort(int32_t sig1, int32_t sig2, sort_table_t *sort_table);

extern int32_t rule_atom_arity(rule_atom_t *atom, pred_table_t *pred_table);
extern int32_t samp_atom_index(samp_atom_t *atom, samp_table_t *table);
extern samp_atom_t *rule_atom_to_samp_atom(rule_atom_t *ratom, substit_entry_t *substs,
		pred_table_t *pred_table);
extern samp_literal_t rule_lit_to_samp_lit(rule_literal_t *rlit, substit_entry_t *substs,
		samp_table_t *table);

static inline void switch_assignment_array(atom_table_t *atom_table) {
	atom_table->assignment_index ^= 1; // flip low order bit: 1 --> 0, 0 --> 1
	atom_table->assignment = atom_table->assignments[atom_table->assignment_index];
}

static inline void switch_and_copy_assignment_array(atom_table_t *atom_table) {
	samp_truth_value_t *old_assignment = atom_table->assignment;
	atom_table->assignment_index ^= 1; // flip low order bit: 1 --> 0, 0 --> 1
	atom_table->assignment = atom_table->assignments[atom_table->assignment_index];
	memcpy(atom_table->assignment, old_assignment,
			sizeof(samp_truth_value_t) * atom_table->num_vars);
}

extern inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// Based on Bruno's sort_int_array - works on two arrays in parallel
extern void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// Insertion sort
extern void isort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

/* Evaluates a clause to false (-1) or to the literal index evaluating to true. */
extern int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause);

// The builtin binary predicates
extern char* builtinop_string (int32_t bop);
extern int32_t builtin_arity (int32_t op);
extern bool call_builtin (int32_t bop, int32_t arity, int32_t *args);

#endif /* __UTILS_H */

