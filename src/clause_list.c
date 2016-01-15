#include "tables.h"
#include "clause_list.h"
#include "utils.h"
#include "memalloc.h"

bool valid_clause_list(samp_clause_list_t *list) {
	assert(list->tail->link == NULL);
	samp_clause_t *ptr;
	int32_t length = 0;
	for (ptr = list->head; ptr != list->tail; ptr = next_clause_ptr(ptr)) {
		length++;
	}
	assert(length == list->length);
	return true;
}

void init_clause_list(samp_clause_list_t *list) {
	/* sentinel */
	list->head = (samp_clause_t *) safe_malloc(sizeof(samp_clause_t));
	list->head->link = NULL;
	list->tail = list->head;
	list->length = 0;
	assert(valid_clause_list(list));
}

#if 0
void copy_clause_list(samp_clause_list_t, *to, samp_clause_list_t *from) {
	/* sentinel */
  to->length = from->length;

	list->head = (samp_clause_t *) safe_malloc(sizeof(samp_clause_t));
	list->head->link = NULL;
	list->tail = list->head;
	list->length = 0;
	assert(valid_clause_list(list));
}
#endif


void empty_clause_list(samp_clause_list_t *list) {
	list->head->link = NULL;
	list->tail = list->head;
	list->length = 0;
}

void clause_list_insert(samp_clause_t *clause, samp_clause_list_t *list,
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

samp_clause_t *clause_list_pop(samp_clause_list_t *list, samp_clause_t *ptr) {
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

void clause_list_concat(samp_clause_list_t *list_src, samp_clause_list_t *list_dst) {
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

samp_clause_t *choose_random_clause(samp_clause_list_t *list) {
	//int32_t clsidx = random_uint(list->length);
	int32_t clsidx = genrand_uint(list->length);
	samp_clause_t *ptr = list->head;
	for (; clsidx > 0; clsidx--) {
		ptr = next_clause_ptr(ptr);
	}
	return ptr->link;
}

