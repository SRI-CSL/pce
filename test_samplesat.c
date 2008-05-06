#include "samplesat.h"
#include "memalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

uint32_t num_sorts = 100;
uint32_t num_consts = 1000;
uint32_t buf_len = 11;

#define ENABLE 0

typedef struct inclause_buffer_t {
  int32_t size;
  input_literal_t ** data;
} inclause_buffer_t;


static inclause_buffer_t inclause_buffer = {0, NULL};

int main(){
  samp_table_t table;
  init_samp_table(&table);
  sort_table_t *sort_table = &(table.sort_table);
  const_table_t *const_table = &(table.const_table);
  pred_table_t *pred_table = &(table.pred_table);
  atom_table_t *atom_table = &(table.atom_table);
  clause_table_t *clause_table = &(table.clause_table);
  uint32_t i, j;
  char sort_num[6];
  char const_num[6];
  char pred_num[6];
  char *buffer;
  for (i = 0; i < num_sorts; i++){
    sprintf(sort_num, "%d", i);
    buffer = (char *) safe_malloc(sizeof(char)*buf_len);
    memcpy(buffer, "sort", buf_len);
    strcat(buffer, sort_num);
    add_sort(sort_table, buffer);
  }
  if (ENABLE){
  printf("\nsort_table: size: %"PRId32", num_sorts: %"PRId32"",
	 sort_table->size,
	 sort_table->num_sorts);
  for (i = 0; i < num_sorts; i++){
    buffer = sort_table->entries[i].name;
    printf("\nsort[%"PRIu32"]: %s; symbol_table: %"PRId32"",
	   i, buffer, stbl_find(&(sort_table->sort_name_index), buffer));
    add_sort(sort_table, buffer);//should do nothing
    printf("\nRe:sort[%"PRIu32"]: %s; symbol_table: %"PRId32"",
	   i, buffer, stbl_find(&(sort_table->sort_name_index), buffer));
  }
  }
  /* Now testing constants
   */
  for (i = 0; i < num_sorts; i++){
    for (j = 0; j < (num_consts/num_sorts); j++){
      sprintf(const_num, "%d", i*(num_consts/num_sorts) + j);
      memcpy(buffer, "const", 6);
      strcat(buffer, const_num);
      add_const(const_table, buffer, sort_table, sort_table->entries[i].name);
    }
  }
  if (ENABLE){
  for (i=0; i < num_sorts; i++){
    printf("\nsort = %s", sort_table->entries[i].name);
    for (j = 0; j < sort_table->entries[i].cardinality; j++){
      printf("\nconst = %s", const_table->entries[sort_table->entries[i].constants[j]].name);
    }
  }
  }
  /* Now testing signatures
   */

  bool evidence = 0;
  char **sort_name_buffer = (char **) safe_malloc(num_sorts * sizeof(char *));
  printf("\nAdding normal preds");
  for (i = 0; i < num_sorts; i++){
    sort_name_buffer[i] = sort_table->entries[i].name;
    memcpy(buffer, "pred", 5);
    sprintf(pred_num, "%d", i);
    strcat(buffer, pred_num);
    add_pred(pred_table, buffer, evidence, i, sort_table,
	     sort_name_buffer);
  }
  evidence = !evidence;
  printf("\nAdding evidence preds");
  for (j = 0; j < num_sorts; j++){
    sort_name_buffer[j] = sort_table->entries[j].name;
    memcpy(buffer, "pred", 5);
    sprintf(pred_num, "%d", j+i);
    strcat(buffer, pred_num);
    add_pred(pred_table, buffer, evidence, j, sort_table, sort_name_buffer);
  }

  if (ENABLE){
  for (i = 0; i < pred_table->evpred_tbl.num_preds; i++){
    printf("\n evpred[%d] = ", i);
    buffer = pred_table->evpred_tbl.entries[i].name;
    printf("index(%d); ", pred_index(buffer, pred_table));
    printf("%s(", buffer);
    for (j = 0; j < pred_table->evpred_tbl.entries[i].arity; j++){
      printf(" %s, ", sort_table->entries[pred_table->evpred_tbl.entries[i].signature[j]].name);
    }
    printf(")");
  }
  for (i = 0; i < pred_table->pred_tbl.num_preds; i++){
    printf("\n pred[%d] = ", i);
    buffer = pred_table->pred_tbl.entries[i].name;
    printf("index(%d); ", pred_index(buffer, pred_table));
    printf("%s(", buffer);
    for (j = 0; j < pred_table->pred_tbl.entries[i].arity; j++){
      printf(" %s, ", sort_table->entries[pred_table->pred_tbl.entries[i].signature[j]].name);
    }
    printf(")");
  }
  }
  /* Now adding evidence atoms
   */
  input_atom_t * atom_buffer = (input_atom_t *) safe_malloc(101 * sizeof(char *));
  int32_t index;
  for (i = 0; i < pred_table->evpred_tbl.num_preds; i++){
    atom_buffer->pred = pred_table->evpred_tbl.entries[i].name;
    printf("\nBuilding atom[%d]: %s(", i, atom_buffer->pred);
    for(j = 0; j < pred_table->evpred_tbl.entries[i].arity; j++){
      index = sort_table->entries[pred_table->evpred_tbl.entries[i].signature[j]].constants[0];
      buffer = const_table->entries[index].name;
      printf("[%d]%s,", index, buffer);
      atom_buffer->args[j] = buffer;
    }
    printf(")");
    add_atom(&table, atom_buffer);
  }
  if (ENABLE){
  for (i = 0; i < atom_table->num_vars; i++){
    printf("\natom[%d] = %d(", i, atom_table->atom[i]->pred);
    for (j = 0; j < pred_table->evpred_tbl.entries[-atom_table->atom[i]->pred].arity; j++){
      printf("%d,", atom_table->atom[i]->args[j]);
    }
    printf(")");
    printf("\nassignment[%d] = %d", i, atom_table->assignment[i]);
  }
  for (i = 0; i < pred_table->evpred_tbl.num_preds; i++){
    printf("\n");
    for (j = 0; j < pred_table->evpred_tbl.entries[i].num_atoms; j++){
      printf("%d", pred_table->evpred_tbl.entries[i].atoms[j]);
    }
  }
  }

  /* Now adding atoms
   */
  int32_t numvars = atom_table->num_vars;
  for (i = 0; i < pred_table->pred_tbl.num_preds; i++){
    atom_buffer->pred = pred_table->pred_tbl.entries[i].name;
    printf("\nBuilding atom[%d]: %s(", i, atom_buffer->pred);
    for(j = 0; j < pred_table->pred_tbl.entries[i].arity; j++){
      index = sort_table->entries[pred_table->pred_tbl.entries[i].signature[j]].constants[0];
      buffer = const_table->entries[index].name;
      printf("[%d]%s,", index, buffer);
      atom_buffer->args[j] = buffer;
    }
    printf(")");
    add_atom(&table, atom_buffer);
  }
  if (ENABLE){
  for (i = numvars; i < atom_table->num_vars; i++){
    printf("\natom[%d] = %d(", i, atom_table->atom[i]->pred);
    for (j = 0; j < pred_table->pred_tbl.entries[atom_table->atom[i]->pred].arity; j++){
      printf("%d,", atom_table->atom[i]->args[j]);
    }
    printf(")");
    printf("\nassignment[%d] = %d", i, atom_table->assignment[i]);
  }
  for (i = 0; i < pred_table->pred_tbl.num_preds; i++){
    printf("\n");
    for (j = 0; j < pred_table->pred_tbl.entries[i].num_atoms; j++){
      printf("%d", pred_table->pred_tbl.entries[i].atoms[j]);
    }
  }
  }


  /*
   * Adding clauses
   */
  double weight = 0;

  j = pred_table->pred_tbl.num_preds + 1;
  inclause_buffer.data = (input_literal_t **) safe_malloc(j * sizeof(input_literal_t *));
  inclause_buffer.size = j;
  for (i = 0; i < j; i++){
    inclause_buffer.data[i] = NULL;
  }

  bool negate = false;

  for (i = 0; i < pred_table->pred_tbl.num_preds-1; i++){
    inclause_buffer.data[i] = (input_literal_t *) 
      safe_malloc(sizeof(input_literal_t) + i*sizeof(char *));
    /// inclause_buffer.data[i]->neg = !inclause_buffer.data[i]->neg; // ??? 
    inclause_buffer.data[i]->neg = negate;
    negate = !negate;
    inclause_buffer.data[i]->atom.pred = pred_table->pred_tbl.entries[i+1].name;
    for (j = 0; j < i; j++){
      index = sort_table->entries[pred_table->pred_tbl.entries[i+1].signature[j]].constants[0];
      inclause_buffer.data[i]->atom.args[j] = const_table->entries[index].name;
    }
    weight += .25;
    add_clause(&table, inclause_buffer.data, weight);
  }
  print_clauses(clause_table);
  print_atoms(atom_table, pred_table, const_table);
  
  
  return 1;
}
    
