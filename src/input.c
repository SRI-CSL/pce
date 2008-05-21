/* Functions for creating input structures */

#include <inttypes.h>

#include "memalloc.h"
#include "utils.h"
#include "input.h"

input_clause_buffer_t input_clause_buffer = {0, 0, NULL};
input_literal_buffer_t input_literal_buffer = {0, 0, NULL};
input_atom_buffer_t input_atom_buffer = {0, 0, NULL};

void input_clause_buffer_resize (){
  if (input_clause_buffer.capacity == 0){
    input_clause_buffer.clauses = (input_clause_t *)
      safe_malloc(INIT_INPUT_CLAUSE_BUFFER_SIZE * sizeof(input_clause_t));
    input_clause_buffer.capacity = INIT_INPUT_CLAUSE_BUFFER_SIZE;
  } else {
    uint32_t size = input_clause_buffer.size;
    uint32_t capacity = input_clause_buffer.capacity;
    if (size+1 >= capacity){
      if (MAX_SIZE(sizeof(input_clause_t), 0) - capacity <= capacity/2){
	out_of_memory();
      }
      capacity += capacity/2;
      printf("Increasing clause buffer to %"PRIu32"\n", capacity);
      input_clause_buffer.clauses = (input_clause_t *)
	safe_realloc(input_clause_buffer.clauses,
		     capacity * sizeof(input_clause_t));
      input_clause_buffer.capacity = capacity;
    }
  }
}

void input_literal_buffer_resize (){
  if (input_literal_buffer.capacity == 0){
    input_literal_buffer.literals = (input_literal_t *)
      safe_malloc(INIT_INPUT_LITERAL_BUFFER_SIZE * sizeof(input_literal_t));
    input_literal_buffer.capacity = INIT_INPUT_LITERAL_BUFFER_SIZE;
  } else {
    uint32_t size = input_literal_buffer.size;
    uint32_t capacity = input_literal_buffer.capacity;
    if (size+1 >= capacity){
      if (MAX_SIZE(sizeof(input_literal_t), 0) - capacity <= capacity/2){
	out_of_memory();
      }
      capacity += capacity/2;
      printf("Increasing literal buffer to %"PRIu32"\n", capacity);
      input_literal_buffer.literals = (input_literal_t *)
	safe_realloc(input_literal_buffer.literals,
		     capacity * sizeof(input_literal_t));
      input_literal_buffer.capacity = capacity;
    }
  }
}


void input_atom_buffer_resize (){
  if (input_atom_buffer.capacity == 0){
    input_atom_buffer.atoms = (input_atom_t *)
      safe_malloc(INIT_INPUT_ATOM_BUFFER_SIZE * sizeof(input_atom_t));
    input_atom_buffer.capacity = INIT_INPUT_ATOM_BUFFER_SIZE;
  } else {
    uint32_t size = input_atom_buffer.size;
    uint32_t capacity = input_atom_buffer.capacity;
    if (size+1 >= capacity){
      if (MAX_SIZE(sizeof(input_atom_t), 0) - capacity <= capacity/2){
	out_of_memory();
      }
      capacity += capacity/2;
      printf("Increasing atom buffer to %"PRIu32"\n", capacity);
      input_atom_buffer.atoms = (input_atom_t *)
	safe_realloc(input_atom_buffer.atoms, capacity * sizeof(input_atom_t));
      input_atom_buffer.capacity = capacity;
    }
  }
}

input_clause_t *new_input_clause () {
  input_clause_buffer_resize();
  return &input_clause_buffer.clauses[input_clause_buffer.size++];
}

input_literal_t *new_input_literal () {
  input_literal_buffer_resize();
  return &input_literal_buffer.literals[input_literal_buffer.size++];
}

input_atom_t *new_input_atom () {
  input_atom_buffer_resize();
  return &input_atom_buffer.atoms[input_atom_buffer.size++];
}
