#ifndef __CLAUSE_LIST
#define __CLAUSE_LIST 1

#include "tables.h"

/* Validates a clause list */
bool valid_clause_list(samp_clause_list_t *list);

/* Moves a ptr forward */
static inline samp_clause_t *next_clause(samp_clause_t *ptr) {
	return ptr->link;
}

/* Returns the current clause, the list keeps unchanged */
static inline samp_clause_t *get_clause(samp_clause_t *ptr) {
	return ptr->link;
}

/* Initialize a clause list */
extern void init_clause_list(samp_clause_list_t *list);
extern void empty_clause_list(samp_clause_list_t *list);

/* Returns true if the clause list is empty */
static inline bool is_empty_clause_list(samp_clause_list_t *list) {
	return list->length == 0;
}

/* Inserts a clause after a ptr */
extern void clause_list_insert(samp_clause_t *clause, samp_clause_list_t *list,
		samp_clause_t *ptr);

/* Inserts a clause before a ptr */
static inline void clause_list_push(samp_clause_t *clause, samp_clause_list_t *list,
		samp_clause_t *ptr) {
	clause_list_insert(clause, list, ptr);
	ptr = next_clause(ptr);
}

/* Inserts a clause at the head of a list */
static inline void clause_list_insert_head(samp_clause_t *clause, samp_clause_list_t *list) {
	clause_list_insert(clause, list, list->head);
}

/* Inserts a clause at the tail of a list */
static inline void clause_list_push_back(samp_clause_t *clause, samp_clause_list_t *list) {
	clause_list_insert(clause, list, list->tail);
}

/* Removes the current clause from the list and return it */
extern samp_clause_t *clause_list_pop(samp_clause_list_t *list, samp_clause_t *ptr);

/* Removes and returns the first clause in the list */
static inline samp_clause_t *clause_list_pop_head(samp_clause_list_t *list) {
	return clause_list_pop(list, list->head);
}

/* Concatinate two clause lists together */
extern void clause_list_concat(samp_clause_list_t *list_src, samp_clause_list_t *list_dst);

/* Move all unsat clauses to the live clause list */
extern void move_unsat_to_live_clauses(rule_inst_table_t *rule_inst_table);

/* Move all sat clauses to the live clause list */
extern void move_sat_to_live_clauses(rule_inst_table_t *rule_inst_table);

/* Return a random clause from a clause list */
extern samp_clause_t *choose_random_clause(samp_clause_list_t *list);

#endif /* __CLAUSE_LIST */

