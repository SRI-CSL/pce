#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "memalloc.h"
#include "prng.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "gcd.h"
#include "utils.h"
#include "samplesat.h"
#include "print.h"
#include "vectors.h"
#include "samplesat.h"
#ifdef MINGW

/*
 * Need some version of random()
 * rand() exists on mingw but random() does not
 */
static inline int random(void) {
  return rand();
}

#endif

int32_t pred_cardinality(pred_tbl_t *pred_tbl,
			 sort_table_t *sort_table,
			 int32_t predicate){
  if (predicate < 0 || predicate >= pred_tbl->num_preds){
    return -1;
  }
  int32_t card = 1;
  pred_entry_t *entry = &(pred_tbl->entries[predicate]);
  int32_t i;
  for (i = 0; i < entry->arity; i++){
    card *= sort_table->entries[entry->signature[i]].cardinality;
  }
  return card;
}

int32_t all_atoms_cardinality(pred_tbl_t *pred_tbl, sort_table_t *sort_table) {
  int32_t i;
  int32_t card = 0;
  for (i = 0; i < pred_tbl->num_preds; i++){
    card += pred_cardinality(pred_tbl, sort_table, i);
  }
  return card;
}

//static clause_buffer_t rule_atom_buffer = {0, NULL}; 

// void rule_atom_buffer_resize(int32_t size){
//     if (rule_atom_buffer.data == NULL){
//     rule_atom_buffer.data = (int32_t *) safe_malloc(INIT_CLAUSE_SIZE * sizeof(int32_t));
//     rule_atom_buffer.size = INIT_CLAUSE_SIZE;
//   }
//   int32_t size = rule_atom_buffer.size;
//   if (size < length){
//     if (MAXSIZE(sizeof(int32_t), 0) - size <= size/2){
//       out_of_memory();
//     }
//     size += size/2;
//     rule_atom_buffer.data =
//       (int32_t  *) safe_realloc(rule_atom_buffer.data, size * sizeof(int32_t));
//     rule_atom_buffer.size = size;
//   }
// }


// bool check_clause_instance(samp_table_t *table,
// 			   substit_buffer_t *substit,
// 			   samp_rule_t *rule,
// 			   int32_t atom_index){//index of atom being activated
//   //Use a local buffer to build the literals and check
//   //if they are active and not fixed true.
//   pred_tbl_t pred_tbl = &(table->pred_table.pred_tbl);
//   atom_table_t atom_table = &(table->atom_table);
//   int32_t predicate, arity, i, j;
//   samp_atom_t *atom;
//     samp_atom_t *rule_atom; 
//   array_hmap_pair_t *atom_map;
//   for (i = 0; i < rule->num_lits; i++){//for each literal
//     if (i != atom_index){//if literal is not the one being activated
//       atom = rule->literals[i]->atom;
//       predicate = atom->pred; 
//       arity = table->pred_table.pred_tbl[predicate].arity;
//       resize_rule_atom_buffer(arity);
//       rule_atom = (samp_atom_t *) &(rule_atom_buffer->data);
//       rule_atom->pred = predicate; 
//       for (j = 0; j < arity; j++){//copy each instantiated argument
// 	if (atom->args[j] < 0){
// 	  rule_atom->args[j] = subst_buffer->entries[-atom->args[j] - 1];
// 	} else {
// 	  rule_atom->args[j] = atom->args[j];
// 	}
//       }
//       //find the index of the atom
//       atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
// 				  arity + 1,
// 				  (int32_t *) rule_atom);
//       if (atom_map == NULL) return false;//atom is inactive
//       if (rule->neg &&
// 	  table->assignment[atom_map->val] == v_fixed_false){
// 	return false;//literal is fixed true
//       }
//       if (!rule->neg &&
// 	  table->assignment[atom_map->val] == v_fixed_true){
// 	return false;//literal is fixed true
//       }
//     }
//   }
//   return true;
// }

int32_t choose_random_atom(samp_table_t *table){
  uint32_t i, atom_num, anum;
  int32_t card, all_card, acard, pcard, predicate;
  pred_tbl_t *pred_tbl = &table->pred_table.pred_tbl; // Indirect preds
  atom_table_t *atom_table = &table->atom_table;
  sort_table_t *sort_table = &table->sort_table;
  const_table_t *const_table = &table->const_table;
  pred_entry_t *pred_entry;

  // Get the number of possible indirect atoms
  all_card = all_atoms_cardinality(pred_tbl, sort_table);

  atom_num = random_uint(all_card);
  //assert(valid_table(table));

  predicate = 1; // Skip past true
  acard = 0;
  while (true) {//determine the predicate
    assert (predicate <= pred_tbl->num_preds);
    pcard = pred_cardinality(pred_tbl, sort_table, predicate);
    if (acard + pcard >= atom_num) {
      break;
    }
    acard += pcard;
    predicate++;
  }
  //assert(valid_table(table));
  assert(pred_cardinality(pred_tbl, sort_table, predicate) != 0);

  anum = atom_num - acard; //gives the position of atom within predicate
  //Now calculate the arguments.  We represent the arguments in
  //little-endian form
  pred_entry = &pred_tbl->entries[predicate];
  int32_t *signature = pred_entry->signature;
  int32_t arity = pred_entry->arity;
  atom_buffer_resize(arity);
  int32_t constant;
  samp_atom_t *atom = (samp_atom_t *) atom_buffer.data;
  //Build atom from atom_num by successive division
  atom->pred = predicate;
  for (i = 0; i < arity; i++){
    card = sort_table->entries[signature[i]].cardinality;
    constant = anum % card; //card can't be zero
    anum = anum/card;
    atom->args[i] = sort_table->entries[signature[i]].constants[constant];
    // Quick typecheck
    assert(const_sort_index(atom->args[i],const_table) == signature[i]);
  }
  
  //assert(valid_table(table));

  array_hmap_pair_t *atom_map;
  atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
				  arity + 1,
				  (int32_t *) atom);
  //assert(valid_table(table));
  if (atom_map == NULL){//need to activate atom
    if (get_verbosity_level() > 1) {
      printf("Activating atom ");
      print_atom(atom, table);
      printf("\n");
      fflush(stdout);
    }
    return activate_atom(table, atom);
  } else {
    return atom_map->val;
  }
}

void lazy_sample_sat_body(samp_table_t *table, double sa_probability,
			  double samp_temperature, double rvar_probability){
  //Assumed that table is in a valid state with a random assignment.
  //We first decide on simulated annealing vs. walksat.
  clause_table_t *clause_table = &table->clause_table;
  atom_table_t *atom_table = &table->atom_table;
  samp_truth_value_t *assignment;
  int32_t dcost;
  double choice; 
  int32_t var;
  uint32_t clause_position;
  samp_clause_t *link;

  assignment = atom_table->assignment[atom_table->current_assignment];
  //assert(valid_table(table));
  choice = choose();
  if (clause_table->num_unsat_clauses <= 0 || choice < sa_probability) {
    /*
     * Simulated annealing step
     */
    // choose a random atom
    //assert(valid_table(table));
    var = choose_random_atom(table);
    //var = choose_unfixed_variable(assignment, atom_table->num_vars,
    //			  atom_table->num_unfixed_vars);
    //assert(valid_table(table));
    if (var == -1) return;
    cost_flip_unfixed_variable(table, &dcost, var);
    //assert(valid_table(table));
    if (dcost < 0){
      flip_unfixed_variable(table, var);
      //assert(valid_table(table));
    } else {
      choice = choose();
      if (choice < exp(-dcost/samp_temperature)) {
	flip_unfixed_variable(table, var);
	//assert(valid_table(table));
      }
    }
  } else {
    /*
     * Walksat step
     */
    //choose an unsat clause
    clause_position = random_uint(clause_table->num_unsat_clauses);
    link = clause_table->unsat_clauses;
    while (clause_position != 0){
      link = link->link;
      clause_position--;
    } 
    //assert(valid_table(table));
    //link points to chosen clause
    var = choose_clause_var(table, link, assignment, rvar_probability);
    flip_unfixed_variable(table, var);
    //assert(valid_table(table));
  }
}

int32_t first_lazy_sample_sat(samp_table_t *table, double sa_probability,
			      double samp_temperature, double rvar_probability,
			      uint32_t max_flips){
  int32_t conflict;
  
  conflict = init_sample_sat(table);
  if (conflict == -1) return -1;
  uint32_t num_flips = max_flips;
  while (table->clause_table.num_unsat_clauses > 0 &&
	 num_flips > 0){
    lazy_sample_sat_body(table, sa_probability, samp_temperature,
			 rvar_probability);
    num_flips--;
  }
  if (table->clause_table.num_unsat_clauses > 0){
    fprintf(stderr, "Initialization failed to find a model; increase max_flips\n");
    return -1;
  }
  update_pmodel(table);
  return 0;
}

/*
 * One round of the mc_sat loop:
 * - select a set of live clauses from the current assignment
 * - compute a new assignment by sample_sat
 */
void lazy_sample_sat(samp_table_t *table, double sa_probability,
		     double samp_temperature, double rvar_probability,
		     uint32_t max_flips, uint32_t max_extra_flips) {
  int32_t conflict;
  //assert(valid_table(table));
  conflict = reset_sample_sat(table);
  //assert(valid_table(table));
  uint32_t num_flips = max_flips;
  if (conflict != -1) {
    while (table->clause_table.num_unsat_clauses > 0 && num_flips > 0) {
      lazy_sample_sat_body(table, sa_probability, samp_temperature,
			   rvar_probability);
      //assert(valid_table(table));
      num_flips--;
    }
    if (max_extra_flips < num_flips){
      num_flips = max_extra_flips;
    }
    while (num_flips > 0){
      lazy_sample_sat_body(table, sa_probability, samp_temperature,
			   rvar_probability);
      //assert(valid_table(table));
      num_flips--;
    }
  }
    
  if (conflict != -1 && table->clause_table.num_unsat_clauses == 0){
    update_pmodel(table);
  } else { 
    /*
     * Sample sat did not find a model (within max_flips)
     * restore the earlier assignment
     */
    if (conflict == -1){
      cprintf(2, "Hit a conflict.\n");
    } else {
      cprintf(2, "Failed to find a model.\n");
    }

    // Flip current_assignment (restore the saved assignment)
    assert(table->atom_table.current_assignment == 0
	   || table->atom_table.current_assignment == 1);
    table->atom_table.current_assignment ^= 1;
    
    empty_clause_lists(table);
    init_clause_lists(&table->clause_table);
    update_pmodel(table);
  }
}

/*
 * Top-level LAZY-MCSAT call
 *
 * Parameters for lazy sample sat:
 * - sa_probability = probability of a simulated annealing step
 * - samp_temperature = temperature for simulated annealing
 * - rvar_probability = probability used by a Walksat step:
 *   a Walksat step selects an unsat clause and flips one of its variables
 *   - with probability rvar_probability, that variable is chosen randomly
 *   - with probability 1(-rvar_probability), that variable is the one that
 *     results in minimal increase of the number of unsat clauses.
 * - max_flips = bound on the number of sample_sat steps
 * - max_extra_flips = number of additional (simulated annealing) steps to perform
 *   after a satisfying assignment is found
 *
 * Parameter for mc_sat:
 * - max_samples = number of samples generated
 */
void lazy_mc_sat(samp_table_t *table, double sa_probability,
		 double samp_temperature, double rvar_probability,
		 uint32_t max_flips, uint32_t max_extra_flips,
		 uint32_t max_samples){
  int32_t conflict;
  uint32_t i;

  conflict = first_lazy_sample_sat(table, sa_probability, samp_temperature,
				   rvar_probability, max_flips);
  if (conflict == -1) {
    printf("Found conflict in initialization.\n");
    return;
  }

  //  print_state(table, 0);  
  //assert(valid_table(table));
  for (i = 0; i < max_samples; i++){
    cprintf(2, "---- sample[%"PRIu32"] ---\n", i);
    lazy_sample_sat(table, sa_probability, samp_temperature,
		    rvar_probability, max_flips, max_extra_flips);
    //    print_state(table, i+1);
    //assert(valid_table(table));
  }

  //print_atoms(table);
}
