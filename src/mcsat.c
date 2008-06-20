/* The main for mcsat - the read_eval_print_loop is in input.c */

#include "samplesat.h"
#include "tables.h"
#include "input.h"

int main() {
  samp_table_t table;
  
  init_samp_table(&table);
  read_eval_print_loop(stdin, &table);
  printf("Exiting MCSAT\n");
  return 0;
}
