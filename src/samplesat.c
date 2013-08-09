#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "tables.h"
#include "memalloc.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "utils.h"
#include "print.h"
#include "vectors.h"
#include "buffer.h"
#include "clause_list.h"
#include "yacc.tab.h"
#include "ground.h"
#include "samplesat.h"

/*
 * Examines if a clause contains a fixable literal
 *
 * @disjunct: the literal array of the clause
 * 
 * returns: the literal index corresponding to a fixed true literal or a unique
 * non-fixed implied literal; -1 if there are at least two unfixed literals;
 * the total number of literals in the clause when the clause is fixed unsat.
 */
static int32_t get_fixable_literal(samp_truth_value_t *assignment,
		samp_literal_t *disjunct, int32_t numlits) {
	int32_t i, j;
	i = 0;
	while (i < numlits && assigned_fixed_false_lit(assignment, disjunct[i])) {
		i++;
	} 
	/* i = numlits or not fixed, or disjunct[i] is true; now check the remaining lits */
	if (i < numlits) {
		if (assigned_fixed_true_lit(assignment, disjunct[i]))
			return i;
		j = i + 1;
		while (j < numlits && assigned_fixed_false_lit(assignment, disjunct[j])) {
			j++;
		} 
		/* if j = numlits, then i is propagated */
		if (j < numlits) {
			if (assigned_fixed_true_lit(assignment, disjunct[j])) {
				return j;
			}
			else {
				/* there are two unfixed lits */
				return -1; 
			}
		}
	}
	return i; /* i is fixable */
}

/*
 * Returns: the index of a satisfied literal of a clause; the number of
 * literals if none exists.
 */
static int32_t get_true_lit(samp_truth_value_t *assignment, samp_literal_t *disjunct,
		int32_t numlits) {
	int32_t i;
	i = 0;
	while (i < numlits && assigned_false_lit(assignment, disjunct[i])) {
		i++;
	}
	return i;
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
static void link_propagate(samp_table_t *table, samp_literal_t lit) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t i;
	samp_literal_t new_watched;

	assert(assigned_false_lit(atom_table->assignment, lit));

	samp_clause_t *ptr;
	samp_clause_t *clause;
	for (ptr = clause_table->watched[lit].head; ptr != clause_table->watched[lit].tail; ) {
		clause = clause_list_pop(&clause_table->watched[lit], ptr);

		i = get_true_lit(atom_table->assignment, clause->disjunct,
				clause->numlits);

		if (i < clause->numlits) {
			/* the clause is still true, make disjunct[i] the new watched literal */
			new_watched = clause->disjunct[i];
			assert(new_watched != lit);

			clause_list_insert_head(clause, &clause_table->watched[new_watched]);
			assert(assigned_true_lit(atom_table->assignment,
						clause_table->watched[new_watched].head->link->disjunct[i]));
		} else {
			/* move the clause to the unsat_clause list */
			clause_list_insert_head(clause, &clause_table->unsat_clauses);
		}
	}

	/* since there are no clauses where it is true */
	assert(is_empty_clause_list(&clause_table->watched[lit]));

	/* TODO unsat clause list is dirty, need to rescan */
	move_unsat_to_live_clauses(clause_table);
}

/*
 * Try to set the value of an atom.
 *
 * If the atom has a non-fixed value and is set to a fixed value, run
 * unit_propagation...;
 * If the atom has a non-fixed value and is set to the opposite non-fixed value,
 * just change the value (and change the state of the relavent clauses?)
 *
 * @var: the variable whose value is to set
 * @tval: the value to set
 */
static int32_t set_atom_tval(int32_t var, samp_truth_value_t tval, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_atom_t *atom = atom_table->atom[var];
	pred_entry_t *pred_entry = get_pred_entry(pred_table, atom->pred);

	assert(valid_table(table));
	
	/*
	 * If the atom has been fixed, check if consistent: if it is assigned to
	 * the opposite value, return inconsistency; otherwise do nothing;
	 */
	samp_truth_value_t old_tval = atom_table->assignment[var];
	if (!unfixed_tval(old_tval)) {
		if ((assigned_true(old_tval) && assigned_false(tval))
				|| (assigned_false(old_tval) && assigned_true(tval))) {
			mcsat_err("[set_atom_tval] Assigning a conflict truth value. No model exists.\n");
			return -1;
		} else {
			return 0;
		}
	}
	
	char *var_str = var_string(var, table);
	cprintf(3, "[set_atom_tval] Setting the value of %s to %s\n",
			var_str, string_of_tval(tval));
	free(var_str);

	atom_table->assignment[var] = tval;
	if (fixed_tval(tval)) {
		atom_table->num_unfixed_vars--;
	}
	if (assigned_true(tval)) {
		link_propagate(table, neg_lit(var));
	}
	else {
		link_propagate(table, pos_lit(var));
	}
	assert(valid_table(table));

	/* If the atom is inactive AND the value is non-default, activate the atom. */
	if (lazy_mcsat() && !atom_table->active[var]
			&& assigned_false(tval) == pred_default_value(pred_entry)) {
		activate_atom(table, var);
	}
	assert(valid_table(table));

	samp_truth_value_t new_tval = atom_table->assignment[var];
	assert(new_tval == tval || (fixed_tval(new_tval)
			&& tval == unfix_tval(negate_tval(new_tval))));

	/*
	 * WARNING: in lazy mcsat, when we activate a new clause, it may force the
	 * value of the atom being activated to be the nagation of the value we
	 * intend to assign. E.g., when we want to set p(A) to v_true, and
	 * activated (and kept alive of) the clause ~p(A) or q(B), where q(B) is
	 * fixed to false (either by database or unit propagation), then p(A) has
	 * to change back to v_fixed_false.
	 */
	if (new_tval != tval)
		return -1;
	return 0;
}

/*
 * In unit propagation, fix a literal's value to true
 */
static int32_t fix_lit_true(samp_table_t *table, int32_t lit) {
	int32_t conflict;
	samp_truth_value_t tval;

	if (sign_of_lit(lit)) { // 0: pos; 1: neg
		tval = v_fixed_false;
	} else {
		tval = v_fixed_true;
	}

	conflict = set_atom_tval(var_of(lit), tval, table);

	if (conflict < 0) {
		mcsat_err("[fix_lit_tval] conflict fixing lit %"PRId32" to true.\n", lit);
		return -1;
	}

	//link_propagate(table, not(lit));

	return 0;
}

/*
 * In unit propagation, fix a literal's value to false
 */
static int32_t inline fix_lit_false(samp_table_t *table, int32_t lit) {
	return fix_lit_true(table, not(lit));
}

/*
 * Pushes a clause to a list depending on its evaluation
 *
 * If it is fixed to be satisfied, push to sat_clauses
 * If it is satisfied, push to the corresponding watched list
 * If it is unsatisified, push to unsat_clauses
 */
void insert_live_clause(samp_clause_t *live_clause, samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t i;
	int32_t fixable;
	samp_literal_t lit;
	char *atom_str;

	assert(valid_table(table));
	
	/* See if the clause is fixed-unit propagating */
	fixable = get_fixable_literal(atom_table->assignment, live_clause->disjunct,
			live_clause->numlits);
	if (fixable >= live_clause->numlits) { /* fixed unsat */
		mcsat_err("There is a fixed unsat clause, no model exists.");
		return;
	}
	else if (fixable == -1) { /* more than one unfixed lits */
		i = get_true_lit(atom_table->assignment, live_clause->disjunct,
				live_clause->numlits);
		if (i < live_clause->numlits) { /* currently sat, put to watched list */
			lit = live_clause->disjunct[i];

			clause_list_insert_head(live_clause, &clause_table->watched[lit]);
			assert(assigned_true_lit(atom_table->assignment,
						clause_table->watched[lit].head->link->disjunct[i]));
		} else { /* currently unsat, remains in unsat list */
			clause_list_insert_head(live_clause, &clause_table->unsat_clauses);
		}
	} else { /* we need to fix the truth value of disjunct[fixable] */
		lit = live_clause->disjunct[fixable];

		atom_str = var_string(var_of(lit), table);
		cprintf(2, "[scan_live_clauses] Fixing variable %s\n", atom_str);
		free(atom_str);

		if (unfixed_tval(atom_table->assignment[var_of(lit)])) {
			fix_lit_true(table, lit);
			//if (is_pos(lit)) {
			//	atom_table->assignment[var_of(lit)] = v_fixed_true;
			//} else {
			//	atom_table->assignment[var_of(lit)] = v_fixed_false;
			//}
			//atom_table->num_unfixed_vars--;
		}
		assert(assigned_true_lit(atom_table->assignment, lit));
		//push_integer_stack(lit, &(table->fixable_stack));
		clause_list_insert_head(live_clause, &clause_table->sat_clauses);

		assert(assigned_fixed_true_lit(atom_table->assignment,
					clause_table->sat_clauses.head->link->disjunct[fixable]));
	}
	assert(valid_table(table));
}

/*
 * Insert a new negative or unit clause and fix the value of the atom(s)
 * in the clause.
 */
void insert_negative_unit_clause(samp_clause_t *clause, samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	int32_t i;

	/*
	 * FIXME negative_unit_propagate may call set_atom_tval and activate new
	 * negative or unit clauses and then call this function to fix the 
	 * containing atoms and insert the clauses into negative_unit_clause list.
	 * This may cause an inconsistency of the pointers of the list.
	 */
	if (clause->weight >= 0) {
		/* a unit clause */
		assert(clause->numlits == 1);
		fix_lit_true(table, clause->disjunct[0]);
	} else {
		/* a negative weight clause */
		for (i = 0; i < clause->numlits; i++) {
			samp_literal_t lit = clause->disjunct[i];
			fix_lit_false(table, lit);
		}
	}
	
	clause_list_insert_head(clause, &clause_table->negative_or_unit_clauses);
}

/*
 * Scans the unsat clauses to find those that are sat or to propagate fixed
 * truth values. The propagating clauses are placed on the sat_clauses list,
 * and the propagated literals are placed in fixable_stack so that they will
 * be dealt with by process_fixable_stack later.
 *
 * Returns -1 if a conflict is detected, 0 otherwise.
 */
static int32_t scan_live_clauses(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	samp_clause_t *ptr; /* The list iterator */
	samp_clause_t *cls; /* the current clause */

	for (ptr = clause_table->live_clauses.head; ptr != clause_table->live_clauses.tail;) {
		cls = clause_list_pop(&clause_table->live_clauses, ptr);
		insert_live_clause(cls, table);
	}
	return 0;
}

/*
 * Process link propagation of the fixed variables in a batch. We use a stack
 * to collect the variables being fixed during the scan of the unsat clauses.
 * Only after the scan is complete, do we start to process all the variables.
 * Otherwise link_propagate may introduce new unsat clauses and cause a
 * inconsistency.
 */
static int32_t process_fixable_stack(samp_table_t *table) {
	samp_literal_t lit;
	int32_t conflict = 0;
	while (!empty_integer_stack(&(table->fixable_stack)) && conflict == 0) {
		while (!empty_integer_stack(&(table->fixable_stack)) && conflict == 0) {
			lit = top_integer_stack(&(table->fixable_stack));
			pop_integer_stack(&(table->fixable_stack));
			link_propagate(table, not(lit));
		}
		conflict = scan_live_clauses(table);
	}
	return conflict;
}

/*
 * Fixes the truth values derived from unit and negative weight clauses
 */
static int32_t negative_unit_propagate(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	int32_t i;

	/*
	 * FIXME set_atom_tval may activate new clauses and put them into the list
	 * of negative_or_unit_clauses.
	 */
	samp_clause_t *ptr;
	samp_clause_t *cls;
	for (ptr = clause_table->negative_or_unit_clauses.head;
			ptr != clause_table->negative_or_unit_clauses.tail;
			ptr = next_clause(ptr)) {
		cls = ptr->link;
		if (cls->weight >= 0) {
			/* a unit clause */
			assert(cls->numlits == 1);
			fix_lit_true(table, cls->disjunct[0]);
		} else {
			/* a negative weight clause */
			for (i = 0; i < cls->numlits; i++) {
				samp_literal_t lit = cls->disjunct[i];
				fix_lit_false(table, lit);
			}
		}
	}

	return 0;
}

/*
 * [eager only] Chooses a random unfixed atom in a simmulated annealing
 * step in sample SAT.
 */
static int32_t choose_unfixed_variable(samp_truth_value_t *assignment,
		int32_t num_vars, int32_t num_unfixed_vars) {
	uint32_t var, d, y;

	if (num_unfixed_vars == 0)
		return -1;

	// var = random_uint(num_vars);
	var = genrand_uint(num_vars);
	if (unfixed_tval(assignment[var]))
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

/*
 * Returns the total number of groundings of a predicate
 */
static int32_t pred_cardinality(pred_tbl_t *pred_tbl, sort_table_t *sort_table,
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

/*
 * Returns the total number of groundings of all non-evidence predicate
 */
static int32_t all_atoms_cardinality(pred_tbl_t *pred_tbl, sort_table_t *sort_table) {
	int32_t i;
	int32_t card = 0;
	for (i = 1; i < pred_tbl->num_preds; i++) {
		card += pred_cardinality(pred_tbl, sort_table, i);
	}
	return card;
}

/*
 * [lazy only] Choose a random atom for simulated annealing step in sample SAT.
 * The lazy version of choose_unfixed_variable.  First choose a random atom,
 * regardless whether its value is fixed or not (because we can calculate the
 * total number of atoms and it is convenient to randomly choose one from
 * them). If its value is fixed, we skip this flip using the following
 * statement (return 0).
 */
static int32_t choose_random_atom(samp_table_t *table) {
	uint32_t i, atom_num, anum;
	int32_t card, all_card, acard, pcard, predicate;
	pred_tbl_t *pred_tbl = &table->pred_table.pred_tbl; // Indirect preds
	atom_table_t *atom_table = &table->atom_table;
	sort_table_t *sort_table = &table->sort_table;
	pred_entry_t *pred_entry;

	assert(valid_table(table));

	/* Get the number of possible indirect atoms */
	all_card = all_atoms_cardinality(pred_tbl, sort_table);

	//atom_num = random_uint(all_card);
	atom_num = genrand_uint(all_card);

	predicate = 1; /* Skip past true */
	acard = 0;
	while (true) { /* determine the predicate */
		assert(predicate <= pred_tbl->num_preds);
		pcard = pred_cardinality(pred_tbl, sort_table, predicate);
		if (acard + pcard > atom_num) {
			break;
		}
		acard += pcard;
		predicate++;
	}
	assert(pred_cardinality(pred_tbl, sort_table, predicate) != 0);

	/* gives the position of atom within predicate */
	anum = atom_num - acard; 	

	/*
	 * Now calculate the arguments.  We represent the arguments in
	 * little-endian form
	 */
	pred_entry = &pred_tbl->entries[predicate];
	int32_t *signature = pred_entry->signature;
	int32_t arity = pred_entry->arity;
	atom_buffer_resize(arity);
	int32_t constant;
	samp_atom_t *atom = (samp_atom_t *) atom_buffer.data;
	/* Build atom from atom_num by successive division */
	atom->pred = predicate;
	for (i = 0; i < arity; i++) {
		card = sort_table->entries[signature[i]].cardinality;
		constant = anum % card;
		anum = anum / card;
		sort_entry_t *sort_entry = &sort_table->entries[signature[i]];
		if (sort_entry->constants == NULL) {
			/* Must be an integer */
			if (sort_entry->ints == NULL) {
				atom->args[i] = sort_entry->lower_bound + constant;
			} else {
				atom->args[i] = sort_entry->ints[constant];
			}
		} else {
			atom->args[i] = sort_entry->constants[constant];
			/* Quick typecheck */
			assert(const_sort_index(atom->args[i], &table->const_table) == signature[i]);
		}
	}
	assert(valid_table(table));

	array_hmap_pair_t *atom_map;
	atom_map = array_size_hmap_find(&atom_table->atom_var_hash, arity + 1,
			(int32_t *) atom);

	int32_t atom_index;

	if (atom_map == NULL) {
		/* need to activate atom */
		atom_index = add_internal_atom(table, atom, false);
		atom_map = array_size_hmap_find(&atom_table->atom_var_hash, arity + 1,
				(int32_t *) atom);
		assert(atom_map != NULL);
		activate_atom(table, atom_index);
	}
	else {
		atom_index = atom_map->val;
	}

	return atom_index;
}

/*
 * Computes the cost of flipping an unfixed variable without the actual flip.
 *
 * FIXME in lazy version, we need to consider the clauses being activated (and
 * becoming unsat), therefore the current implementation is an underestimate of
 * the dcost
 */
static void cost_flip_unfixed_variable(samp_table_t *table, int32_t *dcost,
		int32_t var) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_literal_t plit, nlit;
	uint32_t i;

	if (assigned_true(atom_table->assignment[var])) {
		nlit = neg_lit(var);
		plit = pos_lit(var);
	} else {
		nlit = pos_lit(var);
		plit = neg_lit(var);
	}

	*dcost = 0;
	samp_clause_t *ptr;
	samp_clause_t *cls;

	/* add the number of the watched clauses that will be flipped */
	for (ptr = clause_table->watched[plit].head;
			ptr != clause_table->watched[plit].tail;
			ptr = next_clause(ptr)) {
		cls = ptr->link;
		i = 0;
		while (i < cls->numlits
				&& (cls->disjunct[i] == plit
				|| assigned_false_lit(atom_table->assignment, cls->disjunct[i]))) {
			i++;
		}
		if (i == cls->numlits) {
			*dcost += 1;
		}
	}

	/* subtract the number of the unsatisfied clauses that can be flipped */
	for (ptr = clause_table->unsat_clauses.head;
			ptr != clause_table->unsat_clauses.tail;
			ptr = next_clause(ptr)) {
		cls = ptr->link;
		i = 0;
		while (i < cls->numlits && cls->disjunct[i] != nlit) {
			assert(cls->disjunct[i] != plit);
			i++;
		}
		if (i < cls->numlits) {
			*dcost -= 1;
		}
	}
}

/*
 * A temperary stack that store the unfixed variables in a clause;
 * used in choose_clause_var.
 */
static integer_stack_t clause_var_stack = { 0, 0, NULL };

/*
 * Chooses a literal from an unsat clause as a candidate to flip next;
 * makes a random choice with rvar_probability, and otherwise a greedy
 * strategy is used (i.e., choose the literal that minimizes the increase
 * of cost).
 */
static int32_t choose_clause_var(samp_table_t *table, samp_clause_t *clause,
		samp_truth_value_t *assignment, double rvar_probability, int32_t *dcost) {
	uint32_t i, varidx;

	if (clause_var_stack.size == 0) {
		init_integer_stack(&(clause_var_stack), 0);
	} else {
		clear_integer_stack(&clause_var_stack);
	}

	double choice = choose();
	int32_t vcost = 0;
	if (choice < rvar_probability) {//flip a random unfixed variable
		for (i = 0; i < clause->numlits; i++) {
			if (!clause->frozen[i] && unfixed_tval(assignment[var_of(clause->disjunct[i])]))
				push_integer_stack(i, &clause_var_stack);
		} //all unfrozen, unfixed vars are now in clause_var_stack
	} else {
		*dcost = INT32_MAX;
		for (i = 0; i < clause->numlits; i++) {
			if (!clause->frozen[i] && unfixed_tval(assignment[var_of(clause->disjunct[i])])) {
				cost_flip_unfixed_variable(table, &vcost, var_of(clause->disjunct[i]));
				if (*dcost >= vcost) {
					if (*dcost > vcost) {
						*dcost = vcost;
						clear_integer_stack(&clause_var_stack);
					}
					push_integer_stack(i, &clause_var_stack);
				}
			}
		}
	}
	//varidx = random_uint(length_integer_stack(&clause_var_stack));
	varidx = genrand_uint(length_integer_stack(&clause_var_stack));
	cost_flip_unfixed_variable(table, dcost, var_of(clause->disjunct[varidx]));

	assert(varidx < length_integer_stack(&clause_var_stack));
	return var_of(clause->disjunct[nth_integer_stack(varidx, &clause_var_stack)]);
}

/*
 * Executes a variable flip by first scanning all the previously sat clauses
 * in the watched list, and then moving any previously unsat clauses to the
 * sat/watched list.
 */
static int32_t flip_unfixed_variable(samp_table_t *table, int32_t var) {
	//  double dcost = 0;   //dcost seems unnecessary
	atom_table_t *atom_table = &table->atom_table;
	cprintf(4, "[flip_unfixed_variable] Flipping variable %"PRId32" to %s\n", var,
			assigned_true(atom_table->assignment[var]) ? "false" : "true");

	assert(valid_table(table));

	if (assigned_true(atom_table->assignment[var])) {
		if (set_atom_tval(var, v_false, table) >= 0) {
			//link_propagate(table, pos_lit(var));
		}
	} else {
		if (set_atom_tval(var, v_true, table) >= 0) {
			//link_propagate(table, neg_lit(var));
		}
	}
	if (scan_live_clauses(table) == -1) {
		return -1;
	}
	if (process_fixable_stack(table) == -1) {
		return -1;
	}
	assert(valid_table(table));

	return 0;
}

/*
 * Assigns random truth values to unfixed vars and returns number of unfixed
 * vars (num_unfixed_vars).
 */
static void init_random_assignment(samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	uint32_t i;

	cprintf(3, "[init_random_assignment] num_vars = %d\n", atom_table->num_vars);
	for (i = 0; i < atom_table->num_vars; i++) {
		if (unfixed_tval(atom_table->assignment[i])) {
			if (choose() < 0.5) {
				set_atom_tval(i, v_false, table);
			} else {
				set_atom_tval(i, v_true, table);
			}
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
static int32_t init_first_sample_sat(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	int32_t conflict = 0;

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] Started ...\n");
		print_assignment(table);
		print_clause_table(table);
	}

	conflict = negative_unit_propagate(table);
	if (conflict == -1)
		return -1;

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] After negative_unit_propagation:\n");
		print_assignment(table);
		print_clause_table(table);
	}

	init_random_assignment(table);

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] After init_random_assignment:\n");
		print_assignment(table);
		print_clause_table(table);
	}

	if (scan_live_clauses(table) == -1) {
		return -1;
	}
	if (process_fixable_stack(table) == -1) {
		return -1;
	}
	
	if (get_verbosity_level() >= 3) {
		printf("[init_first_sample_sat] After randomization, unsat = %d:\n",
				clause_table->unsat_clauses.length);
		print_assignment(table);
		print_clause_table(table);
	}

	return 0;
}

/*
 * The initialization phase for the mcmcsat step. First place all the clauses
 * in the unsat list, and then use scan_live_clauses to move them into
 * sat_clauses or the watched (sat) lists.
 *
 * Like init_first_sample_sat, but takes an existing state and sets it up for a
 * second round of sampling
 */
static int32_t init_sample_sat(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;

	/*
	 * The current assignment is kept as a backup in case sample_sat fails.
	 * Using the assignment from the previous sample_sat guarantees no new
	 * clauses need to be activated BEFORE we run the unit propagation. Using
	 * default value of each predicate should also be fine, since the initial
	 * unsat clauses have been activated already at the beginning of MCSAT.
	 */
	switch_and_copy_assignment_array(atom_table);
	assert(valid_table(table));

	uint32_t i;
	for (i = 0; i < atom_table->num_vars; i++) {
		if (fixed_tval(atom_table->assignment[i])) { 
			atom_table->assignment[i] = unfix_tval(atom_table->assignment[i]);
			atom_table->num_unfixed_vars++;
		}
	}
	move_sat_to_live_clauses(clause_table);
	assert(valid_table(table));

	/*
	 * Fix some variable by unit propagation
	 */
	if (negative_unit_propagate(table) == -1)
		return -1;
	assert(valid_table(table));

	init_random_assignment(table);
	assert(valid_table(table));

	if (scan_live_clauses(table) == -1)
		return -1;
	if (process_fixable_stack(table) == -1)
		return -1;
	
	if (get_verbosity_level() >= 3) {
		printf("[init_sample_sat] After randomization, unsat = %d:\n",
				clause_table->unsat_clauses.length);
		print_assignment(table);
		print_clause_table(table);
	}

	return 0;
}

/*
 * A step of sample sat, in which the value of a random variable is flipped
 *
 * @lazy: true if inference is lazy
 * @sa_probability: the probability of choosing a simulated annealing step,
 * otherwise choose a walksat step
 * @sa_temperature: (fixed) temperature of simulated annealing
 * @rvar_probability: probability of selecting a random variable in a walksat
 * step, otherwise select a variable that minimizes dcost
 */
static int32_t sample_sat_body(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);
	int32_t dcost;
	int32_t var;
	int32_t conflict = 0;

	double choice = choose();
	cprintf(3, "\n[sample_sat_body] num_unsat = %d, choice = % .4f, sa_probability = % .4f\n",
			clause_table->unsat_clauses.length, choice, sa_probability);

	assert(clause_table->unsat_clauses.length >= 0);
	/* Simulated annlealing vs. walksat */
	if (clause_table->unsat_clauses.length == 0 || choice < sa_probability) {
		/* Simulated annealing step */
		cprintf(3, "[sample_sat_body] simulated annealing\n");

		if (!lazy) {
			var = choose_unfixed_variable(atom_table->assignment, atom_table->num_vars,
					atom_table->num_unfixed_vars);
		} else {
			var = choose_random_atom(table);
		}

		if (var == -1 || !unfixed_tval(atom_table->assignment[var])) {
			return 0;
		}
		
		cost_flip_unfixed_variable(table, &dcost, var);
		cprintf(3, "[sample_sat_body] simulated annealing num_unsat = %d, var = %d, dcost = %d\n",
				clause_table->unsat_clauses.length, var, dcost);
		if (dcost <= 0) {
			conflict = flip_unfixed_variable(table, var);
		} else {
			choice = choose();
			if (choice < exp(-dcost / sa_temperature)) {
				conflict = flip_unfixed_variable(table, var);
			}
		}
	} else {
		/* Walksat step */
		cprintf(3, "[sample_sat_body] WalkSAT\n");

		/* Choose a random unsat clause */
		samp_clause_t *unsat_clause = choose_random_clause(&clause_table->unsat_clauses);

		/* Choose an atom from the clause, with two strategies: random and greedy */
		var = choose_clause_var(table, unsat_clause, atom_table->assignment,
				rvar_probability, &dcost);

		if (get_verbosity_level() >= 3) {
			printf("[sample_sat_body] WalkSAT num_unsat = %d, var = %d, dcost = %d\n",
					clause_table->unsat_clauses.length, var, dcost);
			printf("from unsat clause ");
			print_clause(unsat_clause, table);
			printf("\n");
		}
		conflict = flip_unfixed_variable(table, var);
	}
	return conflict;
}

/*
 * A SAT solver for only the hard formulas
 *
 * TODO: To be replaced by other SAT solvers. It should just give a
 * solution but not necessarily uniformly drawn.
 */
int32_t first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability, uint32_t max_flips) {
	clause_table_t *clause_table = &table->clause_table;
	int32_t conflict;

	conflict = init_first_sample_sat(table);

	if (conflict == -1)
		return -1;

	uint32_t num_flips = max_flips;
	while (clause_table->unsat_clauses.length > 0 && num_flips > 0) {
		if (sample_sat_body(table, lazy, sa_probability, sa_temperature,
					rvar_probability) == -1)
			return -1;
		num_flips--;
	}

	if (clause_table->unsat_clauses.length > 0) {
		printf("num of unsat clauses = %d\n", clause_table->unsat_clauses.length);
		mcsat_err("Initialization failed to find a model; increase max_flips\n");
		return -1;
	}

	if (get_verbosity_level() >= 1) {
		printf("\n[first_sample_sat] initial assignment:\n");
		print_assignment(table);
		print_clause_table(table);
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
		double sa_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips) {
	clause_table_t *clause_table = &table->clause_table;
	int32_t conflict;

	cprintf(2, "[sample_sat] started ...\n");

	conflict = init_sample_sat(table);

	uint32_t num_flips = max_flips;
	while (num_flips > 0 && conflict == 0) {
		if (clause_table->unsat_clauses.length == 0) {
			if (max_extra_flips <= 0) {
				break;
			} else {
				max_extra_flips--;
			}
		}
		conflict = sample_sat_body(table, lazy, sa_probability, sa_temperature,
				rvar_probability);
		//assert(valid_table(table));
		num_flips--;
	}

	/* If we successfully get a model, try to walk around to neighboring models;
	 * Note that if the model is isolated, it will remain unchanged. */
	if (conflict != -1 && clause_table->unsat_clauses.length == 0) {
		num_flips = genrand_uint(max_extra_flips);
		while (num_flips > 0 && conflict == 0) {
			conflict = sample_sat_body(table, lazy, 1, DBL_MAX, rvar_probability);
			//assert(valid_table(table));
			num_flips--;
		}
	}
}

