#ifndef __SAMPLESAT_H
#define __SAMPLESAT_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

#define MCSAT_CONFLICT 21

extern void first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability, uint32_t max_flips);

extern void sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips);

extern int32_t sample_sat_body(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability);

extern int32_t choose_unfixed_variable(samp_truth_value_t *assignment, 
		int32_t num_vars, int32_t num_unfixed_vars);

extern double choose();

extern int32_t choose_random_atom(samp_table_t *table);

extern int32_t choose_clause_var(samp_table_t *table,
		samp_clause_t *link,
		samp_truth_value_t *assignment,
		double rvar_probability);

extern void cost_flip_unfixed_variable(samp_table_t *table,
		int32_t *dcost, int32_t var);

extern int32_t flip_unfixed_variable(samp_table_t *table, int32_t var);

extern void link_propagate(samp_table_t *table, samp_literal_t lit);

extern int32_t get_fixable_literal(samp_truth_value_t *assignment,
		samp_literal_t *disjunct, int32_t numlits);

extern int32_t scan_unsat_clauses(samp_table_t *table);

extern int32_t negative_unit_propagate(samp_table_t *table);

extern void init_random_assignment(samp_table_t *table, int32_t *num_unfixed_vars);

extern int32_t init_first_sample_sat(samp_table_t *table);
extern int32_t init_sample_sat(samp_table_t *table);

extern int32_t set_atom_tval(int32_t var, samp_truth_value_t tval, samp_table_t *table);

extern int32_t fix_lit_tval(samp_table_t *table, int32_t lit, bool tval);
extern int32_t fix_lit_true(samp_table_t *table, int32_t lit);
extern int32_t fix_lit_false(samp_table_t *table, int32_t lit);

#endif /* __SAMPLESAT_H */

