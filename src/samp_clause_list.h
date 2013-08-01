#ifndef __SAMP_CLAUSE_LIST
#define __SAMP_CLAUSE_LIST 1

#include "tables.h"

/* Pushes a clause to some clause linked list */
static inline void push_clause(samp_clause_t *clause, samp_clause_t **list) {
	clause->link = *list;
	*list = clause;
}

/* Pushes a clause to the negative unit clause linked list */
static inline void push_negative_or_unit_clause(clause_table_t *clause_table, uint32_t i) {
	push_clause(clause_table->samp_clauses[i], &clause_table->negative_or_unit_clauses);
}

/* Pushes a clause to the unsat clause linked list */
static inline void push_unsat_clause(clause_table_t *clause_table, uint32_t i) {
	push_clause(clause_table->samp_clauses[i], &clause_table->unsat_clauses);
	clause_table->num_unsat_clauses++;
}

static inline void push_sat_clause(clause_table_t *clause_table, uint32_t i) {
	push_clause(clause_table->samp_clauses[i], &clause_table->sat_clauses);
}

/* TODO A very slow function to calculate the length of a linked list */
static int32_t length_clause_list(samp_clause_t *link) {
	int32_t length = 0;
	while (link != NULL) {
		link = link->link;
		length++;
	}
	return length;
}

static void move_sat_to_unsat_clauses(clause_table_t *clause_table) {
	int32_t length = length_clause_list(clause_table->sat_clauses);
	samp_clause_t **link_ptr = &(clause_table->unsat_clauses);
	samp_clause_t *link = clause_table->unsat_clauses;
	while (link != NULL) {
		link_ptr = &(link->link);
		link = link->link;
	}
	*link_ptr = clause_table->sat_clauses;
	clause_table->sat_clauses = NULL;
	clause_table->num_unsat_clauses += length;
}

/* FIXME: This function is not used?? */
//void move_unsat_to_dead_clauses(clause_table_t *clause_table) {
//	samp_clause_t **link_ptr = &(clause_table->dead_clauses);
//	samp_clause_t *link = clause_table->dead_clauses;
//	while (link != NULL) {
//		link_ptr = &(link->link);
//		link = link->link;
//	}
//	*link_ptr = clause_table->unsat_clauses;
//	clause_table->unsat_clauses = NULL;
//	clause_table->num_unsat_clauses = 0;
//}

#endif /* __SAMP_CLAUSE_LIST */
