#ifndef __GROUND_H
#define __GROUND_H

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

extern bool eql_query_entries(rule_literal_t ***lits, samp_query_t *query,
		samp_table_t *table);

extern void all_pred_instances(char *pred, samp_table_t *table);

extern void all_rule_instances(int32_t rule, samp_table_t *table);

extern void smart_all_rule_instances(int32_t rule_index, samp_table_t *table);

extern void all_query_instances(samp_query_t *query, samp_table_t *table);

extern void fixed_const_query_instances(samp_query_t *query, samp_table_t *table,
		int32_t atom_index);

extern void create_new_const_atoms(int32_t cidx, int32_t csort, samp_table_t *table);

extern void create_new_const_rule_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index);

extern void create_new_const_query_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index);

extern samp_atom_t *rule_atom_to_samp_atom(samp_atom_t *satom, rule_atom_t *ratom, 
		int32_t arity, substit_entry_t *substs);

extern int32_t activate_atom(samp_table_t *table, int32_t atom_index);
extern void activate_rules(int32_t atom_index, samp_table_t *table);

extern int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p);
extern int32_t add_internal_rule_instance(samp_table_t *table, rule_inst_t *entry,
		bool indirect, bool add_weights);

#endif /* __GROUND_H */

