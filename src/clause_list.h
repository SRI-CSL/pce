#ifndef __CLAUSE_LIST
#define __CLAUSE_LIST 1

#include "tables.h"

/* Initialize a clause list */
static inline void init_clause_list(samp_clause_list_t *list) {
	/* sentinel */
	list->head = (samp_clause_t *) safe_malloc(sizeof(samp_clause_t));
	list->head->link = NULL;
	list->tail = list->head;
	list->length = 0;
}

/* Move the ptr forward */
static inline samp_clause_t *next_clause(samp_clause_t *ptr) {
	return ptr->link;
}

/* validates a clause list */
static inline bool valid_clause_list(samp_clause_list_t *list) {
	samp_clause_t *ptr;
	int32_t length = 0;
	for (ptr = list->head; ptr != list->tail; ptr = next_clause(ptr)) {
		length++;
	}
	assert(length == list->length);
	assert(list->tail->link == NULL);
	return true;
}

/* Return if the clause list is empty */
static inline bool is_empty_clause_list(samp_clause_list_t *list) {
	assert(valid_clause_list(list));
	return list->length == 0;
}

/* Inserts a clause after the ptr */
static inline void insert_clause(samp_clause_t *clause, samp_clause_list_t *list,
		samp_clause_t *ptr) {
	assert(valid_clause_list(list));
	clause->link = ptr->link;
	ptr->link = clause;
	if (list->tail == ptr) { /* insert at the end */
		list->tail = clause;
	}
	list->length++;
	assert(valid_clause_list(list));
}

/* Insert a clause before the ptr */
static inline void push_clause(samp_clause_t *clause, samp_clause_list_t *list,
		samp_clause_t *ptr) {
	insert_clause(clause, list, ptr);
	ptr = next_clause(ptr);
}

/* Insert a clause at the head of a list */
static inline void insert_head_clause(samp_clause_t *clause, samp_clause_list_t *list) {
	insert_clause(clause, list, list->head);
}

/* Returns the current clause, the list keeps unchanged */
static inline samp_clause_t *get_clause(samp_clause_list_t *list, samp_clause_t *ptr) {
	return ptr->link;
}

/* Remove the current clause from the list and return it */
static inline samp_clause_t *pop_clause(samp_clause_list_t *list, samp_clause_t *ptr) {
	assert(valid_clause_list(list));
	assert(ptr != list->tail); /* pointer cannot be the tail */
	samp_clause_t *clause = ptr->link;
	if (list->tail == clause) {
		list->tail = ptr;
	}
	ptr->link = clause->link;
	clause->link = NULL;
	list->length--;
	assert(valid_clause_list(list));
	return clause;
}

/* Concatinate two clause lists together */
static inline void append_clause_list(samp_clause_list_t *list_src, samp_clause_list_t *list_dst) {
	assert(valid_clause_list(list_src));
	assert(valid_clause_list(list_dst));
	list_dst->tail->link = list_src->head->link;
	if (!is_empty_clause_list(list_src)) {
		list_dst->tail = list_src->tail;
	}
	list_dst->length += list_src->length;
	list_src->head->link = NULL;
	list_src->tail = list_src->head;
	list_src->length = 0;
	assert(valid_clause_list(list_src));
	assert(valid_clause_list(list_dst));
}

/* Move all unsat clauses to the live clause list */
static inline void move_unsat_to_live_clauses(clause_table_t *clause_table) {
	append_clause_list(&clause_table->unsat_clauses, &clause_table->live_clauses);
}

/* Move all sat clauses to the live clause list */
static inline void move_sat_to_live_clauses(clause_table_t *clause_table) {
	append_clause_list(&clause_table->sat_clauses, &clause_table->live_clauses);
}

/* Return a random clause from a clause list */
static inline samp_clause_t *choose_random_clause(samp_clause_list_t *list) {
	//int32_t clsidx = random_uint(list->length);
	int32_t clsidx;
	samp_clause_t *ptr = list->head;
	for (clsidx = genrand_uint(list->length); clsidx > 0; clsidx--) {
		ptr = next_clause(ptr);
	}
	return ptr->link;
}

#endif /* __CLAUSE_LIST */

