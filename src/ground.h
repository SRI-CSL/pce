#ifndef __GROUND_H
#define __GROUND_H

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

/* Instantiate all instances for a predicate */
extern void all_pred_instances(char *pred, samp_table_t *table);

/* Instantiate all instances for a rule */
extern void all_rule_instances(int32_t rule, samp_table_t *table);

/* Lazily instantiate (only the unsat) instances for a rule */
extern void smart_all_rule_instances(int32_t rule_index, samp_table_t *table);

/* Instantiate all instances for a query */
extern void all_query_instances(samp_query_t *query, samp_table_t *table);

/* Instantiate query instances in which some quantified variables are fixed */
extern void fixed_const_query_instances(samp_query_t *query, samp_table_t *table,
		int32_t atom_index);

/* When new constants are introduced, may need to add new atoms */
extern void create_new_const_atoms(int32_t cidx, int32_t csort, samp_table_t *table);

/* Generates all new instances of rules involving this constant. */
extern void create_new_const_rule_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index);

/* Instantiate new query instances when new constants are introduced. */
extern void create_new_const_query_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index);

/* Converts a rule_atom into a samp_atom */
extern samp_atom_t *rule_atom_to_samp_atom(samp_atom_t *satom, rule_atom_t *ratom, 
		int32_t arity, substit_entry_t *substs);

/* 
 * In lazy inference, an atom has the default truth value at the beginning. When
 * the value is set to non-default during the inference, the atom is activated
 * and put into memory, and all the associated rule instances are also activated.
 */
extern int32_t activate_atom(samp_table_t *table, int32_t atom_index);

/* Lazily activates the rules associated with the specified atom */
extern void activate_rules(int32_t atom_index, samp_table_t *table);

/* Insert a ground atom to the atom_table, used by both lazy and eager inference */
extern int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p);

/*
 * An internal operation used to add a rule instance. These rule instances are
 * already simplified so that they do not contain any ground evidence
 * literals.
 */
extern int32_t add_internal_rule_instance(samp_table_t *table, rule_inst_t *entry,
		bool indirect, bool add_weights);

#endif /* __GROUND_H */

