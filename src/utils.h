#ifndef __UTILS_H
#define __UTILS_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

#ifdef _WIN32
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#endif

/*
 * Missing comment: what does this do?
 * FIXME: MAX_SIZE is used already. We need a different name
 */
#define MAXSIZE(size, offset) ((UINT32_MAX - (offset))/(size))

/* generates a random number in [0, 1) */
inline double choose();

/*
 * min and max of two integers
 */
extern int32_t imax(int32_t i, int32_t j);
extern int32_t imin(int32_t i, int32_t j);

/*
 * Return a freshly allocated copy of string s.
 * - the result must be deleted by calling safe_free later
 */
extern char *str_copy(char *s);
extern samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity);
extern int32_t *intarray_copy(int32_t *signature, int32_t length);

extern int32_t str2int(char *cnst);

extern bool assigned_undef(samp_truth_value_t value);
extern bool assigned_true(samp_truth_value_t value);
extern bool assigned_false(samp_truth_value_t value);
extern bool assigned_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_fixed_true_lit(samp_truth_value_t *assignment, samp_literal_t lit);
extern bool assigned_fixed_false_lit(samp_truth_value_t *assignment, samp_literal_t lit);

static inline bool db_tval(samp_truth_value_t v){
	return (v == v_db_true || v == v_db_false);
}

static inline bool fixed_tval(samp_truth_value_t v){
	return (v == v_fixed_true || v == v_fixed_false);
}

static inline bool unfixed_tval(samp_truth_value_t v){
	return (v == v_true || v == v_false);
}

static inline samp_truth_value_t unfix_tval(samp_truth_value_t v) {
	if (assigned_true(v)) {
		return v_true;
	} 
	else if (assigned_false(v)) {
		return v_false;
	}
	else {
		return v_undef;
	}
}

/*
 * This function is too large to be inlined
 */
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

extern void free_strings (char **string);

extern char *const_sort_name(int32_t const_idx, samp_table_t *table);

extern int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause);
extern int32_t eval_neg_clause(samp_truth_value_t *assignment, samp_clause_t *clause);

extern bool subsort_p(int32_t sig1, int32_t sig2, sort_table_t *sort_table);
extern int32_t least_common_supersort(int32_t sig1, int32_t sig2, sort_table_t *sort_table);
extern int32_t greatest_common_subsort(int32_t sig1, int32_t sig2, sort_table_t *sort_table);

extern int32_t samp_atom_index(samp_atom_t *atom, samp_table_t *table);
extern samp_atom_t *rule_atom_to_samp_atom(rule_atom_t *ratom, substit_entry_t *substs,
		pred_table_t *pred_table);
extern samp_literal_t rule_lit_to_samp_lit(rule_literal_t *rlit, substit_entry_t *substs,
		samp_table_t *table);

extern inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// Based on Bruno's sort_int_array - works on two arrays in parallel
extern void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

// insertion sort
extern void isort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n);

#endif /* __UTILS_H */     
