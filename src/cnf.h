#ifndef __CNF_H
#define __CNF_H 1

#include "vectors.h"
#include "input.h"

extern pvector_t ask_buffer;

/*
 * Generates the CNF form of a formula, and updates the clause table (if no
 * variables) or the rule table.
 */
extern void add_cnf(char **frozen, input_formula_t *formula, double weight, 
		char *source, bool add_weights);

/* 
 * Generates the CNF form of a query formula, and updates the query and
 * query_instance tables. These are analogous to the rule and rule_inst tables,
 * respectively. Thus queries without variables are immediately added to the
 * query_instance table, while queries with variables are added to the query
 * table, and instances of them are generated as needed and added to the
 * query_instance table.
 *
 * Result is a sorted list of samp_query_instance_t's in the ask_buffer.
 */
extern void ask_cnf(input_formula_t *formula, double threshold, int32_t numresults);

/* 
 * Similar to ask_cnf, but the role of it is to add queries one by one to allow
 * sampling of them at the same time by a later call to mc_sat()
 */
extern void add_cnf_query(input_formula_t *formula);

#endif /* __CNF_H */     
