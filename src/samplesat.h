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

extern int32_t add_var(var_table_t *var_table,
		       char *name,
		       sort_table_t *sort_table,
		       char * sort_name);

/* extern int32_t add_pred(pred_table_t *pred_table, */
/* 		     char *name, */
/* 		     bool evidence, */
/* 		     int32_t arity, */
/* 		     sort_table_t *sort_table, */
/* 		     char **in_signature); */

extern clause_buffer_t atom_buffer;

extern void atom_buffer_resize(int32_t arity);

/* this is static */
// extern clause_buffer_t rule_atom_buffer;

extern void rule_atom_buffer_resize(int32_t length);

extern int32_t rule_atom_default_value(rule_atom_t *rule_atom, pred_table_t *pred_table);
extern int32_t pred_default_value(pred_entry_t *pred);

extern int32_t set_atom_tval(int32_t var, samp_truth_value_t tval, samp_table_t *table);

extern int32_t fix_lit_tval(samp_table_t *table, int32_t lit, bool tval);
extern int32_t fix_lit_true(samp_table_t *table, int32_t lit);
extern int32_t fix_lit_false(samp_table_t *table, int32_t lit);

extern int32_t add_atom(samp_table_t *table, input_atom_t *current_atom);

extern int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p);

extern int32_t assert_atom(samp_table_t *table, input_atom_t *current_atom, char *source);

extern void all_pred_instances(char *pred, samp_table_t *table);

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

extern int32_t add_rule(input_clause_t *in_rule,
			double weight,
			char *source,
			samp_table_t *samp_table);

extern void add_rule_to_pred(pred_table_t *pred_table,
			     int32_t predicate,
			     int32_t current_rule_index);

extern int32_t add_query(var_entry_t **vars, rule_literal_t ***lits,
			 samp_table_t *table);

// extern int32_t add_query_instance(samp_literal_t **lits, int32_t num_lits, samp_table_t *table);

extern int32_t add_subst_query_instance(samp_literal_t **litinst, substit_entry_t *substs,
					samp_query_t *query, samp_table_t *table);

extern void all_rule_instances(int32_t rule, samp_table_t *table);

extern void smart_rule_instances_rec(int32_t order, int32_t *ordered_lits, samp_rule_t *rule,
		samp_table_t *table, int32_t atom_index);

extern void all_rule_instances_rec(int32_t vidx, samp_rule_t *rule, 
		samp_table_t *table, int32_t atom_index);

extern int32_t samp_query_to_query_instance(samp_query_t *query,
					    samp_table_t *table);

extern void all_query_instances_rec(int32_t vidx, samp_query_t *query,
				    samp_table_t *table, int32_t atom_index);
  
extern void all_query_instances(samp_query_t *query, samp_table_t *table);

extern double choose();

extern int32_t choose_unfixed_variable(samp_truth_value_t *assignment, int32_t num_vars, int32_t num_unfixed_vars);

extern void cost_flip_unfixed_variable(samp_table_t *table,
				       int32_t *dcost, 
				       int32_t var);

extern int32_t flip_unfixed_variable(samp_table_t *table,
				     int32_t var);

extern int32_t choose_clause_var(samp_table_t *table,
				 samp_clause_t *link,
				 samp_truth_value_t *assignment,
				 double rvar_probability);

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

extern int32_t add_and_activate_atom(samp_table_t *table, samp_atom_t *atom);
extern int32_t activate_atom(samp_table_t *table, int32_t atom_index);
//extern int32_t activate_atom(samp_table_t *table, samp_atom_t *atom);

extern void link_propagate(samp_table_t *table,
		    samp_literal_t lit);

extern int32_t get_fixable_literal(samp_truth_value_t *assignment,
			    samp_literal_t *disjunct,
			    int32_t numlits);

extern int32_t scan_unsat_clauses(samp_table_t *table);

extern int32_t negative_unit_propagate(samp_table_t *table);
  
//extern void init_random_assignment(samp_truth_value_t *assignment, int32_t num_vars,
//				   int32_t *num_unfixed_vars);
extern void init_random_assignment(samp_table_t *table, int32_t *num_unfixed_vars);

extern int32_t init_sample_sat(samp_table_t *table);

/* bool valid_atom_table(atom_table_t *atom_table, */
/* 		      pred_table_t *pred_table, */
/* 		      const_table_t *const_table, */
/* 		      sort_table_t *sort_table); */

/* bool valid_table(samp_table_t *table); */

int32_t reset_sample_sat(samp_table_t *table);

int32_t sample_sat_body(samp_table_t *table, bool lazy, double sa_probability,
			double samp_temperature, double rvar_probability);

void sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips,
		bool draw_sample);

void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
	    double samp_temperature, double rvar_probability,
	    uint32_t max_flips, uint32_t max_extra_flips, uint32_t timeout,
	    uint32_t burn_in_steps, uint32_t samp_interval);

bool eql_samp_atom(samp_atom_t *atom1, samp_atom_t *atom2, samp_table_t *table);

bool eql_rule_literal(rule_literal_t *lit1, rule_literal_t *lit2, samp_table_t *table);

bool eql_query_entries(rule_literal_t ***lits, samp_query_t *query, samp_table_t *table);

bool eql_query_instance_lits(samp_literal_t **lit1, samp_literal_t **lit2);

void retract_source(char *source, samp_table_t *table);

uint32_t genrand_uint(uint32_t n);
  
#endif /* __SAMPLESAT_H */
