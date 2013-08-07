#ifndef __CLAUSE_LIST
#define __CLAUSE_LIST 1

#include "tables.h"

/* validates a clause list */
bool valid_clause_list(samp_clause_list_t *list);

/* Move the ptr forward */
static inline samp_clause_t *next_clause(samp_clause_t *ptr) {
	return ptr->link;
}

/* Returns the current clause, the list keeps unchanged */
static inline samp_clause_t *get_clause(samp_clause_t *ptr) {
	return ptr->link;
}

/* Initialize a clause list */
extern void init_clause_list(samp_clause_list_t *list);

/* Return if the clause list is empty */
static inline bool is_empty_clause_list(samp_clause_list_t *list) {
	return list->length == 0;
}

/* Inserts a clause after the ptr */
extern void insert_clause(samp_clause_t *clause, samp_clause_list_t *list,
		samp_clause_t *ptr);

/* Insert a clause before the ptr */
extern void push_clause(samp_clause_t *clause, samp_clause_list_t *list,
		samp_clause_t *ptr);

/* Insert a clause at the head of a list */
extern void insert_head_clause(samp_clause_t *clause, samp_clause_list_t *list);

/* Remove the current clause from the list and return it */
extern samp_clause_t *pop_clause(samp_clause_list_t *list, samp_clause_t *ptr);

/* Concatinate two clause lists together */
extern void append_clause_list(samp_clause_list_t *list_src, samp_clause_list_t *list_dst);

/* Move all unsat clauses to the live clause list */
extern void move_unsat_to_live_clauses(clause_table_t *clause_table);

/* Move all sat clauses to the live clause list */
extern void move_sat_to_live_clauses(clause_table_t *clause_table);

/* Return a random clause from a clause list */
extern samp_clause_t *choose_random_clause(samp_clause_list_t *list);

#endif /* __CLAUSE_LIST */

