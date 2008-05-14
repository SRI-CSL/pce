#ifndef __SAMPLESAT_H
#define __SAMPLESAT_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "array_hash_map.h"
#include "symbol_tables.h"
#include "integer_stack.h"
#include "utils.h"

#define MCSAT_CONFLICT 21

#define DEFAULT_SA_PROBABILITY .5
#define DEFAULT_SAMP_TEMPERATURE 0.91
#define DEFAULT_RVAR_PROBABILITY .2
#define DEFAULT_MAX_FLIPS 1000
#define DEFAULT_MAX_SAMPLES 1000

extern int32_t add_const(const_table_t *const_table,
		      char *name,
		      sort_table_t *sort_table,
		      char * sort_name);

extern int32_t add_var(var_table_t *var_table,
		      char *name,
		      sort_table_t *sort_table,
		      char * sort_name);

extern int32_t add_pred(pred_table_t *pred_table,
		     char *name,
		     bool evidence,
		     int32_t arity,
		     sort_table_t *sort_table,
		     char **in_signature);

extern int32_t add_atom(samp_table_t *table, input_atom_t *current_atom);

extern int32_t assert_atom(samp_table_t *table, input_atom_t *current_atom);

extern int32_t add_clause(samp_table_t *table,
			  input_literal_t **in_clause,
			  double weight);

extern int32_t add_rule(input_clause_t *in_rule,
			double weight,
			samp_table_t *samp_table);

extern void all_ground_instances_of_rule(int32_t rule, samp_table_t *table);

extern void create_new_const_rule_instances(int32_t constidx,
					    samp_table_t *table);

extern void link_propagate(samp_table_t *table,
		    samp_literal_t lit);

extern int32_t get_fixable_literal(samp_truth_value_t *assignment,
			    samp_literal_t *disjunct,
			    int32_t numlits);

extern void scan_unsat_clauses(samp_table_t *table);

extern void negative_unit_propagate(samp_table_t *table);
  
extern void init_random_assignment(samp_truth_value_t *assignment, int32_t num_vars,
				   int32_t *num_unfixed_vars);

extern void init_sample_sat(samp_table_t *table);

bool valid_atom_table(atom_table_t *atom_table,
		      pred_table_t *pred_table,
		      const_table_t *const_table,
		      sort_table_t *sort_table);

bool valid_table(samp_table_t *table);

void reset_sample_sat(samp_table_t *table);

void sample_sat_body(samp_table_t *table, double sa_probability,
		     double samp_temperature, double rvar_probability);

void sample_sat(samp_table_t *table, double sa_probability,
		double samp_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_flips);

void mc_sat(samp_table_t *table, double sa_probability,
		double samp_temperature, double rvar_probability,
	    uint32_t max_flips, uint32_t max_extra_flips, uint32_t max_samples);

#endif /* __SAMPLESAT_H */     

