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
			value == v_up_true);
}

static inline bool assigned_false(samp_truth_value_t value){
	return (value == v_false ||
			value == v_db_false ||
			value == v_up_false);
}

static inline bool assigned_fixed_true(samp_truth_value_t value) {
	return (value == v_db_true ||
			value == v_up_true);
}

static inline bool assigned_fixed_false(samp_truth_value_t value) {
	return (value == v_db_false ||
			value == v_up_false);
}

/* Returns whether the value is fixed in input database */
static inline bool db_tval(samp_truth_value_t v){
	return (v == v_db_true || v == v_db_false);
}

/* Returns whether the value is fixed by unit propagation */
static inline bool up_tval(samp_truth_value_t v) {
	return (v == v_up_true || v == v_up_false);
}

/* Returns whether the value is fixed */
static inline bool fixed_tval(samp_truth_value_t v){
	return (v == v_db_true ||
			v == v_db_false ||
			v == v_up_true ||
			v == v_up_false);
}

/* Returns if the value is unfixed */
static inline bool unfixed_tval(samp_truth_value_t v){
	return (v == v_true || v == v_false);
}

/* Changes a value from fixed to unfixed, but keep the truth value */
static inline samp_truth_value_t unfix_tval(samp_truth_value_t v) {
	if (v == v_up_true) {
		return v_true;
	}
	else if (v == v_up_false) {
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
	case v_up_true:
		return v_up_false;
	case v_up_false:
		return v_up_true;
	default:
		return v_undef;
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

/* the rule atom is a direct predicate or builtinop */
static inline bool rule_atom_is_direct(rule_atom_t *rule_atom) {
	return (rule_atom->builtinop > 0 || rule_atom->pred <= 0);
}

/* all atoms in the clause are direct predicates */
static inline bool rule_clause_is_direct(rule_clause_t *rule_clause) {
	int32_t i;
	for (i = 0; i < rule_clause->num_lits; i++) {
		if (!rule_atom_is_direct(rule_clause->literals[i]->atom)) {
			return false;
		}
	}
	return true;
}

extern int32_t samp_atom_index(samp_atom_t *atom, samp_table_t *table);

extern inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// Based on Bruno's sort_int_array - works on two arrays in parallel
extern void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// Insertion sort
extern void isort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

/* Evaluates a clause to false (-1) or to the literal index evaluating to true. */
extern int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause);

static inline bool sat_clause(samp_truth_value_t *assignment, samp_clause_t *clause) {
	return (eval_clause(assignment, clause) != -1);
}

/* Evaluates a rule instance to -1 if sat or the index of an unsat clause */
extern int32_t eval_rule_inst(samp_truth_value_t *assignment, rule_inst_t *rinst);

static inline bool sat_rule_inst(samp_truth_value_t *assignment, rule_inst_t *rinst) {
	return (eval_rule_inst(assignment, rinst) == -1);
}

extern void copy_assignment_array(atom_table_t *atom_table);
extern void restore_assignment_array(atom_table_t *atom_table);

// The builtin binary predicates
extern char* builtinop_string (int32_t bop);
extern int32_t builtin_arity (int32_t op);
extern bool call_builtin (int32_t bop, int32_t arity, int32_t *args);

#endif /* __UTILS_H */

