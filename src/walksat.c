#include <float.h>
#include <time.h>

#include "tables.h"
#include "memalloc.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "hash_map.h"
#include "clause_list.h"
#include "gcd.h"
#include "utils.h"
#include "print.h"
#include "vectors.h"
#include "buffer.h"
#include "yacc.tab.h"
#include "ground.h"
#include "samplesat.h"

#include "walksat.h"

/*
 * Checks the satisfiability of a clause
 *
 * if fixable, i.e., a fixed true literal or a unique non-fixed literal exists,
 * returns the index of the literal;
 * if unfixable, i.e., has at least two unfixed literals, returns -1;
 * if fixed unsat, returns -2.
 */
static int32_t get_fixable_literal(
		samp_truth_value_t *atom_assignment,
		samp_truth_value_t *rule_assignment, 
		samp_clause_t *clause) {
	samp_literal_t *disjunct = clause->disjunct;
	int32_t num_lits = clause->num_lits;
	int32_t i, j;

	/* case 0: fixed sat */
	/* if a rule is fixed false, its clauses should not be in the clause lists */
	assert(!assigned_fixed_false(rule_assignment[clause->rule_index]));
	for (i = 0; i < num_lits; i++) {
		if (assigned_fixed_true_lit(atom_assignment, disjunct[i]))
			return i;
	}

	for (i = 0; i < num_lits; i++) {
		if (!assigned_fixed_false_lit(atom_assignment, disjunct[i]))
			break;
	}
	if (i == num_lits) {
		if (assigned_fixed_true(rule_assignment[clause->rule_index]))
			return -2; /* case 4: fixed unsat */
		else
			return num_lits; /* the rule delegate literal is fixable */ 
	}

	/* i is the first unfixed literal, now check if there is a second one */
	for (j = i + 1; j < num_lits; j++) {
		if (!assigned_fixed_false_lit(atom_assignment, disjunct[j]))
			break;
	}

	/* there are at least two unfixed literals */
	if (j < num_lits) {
		return -1; 
	} else if (!fixed_tval(rule_assignment[clause->rule_index])) {
		return -1;
	} else { /* i is uniquely fixable */
		return i; 
	}
}

/*
 * Returns: the index of a satisfied literal of a clause, or -1 if none exists.
 */
static int32_t get_true_literal(
		samp_truth_value_t *atom_assignment,
		samp_truth_value_t *rule_assignment,
		samp_clause_t *clause) {
	int32_t i;
	for (i = 0; i < clause->num_lits; i++) {
		if (assigned_true_lit(atom_assignment, clause->disjunct[i]))
			return i;
	}
	if (assigned_false(rule_assignment[clause->rule_index]))
		return clause->num_lits;
	return -1;
}

/*
 * Propagates a truth assignment on a link of watched clauses for a literal
 * that has been assigned false. All the literals are assumed to have truth
 * values. The watched literal points to a true literal if there is one in
 * the clause. Whenever a watched literal is assigned false, the
 * propagation must find a new true literal, or retain the existing one. If
 * a clause is falsified, its weight is added to the delta cost. The
 * clauses containing the negation of the literal must be reevaluated and
 * assigned true if previously false.
 */
static void link_propagate(samp_table_t *table, samp_literal_t lit) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
#ifndef NDEBUG
	atom_table_t *atom_table = &table->atom_table;
#endif	

	/* the assignment of the lit has just changed to false */
	assert(assigned_false_lit(atom_table->assignment, lit));

	/* since the assignment of the lit was true, the negate of the lit
	 * should have an empty watched list */
	assert(is_empty_clause_list(&rule_inst_table->watched[not(lit)]));

	/* watched list of lit needs to be rescaned */
	clause_list_concat(&rule_inst_table->watched[lit], &rule_inst_table->live_clauses);

	/* unsat clause list is dirty, need to rescan */
	clause_list_concat(&rule_inst_table->unsat_clauses, &rule_inst_table->live_clauses);
}

/*
 * Try to set the value of an atom.
 *
 * If the atom has a non-fixed value and is set to a fixed value, run
 * unit_propagation...;
 * If the atom has a non-fixed value and is set to the opposite non-fixed value,
 * just change the value (and change the state of the relavent clauses?)
 */
static int32_t update_atom_tval(int32_t var, samp_truth_value_t tval, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_atom_t *atom = atom_table->atom[var];
	pred_entry_t *pred_entry = get_pred_entry(pred_table, atom->pred);

	/*
	 * Case 0: If the atom has been fixed, check if consistent: if it is
	 * assigned to the opposite value, return inconsistency; otherwise do
	 * nothing;
	 */
	samp_truth_value_t old_tval = atom_table->assignment[var];
	if (!unfixed_tval(old_tval)) {
		if ((assigned_true(old_tval) && assigned_false(tval))
				|| (assigned_false(old_tval) && assigned_true(tval))) {
			mcsat_err("[update_atom_tval] Assigning a conflict truth value. No model exists.\n");
			return -1;
		} else {
			return 0;
		}
	}

#if USE_PTHREADS
        /* Internally, this is now theoretically protected with a
           mutex, but do we really want a multithreaded mcmc to be
           waiting for mutexes? */
#else
        /* Something's still bad here.  For now, we ignore this code
           block if we are compiled for pthreads */
	char *var_str = var_string(var, table);
	cprintf(3, "[update_atom_tval] Setting the value of %s to %s\n",
			var_str, samp_truth_value_string(tval));
	free(var_str);
#endif

	/* Not case 0: update the value */
	atom_table->assignment[var] = tval;
	if (fixed_tval(tval)) {
          /* If fixed, this can break a later assertion that there are
             no fixed vars. Why? */
		atom_table->num_unfixed_vars--;
	}

	/* Case 1: If the value just gets fixed but not changed, we are done. */
	if (old_tval == unfix_tval(tval)) {
		return 0;
	}

	/* Case 2: the value has changed */
	if (assigned_true(tval)) {
		link_propagate(table, neg_lit(var));
	} else {
		link_propagate(table, pos_lit(var));
	}
	//assert(valid_table(table));

	/* If the atom is inactive AND the value is non-default, activate the atom. */
	if (lazy_mcsat() && !atom_table->active[var]
			&& assigned_false(tval) == pred_default_value(pred_entry)) {
		activate_atom(table, var);
	}
	//assert(valid_table(table));

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
static int32_t inline fix_lit_true(samp_table_t *table, int32_t lit) {
	return update_atom_tval(var_of(lit), 
			sign_of_lit(lit) ? v_up_false : v_up_true, 
			table);
}

/*
 * In unit propagation, fix a literal's value to false
 */
static int32_t inline fix_lit_false(samp_table_t *table, int32_t lit) {
	return fix_lit_true(table, not(lit));
}

int32_t flip_unfixed_variable(samp_table_t *table, int32_t var) {
	// double dcost = 0; //dcost seems unnecessary
	atom_table_t *atom_table = &table->atom_table;
	cprintf(4, "[flip_unfixed_variable] Flipping variable %"PRId32" to %s\n", var,
			assigned_true(atom_table->assignment[var]) ? "false" : "true");
	assert(valid_table(table));

	if (assigned_true(atom_table->assignment[var])) {
		update_atom_tval(var, v_false, table);
	} else {
		update_atom_tval(var, v_true, table);
	}

	scan_live_clauses(table); 
	assert(valid_table(table));

	return 0;
}

/*
 * Pushes a clause to a list depending on its evaluation
 *
 * If it is fixed to be satisfied, push to sat_clauses
 * If it is satisfied, push to the corresponding watched list
 * If it is unsatisified, push to unsat_clauses
 */
void insert_live_clause(samp_clause_t *clause, samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t i, fixable;
	samp_literal_t lit;

	/* See if the clause is fixed-unit propagating */
	fixable = get_fixable_literal(atom_table->assignment, 
			rule_inst_table->assignment, clause);
	if (fixable == -2) { 
		/* fixed unsat */
		mcsat_err("There is a fixed unsat clause, no model exists.");
		return;
	}
	else if (fixable == -1) { /* more than one unfixed lits */
		i = get_true_literal(atom_table->assignment,
				rule_inst_table->assignment, clause);
		if (i < 0) { 
			/* currently unsat, put back to the unsat list */
			clause_list_insert_head(clause, 
					&rule_inst_table->unsat_clauses);
		} else if (i == clause->num_lits) {
			/* currently sat, put to the rule_watched list */
			clause_list_insert_head(clause, 
					&rule_inst_table->rule_watched[clause->rule_index]);
		} else {
			/* currently sat, put to the watched list */
			clause_list_insert_head(clause, 
					&rule_inst_table->watched[clause->disjunct[i]]);
		}
	} else { 
		/* fix a soft rule to unsat, because one of its clauses is fixed unsat */
		if (fixable == clause->num_lits) {
			rule_inst_t *rinst = rule_inst_table->rule_insts[clause->rule_index];
			if (assigned_true(rule_inst_table->assignment[clause->rule_index])) {
				rule_inst_table->unsat_weight += rinst->weight;
			} else {
				rule_inst_table->sat_weight   += rinst->weight;
                        }
			if (get_verbosity_level() >= 3) {
				printf("[insert_live_clause] Fix rule %d: \n", clause->rule_index);
				print_rule_instance(rinst, table);
				printf(" to false\n");
			}
			rule_inst_table->assignment[clause->rule_index] = v_up_false;
		} else {
			/* fixed sat if we fix the 'fixable' literal */
			lit = clause->disjunct[fixable];
			if (unfixed_tval(atom_table->assignment[var_of(lit)])) {
				/* unit propagation */
				fix_lit_true(table, lit);
			} else {
				// already fixed sat
			}
			assert(assigned_fixed_true_lit(atom_table->assignment, lit));
		}
		clause_list_insert_head(clause, &rule_inst_table->sat_clauses);
	}
}

/*
 * Fixes the truth values derived from unit and negative weight clauses
 *
 * Return the number of atoms that have been fixed. The process will be
 * repeated until no new atoms are fixed.
 *
 * FIXME This seems to be very similar to rescan_live_clauses, it can
 * be replaced by rescan_live_clauses
 */
// static int32_t unit_propagate(samp_table_t *table) {
// 	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
// 	atom_table_t *atom_table = &table->atom_table;
// 	int32_t num_unfixed = 0;

// 	samp_clause_t *cls;
// 	while (num_unfixed < rule_inst_table->live_clauses.length) {
// 		cls = clause_list_pop_head(&rule_inst_table->live_clauses);

// 		int32_t fixable = get_fixable_literal(atom_table->assignment,
// 				rule_inst_table->assignment, cls);

// 		if (fixable == cls->num_lits) {
// 			mcsat_err("There is a fixed unsat clause, no model exists.");
// 			return -1;
// 		}
// 		if (fixable == -2) {
// 			num_unfixed++;
// 			clause_list_insert_tail(cls, &rule_inst_table->live_clauses);
// 		} else {
// 			num_unfixed = 0;
// 			clause_list_insert_tail(cls, &rule_inst_table->sat_clauses);
// 			samp_literal_t lit = cls->disjunct[fixable];
// 			if (unfixed_tval(atom_table->assignment[var_of(lit)])) {
// 				fix_lit_true(table, lit);
// 			}
// 			assert(assigned_fixed_true_lit(atom_table->assignment, lit));
// 		}
// 	}
	
// 	return 0;
// }

void init_live_clauses(samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;

	int32_t i, j;
	for (i = 0; i < atom_table->num_vars; i++) {
		empty_clause_list(&rule_inst_table->watched[pos_lit(i)]);
		empty_clause_list(&rule_inst_table->watched[neg_lit(i)]);
	}
	for (i = 0; i < rule_inst_table->num_rule_insts; i++)
		empty_clause_list(&rule_inst_table->rule_watched[i]);
	empty_clause_list(&rule_inst_table->sat_clauses);
	empty_clause_list(&rule_inst_table->unsat_clauses);
	empty_clause_list(&rule_inst_table->live_clauses);

	for (i = 0; i < rule_inst_table->num_rule_insts; i++) {
		if (assigned_true(rule_inst_table->assignment[i])) {
			rule_inst_t *rinst = rule_inst_table->rule_insts[i];
			for (j = 0; j < rinst->num_clauses; j++) {
				clause_list_insert_tail(rinst->conjunct[j], &rule_inst_table->live_clauses);
			}
		}
	}
}

void scan_live_clauses(samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	samp_clause_t *cls;
	while (rule_inst_table->live_clauses.length > 0) {
		cls = clause_list_pop_head(&rule_inst_table->live_clauses);
		insert_live_clause(cls, table);
	}
}

void init_random_assignment(samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	uint32_t i;

	cprintf(3, "[init_random_assignment] num_vars = %d\n", atom_table->num_vars);
	for (i = 0; i < atom_table->num_vars; i++) {
		if (!db_tval(atom_table->assignment[i])
				&& !fixed_tval(atom_table->assignment[i])) {
			if (choose() < 0.5) {
				update_atom_tval(i, v_false, table);
			} else {
				update_atom_tval(i, v_true, table);
			}
		}
	}
}

int32_t choose_unfixed_variable(atom_table_t *atom_table) {
	samp_truth_value_t *assignment = atom_table->assignment;
	int32_t num_vars = atom_table->num_vars;
	int32_t num_unfixed_vars = atom_table->num_unfixed_vars;

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
	} while (!unfixed_tval(assignment[y]));
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
 * The lazy version of choose_unfixed_variable. First choose a random atom,
 * regardless whether its value is fixed or not (because we can calculate the
 * total number of atoms and it is convenient to randomly choose one from
 * them). If its value is fixed, we skip this flip using the following
 * statement (return 0).
 */
int32_t choose_random_atom(samp_table_t *table) {
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
	 * Now calculate the arguments. We represent the arguments in
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
 * FIXME in lazy version, we need to consider the clauses that would be
 * activated and becoming unsat if the variable is flipped. The dcost computed
 * by the current code is an underestimate
 */
void cost_flip_unfixed_variable(samp_table_t *table, int32_t var, 
		int32_t *dcost) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
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
	for (ptr = rule_inst_table->watched[plit].head;
			ptr != rule_inst_table->watched[plit].tail;
			ptr = next_clause_ptr(ptr)) {
		cls = ptr->link;
		i = 0;
		while (i < cls->num_lits
				&& (cls->disjunct[i] == plit
				|| assigned_false_lit(atom_table->assignment, cls->disjunct[i]))) {
			i++;
		}
		if (i == cls->num_lits) {
			*dcost += 1;
		}
	}

	/* subtract the number of the unsatisfied clauses that can be flipped */
	for (ptr = rule_inst_table->unsat_clauses.head;
			ptr != rule_inst_table->unsat_clauses.tail;
			ptr = next_clause_ptr(ptr)) {
		cls = ptr->link;
		i = 0;
		while (i < cls->num_lits && cls->disjunct[i] != nlit) {
			assert(cls->disjunct[i] != plit);
			i++;
		}
		if (i < cls->num_lits) {
			*dcost -= 1;
		}
	}
}

void cost_flip_unfixed_rule(samp_table_t *table, int32_t rule_index,
		int32_t *dcost) {
	atom_table_t *atom_table = &table->atom_table;
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	rule_inst_t *rinst = rule_inst_table->rule_insts[rule_index];
	*dcost = 0;
	int32_t i;
	for (i = 0; i < rinst->num_clauses; i++) {
		if (eval_clause(atom_table->assignment, rinst->conjunct[i]) == -1)
			(*dcost)--;
	}
}

/*
 * A temperary stack that store the unfixed variables in a clause;
 * used in choose_clause_var.
 *
 */
//static integer_stack_t clause_var_stack = { 0, 0, NULL };

int32_t choose_clause_var(samp_table_t *table, samp_clause_t *clause,
		double rvar_probability) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;
	integer_stack_t *clause_var_stack = &table->clause_var_stack;
	uint32_t i, sidx, vidx;

	if (clause_var_stack->size == 0) {
		init_integer_stack(clause_var_stack, 0);
	} else {
		clear_integer_stack(clause_var_stack);
	}

	double choice = choose();
	if (choice < rvar_probability) { /* flip a random unfixed variable */
		for (i = 0; i < clause->num_lits; i++) {
			if (unfixed_tval(atom_table->assignment[var_of(clause->disjunct[i])]))
				push_integer_stack(i, clause_var_stack);
		} 
		if (unfixed_tval(rule_inst_table->assignment[clause->rule_index]))
			push_integer_stack(clause->num_lits, clause_var_stack);
		/* all unfixed vars are now in clause_var_stack */
	} else {
		int32_t dcost = INT32_MAX, vcost = 0;
		for (i = 0; i < clause->num_lits; i++) {
			if (unfixed_tval(atom_table->assignment[var_of(clause->disjunct[i])])) {
				cost_flip_unfixed_variable(table, var_of(clause->disjunct[i]), &vcost);
				if (dcost >= vcost) {
					if (dcost > vcost) {
						dcost = vcost;
						clear_integer_stack(clause_var_stack);
					}
					push_integer_stack(i, clause_var_stack);
				}
			}
		}
		if (unfixed_tval(rule_inst_table->assignment[clause->rule_index])) {
			cost_flip_unfixed_rule(table, clause->rule_index, &vcost);
			if (dcost > vcost) {
				dcost = vcost;
				clear_integer_stack(clause_var_stack);
				push_integer_stack(clause->num_lits, clause_var_stack);
			}
		}
	}
	//sidx = random_uint(length_integer_stack(clause_var_stack));
	sidx = genrand_uint(length_integer_stack(clause_var_stack));
	vidx = nth_integer_stack(sidx, clause_var_stack);

	return vidx;
}

/* MaxWalkSAT */
void mw_sat(samp_table_t *table, int32_t num_trials, double rvar_probability,
		int32_t max_flips, uint32_t timeout) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t i, j;
	time_t fintime;

	if (timeout != 0) {
		fintime = time(NULL) + timeout;
	}

	double best_cost = DBL_MAX;
	for (i = 0; i < num_trials; i++) {
		init_random_assignment(table);
		//scan_rule_instances(table);
		for (j = 0; j < rule_inst_table->num_rule_insts; j++) {
			if (rule_inst_table->rule_insts[j]->weight == DBL_MAX) 
				rule_inst_table->assignment[j] = v_up_true;
			else
				rule_inst_table->assignment[j] = v_true;
		}
		init_live_clauses(table);
		scan_live_clauses(table);

		if (get_verbosity_level() >= 3) {
			printf("\n[mw_sat] Trial %d: \n", i);
			print_live_clauses(table);
		}

		for (j = 0; j < max_flips; j++) {

			if (timeout != 0 && time(NULL) >= fintime) {
				printf("Timeout after %"PRIu32" samples\n", i);
				break;
			}

			if (rule_inst_table->unsat_clauses.length == 0 
					&& rule_inst_table->unsat_soft_rules.nelems == 0) {
				return;
			}
			int32_t c = genrand_uint(rule_inst_table->unsat_clauses.length
					+ rule_inst_table->unsat_soft_rules.nelems);

			if (c < rule_inst_table->unsat_clauses.length) { /* picked a hard clause */
				samp_clause_t *clause = choose_random_clause(&rule_inst_table->unsat_clauses);
				int32_t litidx = choose_clause_var(table, clause, rvar_probability);
				
				if (litidx == clause->num_lits) {
					if (get_verbosity_level() >= 3) {
						printf("[flip_rule_to_unsat] Rule %d: ", clause->rule_index);
						print_rule_instance(rule_inst_table->rule_insts[clause->rule_index], table);
						printf("\n");
					}
					/* picked the rule auxiliary variable, i.e., rinst T -> F */
					hmap_get(&rule_inst_table->unsat_soft_rules, clause->rule_index);
					rule_inst_table->assignment[clause->rule_index] = v_false;
					rule_inst_t *rinst = rule_inst_table->rule_insts[clause->rule_index];
					rule_inst_table->unsat_weight += rinst->weight;
                                        /* Is this really going to do the right thing?? */
					rule_inst_table->sat_weight   -= rinst->weight;
					/* move unsat_clauses to live_clause_list and rescan */
					clause_list_concat(&rule_inst_table->unsat_clauses, 
							&rule_inst_table->live_clauses);
					scan_live_clauses(table);
				} else { 
					flip_unfixed_variable(table, var_of(clause->disjunct[litidx]));
				}
			} else {
				/* picked a soft rule auxiliary variable, i.e., rinst F -> T */
				int32_t rule_index = hmap_remove_random(&rule_inst_table->unsat_soft_rules);
				rule_inst_table->assignment[rule_index] = v_true;
				rule_inst_t *rinst = rule_inst_table->rule_insts[rule_index];
				rule_inst_table->unsat_weight -= rinst->weight;
				rule_inst_table->sat_weight   += rinst->weight;
				if (get_verbosity_level() >= 3) {
					printf("[flip_rule_to_sat] Rule %d: ", rule_index);
					print_rule_instance(rinst, table);
					printf("\n");
				}
				/* move watch[~r] to live_clause_list and rescan */
				clause_list_concat(&rule_inst_table->rule_watched[rule_index],
						&rule_inst_table->live_clauses);
				scan_live_clauses(table);
			}

			if (get_verbosity_level() >= 1) {
				printf("[mw_sat] Flip %d: # unsat hard = %d, # unsat soft = %d, " 
						"weight of unsat soft = %f; weight of sat soft = %f\n", j,
						rule_inst_table->unsat_clauses.length,
						rule_inst_table->unsat_soft_rules.nelems,
                                                rule_inst_table->unsat_weight,
                                                rule_inst_table->sat_weight
                                       );
			}
			if (get_verbosity_level() >= 3) {
				print_live_clauses(table);
			}

			if (best_cost > rule_inst_table->unsat_weight) {
				best_cost = rule_inst_table->unsat_weight;
				copy_assignment_array(atom_table);
			}
		}
	}
	restore_assignment_array(atom_table);
}

