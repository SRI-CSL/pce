#ifndef __GROUND_H
#define __GROUND_H

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

extern bool eql_query_entries(rule_literal_t ***lits, samp_query_t *query,
		samp_table_t *table);

extern void all_pred_instances(char *pred, samp_table_t *table);

extern void all_rule_instances(int32_t rule, samp_table_t *table);

extern void fixed_const_rule_instances(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index);

extern void smart_rule_instances(int32_t rule_index, samp_table_t *table);

extern void all_query_instances(samp_query_t *query, samp_table_t *table);

extern void fixed_const_query_instances(samp_query_t *query, samp_table_t *table,
		int32_t atom_index);

extern void create_new_const_atoms(int32_t cidx, int32_t csort, samp_table_t *table);

extern void create_new_const_rule_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index);

extern void create_new_const_query_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index);

extern int32_t activate_atom(samp_table_t *table, int32_t atom_index);
extern void activate_rules(int32_t atom_index, samp_table_t *table);

extern int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p);
extern int32_t add_atom(samp_table_t *table, input_atom_t *current_atom);

extern int32_t add_internal_clause(samp_table_t *table, int32_t *clause,
		int32_t length, int32_t *fixed_preds, double weight,
		bool indirect, bool add_weights);
extern int32_t add_clause(samp_table_t *table, input_literal_t **in_clause,
		double weight, char *source, bool add_weights);

#endif /* __GROUND_H */

