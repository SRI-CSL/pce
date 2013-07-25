#include <ctype.h>
#include <inttypes.h>
#include <float.h>
#include <string.h>
#include <time.h>

#include "tables.h"
#include "buffer.h"
#include "print.h"
#include "utils.h"

/* Global buffers */
atom_buffer_t samp_atom_buffer = { 0, NULL, 0};
clause_buffer_t clause_buffer = {0, NULL};
substit_buffer_t substit_buffer = {0, NULL};

input_clause_buffer_t input_clause_buffer = { 0, 0, NULL };
input_literal_buffer_t input_literal_buffer = { 0, 0, NULL };
input_atom_buffer_t input_atom_buffer = { 0, 0, NULL };

input_clause_t *new_input_clause() {
	input_clause_buffer_resize();
	return input_clause_buffer.clauses[input_clause_buffer.size++];
}

input_literal_t *new_input_literal() {
	input_literal_buffer_resize();
	return &input_literal_buffer.literals[input_literal_buffer.size++];
}

input_atom_t *new_input_atom() {
	input_atom_buffer_resize();
	return &input_atom_buffer.atoms[input_atom_buffer.size++];
}

/* Allocates enough space for an atom_buffer */
void atom_buffer_resize(atom_buffer_t *atom_buffer, int32_t arity) {
	assert(atom_buffer->lock == 0);
	atom_buffer->lock = 1;

	int32_t size = atom_buffer->size;

	if (size < arity + 1) {
		if (size == 0) {
			size = INIT_ATOM_SIZE;
		} else {
			if (MAXSIZE(sizeof(int32_t), 0) - size <= size / 2) {
				out_of_memory();
			}
			size += size / 2;
		}
		if (size < arity + 1) {
			if (MAXSIZE(sizeof(int32_t), 0) <= arity)
				out_of_memory();
			size = arity + 1;
		}
		atom_buffer->data = (int32_t *) safe_realloc(atom_buffer->data,
				size * sizeof(int32_t));
		atom_buffer->size = size;
	}
}

/* used with atom_buffer_resize to check if there is a conflict */
void atom_buffer_release(atom_buffer_t *atom_buffer) {
	assert(atom_buffer->lock == 1);
	atom_buffer->lock = 0;
}

void clause_buffer_resize (int32_t length){
	if (clause_buffer.data == NULL){
		clause_buffer.data = (int32_t *) safe_malloc(INIT_CLAUSE_SIZE * sizeof(int32_t));
		clause_buffer.size = INIT_CLAUSE_SIZE;
	}
	int32_t size = clause_buffer.size;
	if (size < length){
		if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
			out_of_memory();
		}
		size += size/2;
		clause_buffer.data =
			(int32_t  *) safe_realloc(clause_buffer.data, size * sizeof(int32_t));
		clause_buffer.size = size;
	}
}

void substit_buffer_resize(int32_t length) {
	if (substit_buffer.entries == NULL) {
		substit_buffer.entries = (substit_entry_t *)
			safe_malloc(INIT_SUBSTIT_TABLE_SIZE * sizeof(substit_entry_t));
		substit_buffer.size = INIT_SUBSTIT_TABLE_SIZE;
	}
	int32_t size = substit_buffer.size;
	if (size < length){
		if (MAXSIZE(sizeof(substit_entry_t), 0) - size <= size/2){
			out_of_memory();
		}
		size += size/2;
		substit_buffer.entries = (substit_entry_t *)
			safe_realloc(substit_buffer.entries, size * sizeof(substit_entry_t));
		substit_buffer.size = size;
	}
}

void input_clause_buffer_resize() {
	if (input_clause_buffer.capacity == 0) {
		input_clause_buffer.clauses = (input_clause_t **) safe_malloc(
				INIT_INPUT_CLAUSE_BUFFER_SIZE * sizeof(input_clause_t *));
		input_clause_buffer.capacity = INIT_INPUT_CLAUSE_BUFFER_SIZE;
	} else {
		uint32_t size = input_clause_buffer.size;
		uint32_t capacity = input_clause_buffer.capacity;
		if (size + 1 >= capacity) {
			if (MAX_SIZE(sizeof(input_clause_t), 0) - capacity
					<= capacity / 2) {
				out_of_memory();
			}
			capacity += capacity / 2;
			cprintf(2, "Increasing clause buffer to %"PRIu32"\n", capacity);
			input_clause_buffer.clauses = (input_clause_t **) safe_realloc(
					input_clause_buffer.clauses,
					capacity * sizeof(input_clause_t *));
			input_clause_buffer.capacity = capacity;
		}
	}
}

void input_literal_buffer_resize() {
	if (input_literal_buffer.capacity == 0) {
		input_literal_buffer.literals = (input_literal_t *) safe_malloc(
				INIT_INPUT_LITERAL_BUFFER_SIZE * sizeof(input_literal_t));
		input_literal_buffer.capacity = INIT_INPUT_LITERAL_BUFFER_SIZE;
	} else {
		uint32_t size = input_literal_buffer.size;
		uint32_t capacity = input_literal_buffer.capacity;
		if (size + 1 >= capacity) {
			if (MAX_SIZE(sizeof(input_literal_t), 0) - capacity
					<= capacity / 2) {
				out_of_memory();
			}
			capacity += capacity / 2;
			cprintf(2, "Increasing literal buffer to %"PRIu32"\n", capacity);
			input_literal_buffer.literals = (input_literal_t *) safe_realloc(
					input_literal_buffer.literals,
					capacity * sizeof(input_literal_t));
			input_literal_buffer.capacity = capacity;
		}
	}
}

void input_atom_buffer_resize() {
	if (input_atom_buffer.capacity == 0) {
		input_atom_buffer.atoms = (input_atom_t *) safe_malloc(
				INIT_INPUT_ATOM_BUFFER_SIZE * sizeof(input_atom_t));
		input_atom_buffer.capacity = INIT_INPUT_ATOM_BUFFER_SIZE;
	} else {
		uint32_t size = input_atom_buffer.size;
		uint32_t capacity = input_atom_buffer.capacity;
		if (size + 1 >= capacity) {
			if (MAX_SIZE(sizeof(input_atom_t), 0) - capacity <= capacity / 2) {
				out_of_memory();
			}
			capacity += capacity / 2;
			cprintf(2, "Increasing atom buffer to %"PRIu32"\n", capacity);
			input_atom_buffer.atoms = (input_atom_t *) safe_realloc(
					input_atom_buffer.atoms, capacity * sizeof(input_atom_t));
			input_atom_buffer.capacity = capacity;
		}
	}
}

/* get a copy of the buffer */
char *get_string_from_buffer (string_buffer_t *strbuf) {
	char *new_str;
	if (strbuf->size == 0) {
		return "";
	} else {
		new_str = (char *) safe_malloc((strbuf->size+1) * sizeof(char));
		strcpy(new_str, strbuf->string);
		strbuf->size = 0;
		return new_str;
	}
}

void string_buffer_resize (string_buffer_t *strbuf, int32_t delta) {
	if (strbuf->capacity == 0) {
		strbuf->string = (char *)
			safe_malloc(INIT_STRING_BUFFER_SIZE * sizeof(char));
		strbuf->capacity = INIT_STRING_BUFFER_SIZE;
	}
	uint32_t size = strbuf->size;
	uint32_t capacity = strbuf->capacity;
	if (size+delta >= capacity){
		if (MAX_SIZE(sizeof(char), 0) - capacity <= delta){
			out_of_memory();
		}
		capacity += delta;
		strbuf->string = (char *)
			safe_realloc(strbuf->string, capacity * sizeof(char));
		strbuf->capacity = capacity;
	}
}

