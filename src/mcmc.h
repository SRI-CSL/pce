#ifndef __MCMC_H
#define __MCMC_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "array_hash_map.h"
#include "symbol_tables.h"
#include "integer_stack.h"
#include "utils.h"

#define MCSAT_CONFLICT 21

void inline push_clause(samp_clause_t *clause, samp_clause_t **list);
void inline push_negative_or_unit_clause(clause_table_t *clause_table, uint32_t i);
void inline push_unsat_clause(clause_table_t *clause_table, uint32_t i);
void inline push_sat_clause(clause_table_t *clause_table, uint32_t i);

//extern int32_t add_var(var_table_t *var_table,
//		       char *name,
//		       sort_table_t *sort_table,
//		       char * sort_name);

extern int32_t add_atom(samp_table_t *table, input_atom_t *current_atom);

extern int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p);

extern int32_t assert_atom(samp_table_t *table, input_atom_t *current_atom, char *source);

bool member_frozen_preds(samp_literal_t lit, int32_t *frozen_preds,
		samp_table_t *table);

extern int32_t add_clause(samp_table_t *table,
			  input_literal_t **in_clause,
			  double weight, char *source, bool add_weights);

extern int32_t add_internal_clause(samp_table_t *table,
				   int32_t *clause,
				   int32_t length,
				   int32_t *fixed_preds,
				   double weight,
				   bool indirect,
				   bool add_weights);

void add_source_to_clause(char *source, int32_t clause_index, double weight,
		samp_table_t *table);

extern int32_t add_rule(input_clause_t *in_rule,
			double weight,
			char *source,
			samp_table_t *samp_table);

extern int32_t add_query(var_entry_t **vars, rule_literal_t ***lits,
			 samp_table_t *table);

extern int32_t add_subst_query_instance(samp_literal_t **litinst, substit_entry_t *substs,
					samp_query_t *query, samp_table_t *table);

extern void all_pred_instances(char *pred, samp_table_t *table);

extern void all_rule_instances(int32_t rule, samp_table_t *table);

extern void all_rule_instances_rec(int32_t vidx, samp_rule_t *rule, 
		samp_table_t *table, int32_t atom_index);

extern int32_t samp_query_to_query_instance(samp_query_t *query,
					    samp_table_t *table);

extern void all_query_instances_rec(int32_t vidx, samp_query_t *query,
				    samp_table_t *table, int32_t atom_index);
  
extern void all_query_instances(samp_query_t *query, samp_table_t *table);

extern void update_pmodel(samp_table_t *table);

extern void empty_clause_lists(samp_table_t *table);

extern void init_clause_lists(clause_table_t *clause_table);

extern void create_new_const_atoms(int32_t cidx, int32_t csort,
				   samp_table_t *table);

extern void create_new_const_rule_instances(int32_t constidx,
					    int32_t csort,
					    samp_table_t *table,
					    int32_t atom_index);

extern void create_new_const_query_instances(int32_t constidx,
					     int32_t csort,
					     samp_table_t *table,
					     int32_t atom_index);

int32_t reset_sample_sat(samp_table_t *table);

void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
	    double samp_temperature, double rvar_probability,
	    uint32_t max_flips, uint32_t max_extra_flips, uint32_t timeout,
	    uint32_t burn_in_steps, uint32_t samp_interval);

bool eql_samp_atom(samp_atom_t *atom1, samp_atom_t *atom2, samp_table_t *table);

bool eql_rule_literal(rule_literal_t *lit1, rule_literal_t *lit2, samp_table_t *table);

bool eql_query_entries(rule_literal_t ***lits, samp_query_t *query, samp_table_t *table);

bool eql_query_instance_lits(samp_literal_t **lit1, samp_literal_t **lit2);

void retract_source(char *source, samp_table_t *table);

#endif /* __MCMC_H */

