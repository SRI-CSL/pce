#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "memalloc.h"
#include "samplesat.h"


/*
  The test case must include some unit clauses, and positive and negative weight clauses.
  There should be a few direct assertions.  This should lead to nontrivial unit propagation.
  The functions to be tested include link_propagate, scan_unsat_clauses, and
  negative_unit_propagate.

  The first set of experiments involve 1 sort S, 5 constants a, b, c, d, e,
  1 direct predicate P, one indirect predicate Q.  We assert Pa, Pc, Pe,
  add clauses of the form Px -> Qx, and weighted clauses of the form
  -Qa or -Qb and -Qc or -Qd.   We place all the clauses in the unsat part.
  Start with the assignment where the P's have the closed wrld assignment,
  and where Qa, Qc, Qe are false, and Qb, Qc are true.  
 */

int main(){
  samp_table_t table;
  init_samp_table(&table);
  sort_table_t *sort_table = &(table.sort_table);
  const_table_t *const_table = &(table.const_table);
  pred_table_t *pred_table = &(table.pred_table);
  atom_table_t *atom_table = &(table.atom_table);
  clause_table_t *clause_table = &(table.clause_table);

  add_sort(sort_table, "S");
  add_const(const_table, "a", sort_table, "S");
  add_const(const_table, "b", sort_table, "S");
  add_const(const_table, "c", sort_table, "S");
  add_const(const_table, "d", sort_table, "S");
  add_const(const_table, "e", sort_table, "S");

  int32_t i, j;

  for (i=0; i < sort_table->num_sorts; i++){
    printf("\nsort = %s", sort_table->entries[i].name);
    for (j = 0; j < sort_table->entries[i].cardinality; j++){
      printf("\nconst = %s", const_table->entries[sort_table->entries[i].constants[j]].name);
    }
  }
    char **strbuf;
    strbuf = (char **) safe_malloc(sizeof(char *));
    strbuf[0] = "S";
    add_pred(pred_table, "P", 1, 1, sort_table, strbuf);
    add_pred(pred_table, "Q", 0, 1, sort_table, strbuf);
    
    print_predicates(pred_table, sort_table);

    input_atom_t *atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t) + sizeof(int32_t));
    atom->pred = "P";
    atom->args[0] = "a";
    assert_atom(&table, atom);
    atom->args[0] = "b";
    add_atom(&table, atom);
    if (!valid_atom_table(atom_table, pred_table, const_table, sort_table))
      printf("Invalid atom table\n");
    atom->args[0] = "c";
    assert_atom(&table, atom);
    if (!valid_atom_table(atom_table, pred_table, const_table, sort_table))
      printf("Invalid atom table\n");
    atom->args[0] = "d";
    add_atom(&table, atom);
    atom->args[0] = "e";
    assert_atom(&table, atom);
    if (!valid_atom_table(atom_table, pred_table, const_table, sort_table))
      printf("Invalid atom table\n");
    atom->pred = "Q";
    atom->args[0] = "a";
    add_atom(&table, atom);
    if (!valid_atom_table(atom_table, pred_table, const_table, sort_table))
      printf("Invalid atom table\n");
    atom->args[0] = "b";
    add_atom(&table, atom);
    atom->args[0] = "c";
    add_atom(&table, atom);
    if (!valid_atom_table(atom_table, pred_table, const_table, sort_table))
      printf("Invalid atom table\n");
    atom->args[0] = "d";
    add_atom(&table, atom);
    atom->args[0] = "e";
    add_atom(&table, atom);
    if (!valid_atom_table(atom_table, pred_table, const_table, sort_table))
      printf("Invalid atom table\n");

    print_atoms(atom_table, pred_table, const_table);

    input_literal_t *in_clause[3];
    input_literal_t *lit1 = (input_literal_t *) safe_malloc(sizeof(input_literal_t) + sizeof(char *));
    input_literal_t *lit2 = (input_literal_t *) safe_malloc(sizeof(input_literal_t) + sizeof(char *));
    lit1->neg = 1;
    lit1->atom.pred = "P";
    lit1->atom.args[0] = "a";
    lit2->neg = 0;
    lit2->atom.pred = "Q";
    lit2->atom.args[0] = "a";
    in_clause[0] = lit1;
    in_clause[1] = lit2;
    in_clause[2] = NULL;
    add_clause(&table, in_clause, 3);
    lit1->atom.args[0] = "b";
    lit2->atom.args[0] = "b";
    add_clause(&table, in_clause, 3);
    lit1->atom.args[0] = "c";
    lit2->atom.args[0] = "c";
    add_clause(&table, in_clause, 3);
    lit1->atom.args[0] = "d";
    lit2->atom.args[0] = "d";
    add_clause(&table, in_clause, 3);
    lit1->atom.args[0] = "e";
    lit2->atom.args[0] = "e";
    add_clause(&table, in_clause, 3);
    print_clauses(clause_table);
    if (!valid_table(&table))
      printf("Invalid table\n");
    /*    init_sample_sat(&table);
    print_atoms(atom_table, pred_table, const_table);
    print_clause_table(clause_table, atom_table->num_vars);
    if (!valid_table(&table)){
      printf("Invalid table\n");
    }
    uint32_t max_flips = 10;
    while (max_flips > 0 && clause_table->num_unsat_clauses > 0 && valid_table(&table)){
      sample_sat_body(&table, 0.1, 20.0, .1);
      max_flips--;
    }
    max_flips = 10;
    print_atoms(atom_table, pred_table, const_table);
    print_clause_table(clause_table, atom_table->num_vars);
    reset_sample_sat(&table);
    while (max_flips > 0 && clause_table->num_unsat_clauses > 0 && valid_table(&table)){
      sample_sat_body(&table, 0.1, 20.0, .1);
      max_flips--;
    }
    print_atoms(atom_table, pred_table, const_table);
    print_clause_table(clause_table, atom_table->num_vars);
    */

    mc_sat(&table, 0.1, 20.0, 0.1, 20, 30);

    return 0;
}
