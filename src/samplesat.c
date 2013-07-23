#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "memalloc.h"
#include "prng.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "gcd.h"
#include "utils.h"
#include "print.h"
#include "input.h"
#include "vectors.h"
#include "SFMT.h"
#include "yacc.tab.h"
#include "samplesat.h"

clause_buffer_t atom_buffer = { 0, NULL };

/* Allocates enough space for atom_buffer */
void atom_buffer_resize(clause_buffer_t *atom_buffer, int32_t arity) {
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

/*
 * A SAT solver for only the hard formulas
 *
 * TODO: To be replaced by other SAT solvers. It should just give a
 * solution but not necessarily uniformly drawn.
 */
void first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability, uint32_t max_flips) {
	clause_table_t *clause_table = &table->clause_table;
	int32_t conflict;

	conflict = init_first_sample_sat(table);

	if (conflict == -1)
		return -1;

	uint32_t num_flips = max_flips;
	while (clause_table->num_unsat_clauses > 0 && num_flips > 0) {
		if (sample_sat_body(table, lazy, sa_probability, samp_temperature,
					rvar_probability) == -1)
			return -1;
		num_flips--;
	}

	if (clause_table->num_unsat_clauses > 0) {
		printf("num of unsat clauses = %d\n", clause_table->num_unsat_clauses);
		mcsat_err("Initialization failed to find a model; increase max_flips\n");
		return -1;
	}

	return 0;
}

/*
 * Next, need to write the main samplesat routine.
 * How should hard clauses be represented, with weight MAX_DOUBLE?
 * The parameters include clause_set, DB, KB, maxflips, p_anneal,
 * anneal_temp, p_walk.
 * 
 * The assignment will map each atom to undef, T, F, FixedT, FixedF,
 * DB_T, or DB_F.   The latter assignments are fixed by the closed world
 * assumption on the DB.  The other fixed assignments are those obtained
 * from unit propagation on the selected clauses.
 * 
 * The samplesat routine starts with a random assignment to the non-fixed
 * variables and a selection of alive clauses.  The cost is the
 * number of unsat clauses.  It then either (with prob p_anneal) picks a
 * variable and flips it (if it reduces cost) or with probability (based on
 * the cost diff).  Otherwise, it does a walksat step.
 * The tricky question is what it means to activate a clause or atom.
 * 
 * Code for random selection with filtering is in smtcore.c
 * (select_random_bvar)
 * 
 * First step is to write a unit-propagator.  The propagation is
 * done repeatedly to handle recent additions.
 */
void sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability, 
		uint32_t max_flips, uint32_t max_extra_flips) {
	atom_table_t *atom_table = &table->atom_table;
	clause_table_t *clause_table = &table->clause_table;
	int32_t conflict;

	cprintf(2, "[sample_sat] started ...\n");

	init_sample_sat(table);

	uint32_t num_flips = max_flips;
	while (num_flips > 0 && conflict == 0) {
		if (clause_table->num_unsat_clauses == 0) {
			if (max_extra_flips <= 0) {
				break;
			} else {
				max_extra_flips--;
			}
		}
		conflict = sample_sat_body(table, lazy, sa_probability, samp_temperature,
				rvar_probability);
		//assert(valid_table(table));
		num_flips--;
	}

	/* If we successfully get a model, try to walk around to neighboring models;
	 * Note that if the model is isolated, it will remain unchanged. */
	if (conflict != -1 && clause_table->num_unsat_clauses == 0) {
		num_flips = genrand_uint(max_extra_flips);
		while (num_flips > 0 && conflict == 0) {
			conflict = sample_sat_body(table, lazy, 1, DBL_MAX, rvar_probability);
			//assert(valid_table(table));
			num_flips--;
		}
	}
	
	/*
	 * If Sample sat did not find a model (within max_flips)
	 * restore the earlier assignment
	 */
	if (conflict == -1 || clause_table->num_unsat_clauses > 0) {
		if (conflict == -1) {
			cprintf(2, "[sample_sat] Hit a conflict.\n");
		} else {
			cprintf(2, "[sample_sat] Failed to find a model. \
Consider increasing max_flips and max_tries - see mcsat help.\n");
		}

		// Flip current_assignment (restore the saved assignment)
		atom_table->current_assignment_index ^= 1;
		atom_table->current_assignment = atom_table->assignment[atom_table->current_assignment_index];

		empty_clause_lists(table);
		init_clause_lists(&table->clause_table);
	}
}

/* A step of sample sat */
int32_t sample_sat_body(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability) {
	// Assumed that table is in a valid state with a random assignment.
	// We first decide on simulated annealing vs. walksat.
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t dcost;
	double choice;
	int32_t var;
	uint32_t clause_position;
	samp_clause_t *link;
	int32_t conflict = 0;

	choice = choose();
	cprintf(3, "\n[sample_sat_body] num_unsat = %d, choice = % .4f, sa_probability = % .4f\n",
			clause_table->num_unsat_clauses, choice, sa_probability);
	if (clause_table->num_unsat_clauses <= 0 || choice < sa_probability) {
		/* Simulated annealing step */
		cprintf(3, "[sample_sat_body] simulated annealing\n");

		if (!lazy) {
			var = choose_unfixed_variable(assignment, atom_table->num_vars,
					atom_table->num_unfixed_vars);
		} else {
			var = choose_random_atom(table);
		}

		if (var == -1 || fixed_tval(assignment[var])) {
			return 0;
		}
		
		cost_flip_unfixed_variable(table, &dcost, var);
		cprintf(3, "[sample_sat_body] simulated annealing num_unsat = %d, var = %d, dcost = %d\n",
				clause_table->num_unsat_clauses, var, dcost);
		if (dcost <= 0) {
			conflict = flip_unfixed_variable(table, var);
		} else {
			choice = choose();
			if (choice < exp(-dcost / samp_temperature)) {
				conflict = flip_unfixed_variable(table, var);
			}
		}
	} else {
		/*
		 * Walksat step
		 */
		cprintf(3, "[sample_sat_body] WalkSAT\n");

		/* choose an unsat clause, link points to chosen clause */
		// clause_position = random_uint(clause_table->num_unsat_clauses);
		clause_position = genrand_uint(clause_table->num_unsat_clauses);
		link = clause_table->unsat_clauses;
		/*
		 * TODO: This is very inefficient when the number of unsat clauses
		 * is very large. We should consider a different data structure than
		 * a link to store the unsatisfied clauses. This data structure should
		 * support random access (for getting a random unsat clause) as well
		 * as fast insert/remove.
		 */
		while (clause_position != 0) {
			link = link->link;
			clause_position--;
		}

		var = choose_clause_var(table, link, assignment, rvar_probability);
		if (get_verbosity_level() >= 3) {
			printf("[sample_sat_body] WalkSAT chose variable ");
			print_atom(atom_table->atom[var], table);
			printf(" from unsat clause ");
			print_clause(link, table);
			printf("\n");
		}
		conflict = flip_unfixed_variable(table, var);
	}
	return conflict;
}

int32_t choose_unfixed_variable(samp_truth_value_t *assignment,
		int32_t num_vars, int32_t num_unfixed_vars) {
	uint32_t var, d, y;

	if (num_unfixed_vars == 0)
		return -1;

	// var = random_uint(num_vars);
	var = genrand_uint(num_vars);
	if (!fixed_tval(assignment[var]))
		return var;
	// d = 1 + random_uint(num_vars - 1);
	d = 1 + genrand_uint(num_vars - 1);
	while (gcd32(d, num_vars) != 1)
		d--;

	y = var;
	do {
		y += d;
		if (y >= num_vars)
			y -= num_vars;
		assert(var != y);
	} while (fixed_tval(assignment[y]));
	return y;
}

int32_t pred_cardinality(pred_tbl_t *pred_tbl, sort_table_t *sort_table,
		int32_t predicate) {
	if (predicate < 0 || predicate >= pred_tbl->num_preds) {
		return -1;
	}
	int32_t card = 1;
	pred_entry_t *entry = &(pred_tbl->entries[predicate]);
	int32_t i;
	for (i = 0; i < entry->arity; i++) {
		card *= sort_table->entries[entry->signature[i]].cardinality;
	}
	return card;
}

int32_t all_atoms_cardinality(pred_tbl_t *pred_tbl, sort_table_t *sort_table) {
	int32_t i;
	int32_t card = 0;
	for (i = 1; i < pred_tbl->num_preds; i++) {
		card += pred_cardinality(pred_tbl, sort_table, i);
	}
	return card;
}

/**
 * Random number generator:
 * - returns a floating point number in the interval [0.0, 1.0)
 */
double choose() {
	//return ((double) random()) / ((double) RAND_MAX + 1.0);
	//return ((double) gen_rand64()) / ((double) UINT64_MAX + 1.0);
	return genrand_real2();
}

/*
 * choose a random atom for simulated annealing step in sample SAT. The lazy
 * version of choose_unfixed_variable.  First choose a random atom, regardless
 * whether its value is fixed or not (because we can calculate the total number
 * of atoms and it is convenient to randomly choose one from them). If its
 * value is fixed, we skip this flip using the following statement (return 0).
 */
int32_t choose_random_atom(samp_table_t *table) {
	uint32_t i, atom_num, anum;
	int32_t card, all_card, acard, pcard, predicate;
	pred_tbl_t *pred_tbl = &table->pred_table.pred_tbl; // Indirect preds
	atom_table_t *atom_table = &table->atom_table;
	sort_table_t *sort_table = &table->sort_table;
	pred_entry_t *pred_entry;

	// Get the number of possible indirect atoms
	all_card = all_atoms_cardinality(pred_tbl, sort_table);

	atom_num = random_uint(all_card);
	//assert(valid_table(table));

	predicate = 1; // Skip past true
	acard = 0;
	while (true) { //determine the predicate
		assert(predicate <= pred_tbl->num_preds);
		pcard = pred_cardinality(pred_tbl, sort_table, predicate);
		if (acard + pcard > atom_num) {
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
	atom_buffer_resize(&atom_buffer, arity);
	int32_t constant;
	samp_atom_t *atom = (samp_atom_t *) atom_buffer.data;
	//Build atom from atom_num by successive division
	atom->pred = predicate;
	for (i = 0; i < arity; i++) {
		card = sort_table->entries[signature[i]].cardinality;
		constant = anum % card; //card can't be zero
		anum = anum / card;
		if (sort_table->entries[signature[i]].constants == NULL) {
			// Must be an integer
			atom->args[i] = constant;
		} else {
			atom->args[i] = sort_table->entries[signature[i]].constants[constant];
			// Quick typecheck
			assert(const_sort_index(atom->args[i], &table->const_table) == signature[i]);
		}
	}

	//assert(valid_table(table));

	array_hmap_pair_t *atom_map;
	atom_map = array_size_hmap_find(&atom_table->atom_var_hash, arity + 1,
			(int32_t *) atom);
	//assert(valid_table(table));

	if (atom_map == NULL) { //need to activate atom
		add_and_activate_atom(table, atom);
		atom_map = array_size_hmap_find(&atom_table->atom_var_hash, arity + 1,
				(int32_t *) atom);
		if (atom_map == NULL) {
			printf("Something is wrong in choose_random_atom\n");
			return 0;
		} else {
			return atom_map->val;
		}
	} else {
		return atom_map->val;
	}
}

/*
 * A temperary stack that store the unfixed variables in a clause;
 * used in choose_clause_var.
 */
static integer_stack_t clause_var_stack = { 0, 0, NULL };

/*
 * TODO: Is the returned variable always active?
 * if not, we should activate it somewhere
 */
int32_t choose_clause_var(samp_table_t *table, samp_clause_t *link,
		samp_truth_value_t *assignment, double rvar_probability) {
	uint32_t i, var;

	if (clause_var_stack.size == 0) {
		init_integer_stack(&(clause_var_stack), 0);
	} else {
		clear_integer_stack(&clause_var_stack);
	}

	for (i = 0; i < link->numlits; i++) {
		if (!link->frozen[i] && !fixed_tval(assignment[var_of(link->disjunct[i])]))
			push_integer_stack(i, &clause_var_stack);
	} //all unfrozen, unfixed vars are now in clause_var_stack

	double choice = choose();
	if (choice < rvar_probability) {//flip a random unfixed variable
		// var = random_uint(length_integer_stack(&clause_var_stack));
		var = genrand_uint(length_integer_stack(&clause_var_stack));
	} else {
		int32_t dcost = INT32_MAX;
		int32_t vcost = 0;
		var = length_integer_stack(&clause_var_stack);
		for (i = 0; i < length_integer_stack(&clause_var_stack); i++) {
			cost_flip_unfixed_variable(
					table,
					&vcost,
					var_of(link->disjunct[nth_integer_stack(i, &clause_var_stack)]));
			if (vcost < dcost) {
				dcost = vcost;
				vcost = 0;
				var = i;
			}
		}
	}
	assert(var < length_integer_stack(&clause_var_stack));
	return var_of(link->disjunct[nth_integer_stack(var, &clause_var_stack)]);
}

//computes the cost of flipping an unfixed variable without the actual flip
void cost_flip_unfixed_variable(samp_table_t *table, int32_t *dcost,
		int32_t var) {
	*dcost = 0;
	samp_literal_t lit, nlit;
	uint32_t i;
	atom_table_t *atom_table = &(table->atom_table);
	samp_truth_value_t *assignment = atom_table->current_assignment;

	if (assigned_true(assignment[var])) {
		nlit = neg_lit(var);
		lit = pos_lit(var);
	} else {
		nlit = pos_lit(var);
		lit = neg_lit(var);
	}
	samp_clause_t *link = table->clause_table.watched[lit];
	while (link != NULL) {
		i = 1;
		while (i < link->numlits && assigned_false_lit(assignment,
					link->disjunct[i])) {
			i++;
		}
		if (i >= link->numlits) {
			*dcost += 1;
		}
		link = link->link;
	}
	//Now, subtract the weight of the unsatisfied clauses that can be flipped
	link = table->clause_table.unsat_clauses;
	while (link != NULL) {
		i = 0;
		while (i < link->numlits && link->disjunct[i] != nlit) {
			i++;
		}
		if (i < link->numlits) {
			*dcost -= 1;
		}
		link = link->link;
	}
}

/*
 * Executes a variable flip by first scanning all the previously sat clauses
 * in the watched list, and then moving any previously unsat clauses to the
 * sat/watched list.
 */
int32_t flip_unfixed_variable(samp_table_t *table, int32_t var) {
	//  double dcost = 0;   //dcost seems unnecessary
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	cprintf(4, "Flipping variable %"PRId32" to %s\n", var,
			assigned_true(assignment[var]) ? "false" : "true");
	if (assigned_true(assignment[var])) {
		assignment[var] = v_false;
		link_propagate(table, pos_lit(var));
	} else {
		assignment[var] = v_true;
		link_propagate(table, neg_lit(var));
	}
	if (scan_unsat_clauses(table) == -1) {
		return -1;
	}
	return process_fixable_stack(table);
}

/*
 * Propagates a truth assignment on a link of watched clauses for a literal
 * that has been assigned false.  All the literals are assumed to have truth
 * values.  The watched literal points to a true literal if there is one in
 * the clause.  Whenever a watched literal is assigned false, the
 * propagation must find a new true literal, or retain the existing one.  If
 * a clause is falsified, its weight is added to the delta cost.  The
 * clauses containing the negation of the literal must be reevaluated and
 * assigned true if previously false.
 */
void link_propagate(samp_table_t *table, samp_literal_t lit) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t numlits, i;
	samp_literal_t new_watched;
	samp_clause_t *next_link;
	samp_clause_t *link;
	samp_clause_t **link_ptr;
	samp_literal_t *disjunct;

	//cprintf(0, "link_propagate called\n");

	assert(assigned_false_lit(assignment, lit)); // FIXME ?? this assertion fails!

	link_ptr = &(clause_table->watched[lit]);
	link = clause_table->watched[lit];
	while (link != NULL) {
		//cprintf(0, "Checking watched\n");
		numlits = link->numlits;
		disjunct = link->disjunct;

		assert(disjunct[0] == lit && numlits > 1);
		assert(assigned_false_lit(assignment, disjunct[0]));

		i = get_true_lit(assignment, disjunct, numlits);
		//i = 1;
		//while (i < numlits && assigned_false_lit(assignment, disjunct[i])) {
		//	i++;
		//}

		if (i < numlits) {
			/*
			 * the clause is still true: make tmp = disjunct[i] the new watched literal
			 */
			new_watched = disjunct[i];
			disjunct[i] = disjunct[0];
			disjunct[0] = new_watched;

			// add the clause to the head of watched[tmp]
			next_link = link->link;
			*link_ptr = next_link;
			push_clause(link, &clause_table->watched[disjunct[0]]);
			assert(assigned_true_lit(assignment,
						clause_table->watched[disjunct[0]]->disjunct[0]));
		} else {
			/* move the clause to the unsat_clause list */
			//cprintf(0, "Adding to usat_clauses\n");
			next_link = link->link;
			*link_ptr = next_link;
			push_clause(link, &clause_table->unsat_clauses);
			clause_table->num_unsat_clauses++;
		}
		link = next_link;
	}

	clause_table->watched[lit] = NULL;//since there are no clauses where it is true
}

/*
 * TODO
 */
int32_t process_fixable_stack(samp_table_t *table) {
	samp_literal_t lit;
	int32_t conflict = 0;
	while (!empty_integer_stack(&(table->fixable_stack)) && conflict == 0) {
		while (!empty_integer_stack(&(table->fixable_stack)) && conflict == 0) {
			lit = top_integer_stack(&(table->fixable_stack));
			pop_integer_stack(&(table->fixable_stack));
			link_propagate(table, not(lit));
		}
		conflict = scan_unsat_clauses(table);
	}
	return conflict;
}

/*
 * Scans the unsat clauses to find those that are sat or to propagate
 * fixed truth values.  The propagating clauses are placed on the sat_clauses
 * list, and the propagated literals are placed in fixable_stack so that they
 * TODO: unfinished comments
 *
 * Returns -1 if a conflict is detected, 0 otherwise.
 */
int32_t scan_unsat_clauses(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	samp_clause_t *unsat_clause;
	samp_clause_t **unsat_clause_ptr;
	unsat_clause_ptr = &clause_table->unsat_clauses;
	unsat_clause = *unsat_clause_ptr;
	int32_t i;
	int32_t fixable;
	samp_literal_t lit;
	char *atom_str;

	while (unsat_clause != NULL) {//see if the clause is fixed-unit propagating
		//cprintf(0, "Scanning unsat clauses\n");
		fixable = get_fixable_literal(assignment, unsat_clause->disjunct,
				unsat_clause->numlits);
		if (fixable >= unsat_clause->numlits) {
			// Conflict detected (a fixed unsat clause)
			return -1;
		}
		if (fixable == -1) { // if not propagating
			i = get_true_lit(assignment, unsat_clause->disjunct,
					unsat_clause->numlits);
			if (i < unsat_clause->numlits) {//then lit occurs in the clause
				//      *dcost -= unsat_clause->weight;  //subtract weight from dcost
				//swap literal to watched position
				lit = unsat_clause->disjunct[i]; 
				unsat_clause->disjunct[i] = unsat_clause->disjunct[0];
				unsat_clause->disjunct[0] = lit;

				*unsat_clause_ptr = unsat_clause->link;
				unsat_clause->link = clause_table->watched[lit]; //push into watched list
				clause_table->watched[lit] = unsat_clause;
				clause_table->num_unsat_clauses--;
				unsat_clause = *unsat_clause_ptr;
				assert(assigned_true_lit(assignment,
							clause_table->watched[lit]->disjunct[0]));
			} else {
				unsat_clause_ptr = &(unsat_clause->link); //move to next unsat clause
				unsat_clause = unsat_clause->link;
			}
		} else { // we need to fix the truth value of disjunct[fixable]
			// swap the literal to the front
			lit = unsat_clause->disjunct[fixable]; 
			unsat_clause->disjunct[fixable] = unsat_clause->disjunct[0];
			unsat_clause->disjunct[0] = lit;

			atom_str = var_string(var_of(lit), table);
			cprintf(2, "[scan_unsat_clauses] Fixing variable %s\n", atom_str);
			free(atom_str);
			if (!fixed_tval(assignment[var_of(lit)])) {
				if (is_pos(lit)) {
					assignment[var_of(lit)] = v_fixed_true;
				} else {
					assignment[var_of(lit)] = v_fixed_false;
				}
				atom_table->num_unfixed_vars--;
			}
			assert(assigned_true_lit(assignment, lit));
			//if (assigned_true_lit(assignment, lit)) { //push clause into fixed sat list
			push_integer_stack(lit, &(table->fixable_stack));
			//}
			*unsat_clause_ptr = unsat_clause->link;
			push_clause(unsat_clause, &clause_table->sat_clauses);
			assert(assigned_fixed_true_lit(assignment,
						clause_table->sat_clauses->disjunct[0]));
			unsat_clause = *unsat_clause_ptr;
			clause_table->num_unsat_clauses--;
		}
	}
	return 0;
}

/*
 * Returns a literal index corresponding to a fixed true literal or a
 * unique non-fixed implied literal
 */
int32_t get_fixable_literal(samp_truth_value_t *assignment,
		samp_literal_t *disjunct, int32_t numlits) {
	int32_t i, j;
	i = 0;
	while (i < numlits && assigned_fixed_false_lit(assignment, disjunct[i])) {
		i++;
	} // i = numlits or not fixed, or disjunct[i] is true; now check the remaining lits
	if (i < numlits) {
		if (assigned_fixed_true_lit(assignment, disjunct[i]))
			return i;
		j = i + 1;
		while (j < numlits && assigned_fixed_false_lit(assignment, disjunct[j])) {
			j++;
		} // if j = numlits, then i is propagated
		if (j < numlits) {
			if (assigned_fixed_true_lit(assignment, disjunct[j]))
				return j;
			return -1; // there are two unfixed lits
		}
	}
	return i; // i is fixable
}

int32_t get_true_lit(samp_truth_value_t *assignment, samp_literal_t *disjunct,
		int32_t numlits) {
	int32_t i;
	i = 0;
	while (i < numlits && assigned_false_lit(assignment, disjunct[i])) {
		i++;
	}
	return i;
}

/*
 * Sets the value of an atom.
 * If the atom is inactive AND the value is non-default, we need to activate
 * the atom;
 * If the atom has a fixed value and is assigned to the opposite value, return
 * inconsistency;
 * If the atom has a fixed value and is assigned to the same value, do nothing;
 * If the atom has a non-fixed value and is set to a fixed value, run
 * unit_propagation...;
 * If the atom has a non-fixed value and is set to the opposite non-fixed value,
 * just change the value (and change the state of the relavent clauses?)
 *
 * TODO: all the change of atom value should go through this
 */
int32_t set_atom_tval(int32_t var, samp_truth_value_t tval, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_atom_t *atom = atom_table->atom[var];
	pred_entry_t *pred_entry = get_pred_entry(pred_table, atom->pred);
	samp_truth_value_t *assignment = atom_table->current_assignment;
	
	/* if it has been fixed, check if consistent */
	if (fixed_tval(assignment[var])) {
		if ((assigned_true(assignment[var]) && assigned_false(tval))
				|| (assigned_false(assignment[var]) && assigned_true(tval))) {
			return -1;
		} else {
			return 0;
		}
	}
	
	if (get_verbosity_level() >= 3) {
		char *var_str = var_string(var, table);
		printf("[set_atom_tval] Setting the value of %s to %s\n",
				var_str, string_of_tval(tval));
		free(var_str);
	}

	assignment[var] = tval;

	/* If the atom is inactive AND the value is non-default, activate the atom. */
	if (!atom_table->active[var]
			&& assigned_true(tval) != pred_default_value(pred_entry)) {
		activate_atom(table, var);
	}
	return 0;
}

/*
 * In UNIT PROPAGATION, fix a literal's value
 */
int32_t fix_lit_tval(samp_table_t *table, int32_t lit, bool tval) {
	int32_t var = var_of(lit);
	uint32_t sign = sign_of_lit(lit); // 0: pos; 1: neg
	int32_t ret;
	//char *atom_str = var_string(var, table);

	if (sign != tval) {
		//cprintf(4, "[fix_lit_tval] Fixing %s to true\n", atom_str);
		//free(atom_str);
		ret = set_atom_tval(var, v_fixed_true, table);
		if (ret < 0) return ret;
		table->atom_table.num_unfixed_vars--;
		link_propagate(table, neg_lit(var));
	} else {
		//cprintf(4, "[fix_lit_tval] Fixing %s to false\n", atom_str);
		//free(atom_str);
		ret = set_atom_tval(var, v_fixed_false, table);
		if (ret < 0) return ret;
		table->atom_table.num_unfixed_vars--;
		link_propagate(table, pos_lit(var));
	}

	return 0;
}

int32_t inline fix_lit_true(samp_table_t *table, int32_t lit) {
	return fix_lit_tval(table, lit, true);
}

int32_t inline fix_lit_false(samp_table_t *table, int32_t lit) {
	return fix_lit_tval(table, lit, false);
}

/*
 * Fixes the truth values derived from unit and negative weight clauses
 */
int32_t negative_unit_propagate(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	samp_clause_t *link;
	samp_clause_t **link_ptr;
	int32_t i;
	int32_t conflict = 0;
	double abs_weight;

	link_ptr = &clause_table->negative_or_unit_clauses;
	link = *link_ptr;
	/* TODO Guess the list of negative_or_unit_clauses is keep changing while
	 * `propogating'. How does the linked list work? */
	while (link != NULL) {
		abs_weight = fabs(link->weight);
		cprintf(3, "[negative_unit_propagate] probability is %.4f\n",
				1 - exp(-abs_weight));
		if (abs_weight == DBL_MAX 
				|| choose() < 1 - exp(-abs_weight)
				) {
			/*
			 * FIXME what does choose() > exp(-abs_weight) mean
			 * This clause is considered alive from now on
			 */
			if (link->weight >= 0) {
				assert(link->numlits == 1); // must be a unit clause
				conflict = fix_lit_true(table, link->disjunct[0]);
				if (conflict == -1) {
					char *litstr = literal_string(link->disjunct[0], table);
					cprintf(3, "[negative_unit_propagate] conflict fixing lit %s to true: ",
							litstr);
					free(litstr);
					return -1;
				} else {
					cprintf(3, "[negative_unit_propagate] Picking unit clause\n");
				}
			} else { // we have a negative weight clause
				for (i = 0; i < link->numlits; i++) {
					samp_literal_t lit = link->disjunct[i];
					conflict = fix_lit_false(table, lit);
					if (conflict == -1) {
						char *litstr = literal_string(lit, table);
						cprintf(3, "[negative_unit_propagate] conflict fixing lit %s to true: ",
								litstr);
						free(litstr);
						return -1;
					}
				}
			}
			link_ptr = &link->link;
			link = link->link;
		} else {
			/* Move the clause to the dead_negative_or_unit_clauses list */
			*link_ptr = link->link;
			push_clause(link, &clause_table->dead_negative_or_unit_clauses);
			link = *link_ptr;
		}
	}

	return 0; //no conflict
}

/*
 * Assigns random truth values to unfixed vars and returns number of unfixed
 * vars (num_unfixed_vars).
 */
void init_random_assignment(samp_table_t *table, int32_t *num_unfixed_vars) {
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	uint32_t i, num;
	*num_unfixed_vars = 0;
	char *atom_str;

	cprintf(3, "[init_random_assignment] num_vars = %d\n", atom_table->num_vars);
	for (i = 0; i < atom_table->num_vars; i++) {
		if (!fixed_tval(assignment[i])) {
			atom_str = var_string(i, table);
			if (choose() < 0.5) {
				set_atom_tval(i, v_false, table);
				cprintf(3, "[init_random_assignment] assigned false to %s\n", 
						atom_str);
			} else {
				set_atom_tval(i, v_true, table);
				cprintf(3, "[init_random_assignment] assigned true to %s\n",
						atom_str);
			}
			free(atom_str);
			(*num_unfixed_vars)++;
		}
	}
}

/*
 * Initializes the variables for the first run of sample SAT.  Only hard
 * clauses are considered. Randomizes all active atoms and their neighboors.
 * 
 * TODO In the first run of sample SAT, only hard clauses are considered;
 * Want to generalize it to work for the following initialization of
 * sample SAT.
 */
int32_t init_first_sample_sat(samp_table_t *table) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);
	int32_t conflict = 0;

	empty_clause_lists(table);
	init_clause_lists(clause_table);

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] Started ...\n");
		print_assignment(table);
		print_clause_table(table, -1);
	}

	conflict = negative_unit_propagate(table);
	if (conflict == -1)
		return -1;

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] After negative_unit_propagation:\n");
		print_assignment(table);
		print_clause_table(table, -1);
	}

	int32_t num_unfixed_vars;
	init_random_assignment(table, &num_unfixed_vars);
	atom_table->num_unfixed_vars = num_unfixed_vars;

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] After init_random_assignment:\n");
		print_assignment(table);
		print_clause_table(table, -1);
	}

	/* Till now all the live clauses are in unsat_clauses */

	if (scan_unsat_clauses(table) == -1) {
		return -1;
	}
	return process_fixable_stack(table);
}

/*
 * The initialization phase for the mcmcsat step. First place all the clauses
 * in the unsat list, and then use scan_unsat_clauses to move them into
 * sat_clauses or the watched (sat) lists.
 *
 * Like init_first_sample_sat, but takes an existing state and sets it up for a
 * second round of sampling
 */
int32_t init_sample_sat(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	pred_table_t *pred_table = &table->pred_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;

	samp_truth_value_t *new_assignment;

	/*
	 * Flip the assignment vectors:
	 * - the current assignment is kept as a backup in case sample_sat fails
	 */
	atom_table->current_assignment_index ^= 1; // flip low order bit: 1 --> 0, 0 --> 1
	atom_table->current_assignment = atom_table->assignment[atom_table->current_assignment_index];
	new_assignment = atom_table->current_assignment;

	uint32_t i, choice;

	for (i = 0; i < atom_table->num_vars; i++) {
		new_assignment[i] = assignment[i];
		if (fixed_tval(assignment[i])) {
			new_assignment[i] = unfix_tval(assignment[i]);
		}
	}

	/* unit propagate for the new set of clauses */
	if (negative_unit_propagate(table) == -1)
		return -1;

	/* pick random assignments for the unfixed vars */
	int32_t num_unfixed_vars = 0;
	for (i = 0; i < atom_table->num_vars; i++) {
		//if (!atom_eatom(i, pred_table, atom_table)) { // not an evidence pred
		if (unfixed_tval(new_assignment[i])) {
			choice = (choose() < 0.5);
			if (choice == 0) {
				set_atom_tval(i, v_false, table);
			} else {
				set_atom_tval(i, v_true, table);
			}
			num_unfixed_vars++;
		}
	}

	atom_table->num_unfixed_vars = num_unfixed_vars;

	for (i = 0; i < atom_table->num_vars; i++) {
		// if value has changed ...
		if (assigned_true(assignment[i]) && assigned_false(new_assignment[i])) {
			link_propagate(table, pos_lit(i));
		}
		if (assigned_false(assignment[i]) && assigned_true(new_assignment[i])) {
			link_propagate(table, neg_lit(i));
		}
	}

	//move all sat_clauses to unsat_clauses for rescanning
	move_sat_to_unsat_clauses(clause_table);

	if (scan_unsat_clauses(table) == -1)
		return -1;

	if (process_fixable_stack(table) == -1)
		return -1;
	
	if (get_verbosity_level() >= 3) {
		printf("[init_sample_sat] After randomization, unsat = %d:\n",
				clause_table->num_unsat_clauses);
		print_assignment(table);
		print_clause_table(table, -1);
	}

	return 0;
}

