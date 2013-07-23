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
#include "mcmc.h"
#include "lazysat.h"

/*
 * TODO: can be replaced by literal_falsifiable
 *
 * Evaluates a builtin literal (a builtin operation with an optional negation)
 */
bool builtin_inst_p(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t j, arity, *nargs, argidx;
	bool builtin_result, retval;

	assert(lit->atom->builtinop != 0);
	arity = atom_arity(lit->atom, pred_table);
	nargs = (int32_t *) safe_malloc(arity * sizeof(int32_t));
	for (j = 0; j < arity; j++) {
		argidx = lit->atom->args[j].value;
		if (lit->atom->args[j].kind == variable) {
			nargs[j] = substs[argidx];
		} else {
			nargs[j] = argidx;
		}
	}
	builtin_result = call_builtin(lit->atom->builtinop, arity, nargs);
	retval = builtin_result ? (lit->neg ? false : true) : (lit->neg ? true : false);
	safe_free(nargs);
	return retval;
}

extern clause_buffer_t rule_atom_buffer;

/* Tests whether a literal is falsifiable, i.e., is not fixed to be satisfied */
bool literal_falsifiable(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	rule_atom_t *atom = lit->atom;
	bool neg = lit->neg;
	int32_t predicate = atom->pred;
	int32_t arity = atom_arity(atom, pred_table);
	int32_t i;

	atom_buffer_resize(&rule_atom_buffer, arity);
	samp_atom_t *rule_inst_atom = (samp_atom_t *) rule_atom_buffer.data;
	rule_inst_atom->pred = predicate;
	for (i = 0; i < arity; i++) { //copy each instantiated argument
		if (atom->args[i].kind == variable) {
			rule_inst_atom->args[i] = substs[atom->args[i].value];
		} else {
			rule_inst_atom->args[i] = atom->args[i].value;
		}
	}

	bool value;
	if (atom->builtinop > 0) {
		value = call_builtin(atom->builtinop, builtin_arity(atom->builtinop),
				rule_inst_atom->args);
	}
	else {
		array_hmap_pair_t *atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
				arity + 1, (int32_t *) rule_inst_atom);
		if (atom_map == NULL || !atom_table->active[atom_map->val]) { // if inactive
			value = rule_atom_default_value(atom, pred_table);
		}
		else if (predicate <= 0) { // for evidence predicate
			value = assigned_true(assignment[atom_map->val]);
		} else { // non-evidence predicate has unfixed value;
			return true;
		}
	}
	return (neg == value); // if the literal is unsat
}

/* Tests whether a clause is falsifiable, i.e., is not fixed to be satisfied */
bool check_clause_falsifiable(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t i;
	char *atom_str;

	for (i = 0; i < rule->num_lits; i++) { //for each literal
		rule_literal_t *lit = rule->literals[i];
		bool value = literal_falsifiable(lit, substs, table);
		if (!value) {
			samp_atom_t *atom = rule_atom_to_samp_atom(lit->atom, substs, pred_table);
			if (atom != NULL) {
				atom_str = atom_string(atom, table);
				cprintf(5, "%s not falsifiable\n", atom_str);
				free(atom_str);
			}
			free(atom);
			return false;
		}
	}
	return true;
}

/* Checks whether a clause has been instantiated already */
bool check_clause_duplicate(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table) {
	array_hmap_pair_t *rule_subst_map = array_size_hmap_find(&rule->subst_hash,
			rule->num_vars, (int32_t *) substs);
	if (rule_subst_map != NULL) {
		return false;
	}

	int32_t *new_substs = (int32_t *) clone_memory(substs, sizeof(int32_t) * rule->num_vars);
	/* insert the subst to the hash set */
	rule_subst_map = array_size_hmap_get(&rule->subst_hash, rule->num_vars, 
			(int32_t *) new_substs);
	return true;
}

/*
 * Given a rule, a -1 terminated array of constants corresponding to the
 * variables of the rule, and a samp_table, returns the substituted clause.
 * Checks that the constants array is the right length, and the sorts match.
 * Returns -1 if there is a problem.
 */
int32_t substit_rule(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table) {
	const_table_t *const_table = &table->const_table;
	sort_table_t *sort_table = &table->sort_table;
	pred_table_t *pred_table = &table->pred_table;
	sort_entry_t *sort_entry;
	int32_t vsort, csort, csubst;
	// array_hmap_pair_t *atom_map;
	int32_t i, j, litidx;
	bool found_indirect;

	/* check if the constants are compatible with the sorts */
	for (i = 0; i < rule->num_vars; i++) {
		csubst = substs[i];
		assert(csubst != INT32_MIN);
		//if (csubst == INT32_MIN) {
		//	mcsat_err("substit: Not enough constants - %"PRId32" given, %"PRId32" required\n",
		//			i, rule->num_vars);
		//	return -1;
		//}
		vsort = rule->vars[i]->sort_index;
		sort_entry = &sort_table->entries[vsort];
		if (sort_entry->constants == NULL) {
			// It's an integer; check that its value is within the sort
			if (!(sort_entry->lower_bound <= csubst
						&& csubst <= sort_entry->upper_bound)) {
				mcsat_err("substit: integer is out of bounds %"PRId32"\n", i);
				return -1;
			}
		} else {
			csort = const_sort_index(substs[i], const_table);
			if (!subsort_p(csort, vsort, sort_table)) {
				mcsat_err("substit: Constant/variable sorts do not match at %"PRId32"\n", i);
				return -1;
			}
		}
	}
	assert(substs == NULL || substs[i] == INT32_MIN);
	//if (substs != NULL && substs[i] != INT32_MIN) {
	//	mcsat_err("substit: Too many constants - %"PRId32" given, %"PRId32" required\n",
	//			i, rule->num_vars);
	//	return -1;
	//}

	/* check if the value is fixed to be true, if so discard it */
	if (!check_clause_falsifiable(rule, substs, table)) {
		return -1;
	}

	/* check duplicates and insert to hashset */
	if (!check_clause_duplicate(rule, substs, table)) {
		return -1;
	}

	// Everything is OK, do the substitution
	// We just use the clause_buffer - first make sure it's big enough
	clause_buffer_resize(rule->num_lits);
	litidx = 0;
	found_indirect = false;
	for (i = 0; i < rule->num_lits; i++) {
		rule_literal_t *lit = rule->literals[i];
		int32_t arity = atom_arity(lit->atom, pred_table);
		if (lit->atom->builtinop != 0) {
			// Have a builtin - just test. If false, keep going, else done with this rule
			if (builtin_inst_p(lit, substs, table)) {
				return -1;
			}
		} else {
			// Not a builtin
			samp_atom_t *new_atom = rule_atom_to_samp_atom(lit->atom, substs, pred_table);

			int32_t added_atom = add_internal_atom(table, new_atom, false);
			assert(added_atom != -1);
			//if (added_atom == -1) {
			//	// This shouldn't happen, but if it does, we need to free up space
			//	safe_free(new_atom);
			//	mcsat_err("substit: Bad atom\n");
			//	return -1;
			//}
			if (new_atom->pred > 0) {
				found_indirect = true;
			}
			//if (atom_map != NULL) {
			// already had the atom - free it
			safe_free(new_atom);
			//}
			clause_buffer.data[litidx++] = lit->neg ? neg_lit(added_atom)
				: pos_lit(added_atom);
		}
	}

	int32_t clsidx = add_internal_clause(table, clause_buffer.data, litidx,
			rule->frozen_preds, rule->weight, found_indirect, true);

	return clsidx;
}

/* 
 * Pushes a clause to a list depending on its evaluation
 *
 * If it is fixed to be satisfied, push to sat_clauses
 * If it is satisfied, push to the corresponding watched list
 * If it is unsatisified, push to unsat_clauses
 */
int32_t push_alive_clause(samp_clause_t *clause, samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t i;
	int32_t fixable;
	samp_literal_t lit;
	char *atom_str;

	fixable = get_fixable_literal(assignment, clause->disjunct,
			clause->numlits);
	if (fixable >= clause->numlits) {
		mcsat_err("No model exists.\n");
		return -1;
	}
	if (fixable == -1) { // if not propagating
		i = get_true_lit(assignment, clause->disjunct,
				clause->numlits);
		if (i < clause->numlits) {//then lit occurs in the clause
			//      *dcost -= clause->weight;  //subtract weight from dcost
			//swap literal to watched position
			lit = clause->disjunct[i]; 
			clause->disjunct[i] = clause->disjunct[0];
			clause->disjunct[0] = lit;

			push_clause(clause, &clause_table->watched[lit]);
			assert(assigned_true_lit(assignment,
						clause_table->watched[lit]->disjunct[0]));
		} else {
			push_clause(clause, &clause_table->unsat_clauses);
			clause_table->num_unsat_clauses++;
		}
	} else { // we need to fix the truth value of disjunct[fixable]
		// swap the literal to the front
		lit = clause->disjunct[fixable]; 
		clause->disjunct[fixable] = clause->disjunct[0];
		clause->disjunct[0] = lit;

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
		push_clause(clause, &clause_table->sat_clauses);
		assert(assigned_fixed_true_lit(assignment,
					clause_table->sat_clauses->disjunct[0]));
	}
}

extern bool hard_only;

/*
 * Pushes a newly created clause to the corresponding list
 * 
 * In the first round, we only need to find a model for all the HARD clauses.
 * Therefore, when a new clause is activated, if it is a hard clause, we
 * put it into the corresoponding live list, otherwise we put it into the
 * dead lists. In the following rounds, we determine whether to keep the new
 * clause alive by its weight, more specifically, with probability equal to
 * 1 - exp(-w).
 */
void push_newly_activated_clause(int32_t clsidx, samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_clause_t *clause = clause_table->samp_clauses[clsidx];
	samp_truth_value_t *assignment = atom_table->current_assignment;

	double abs_weight = fabs(clause->weight);
	if (clause->weight == DBL_MAX
			|| (!hard_only && choose() < 1 - exp(-abs_weight))) {
		if (clause->numlits == 1 || clause->weight < 0) {
			push_negative_or_unit_clause(clause_table, clsidx);
		} // should be handled immediately
		else {
			push_alive_clause(clause, table);
		}
	}
	else {
		if (clause->numlits == 1 || clause->weight < 0) {
			push_clause(clause, &clause_table->dead_negative_or_unit_clauses);
		}
		else {
			push_clause(clause, &clause_table->dead_clauses);
		}
	}
}

/* 
 * FIXME
 */
int32_t inline pred_default_value(pred_entry_t *pred) {
	return false;
}

/*
 * [lazy MC-SAT only] Returns the default value of a builtinop
 * or user defined predicate of a rule atom.
 *
 * 0: false; 1: true; -1: neither
 */
int32_t rule_atom_default_value(rule_atom_t *rule_atom, pred_table_t *pred_table) {
	int32_t predidx;
	pred_entry_t *pred;
	bool default_value;

	switch (rule_atom->builtinop)  {
	case EQ:
	case PLUS:
	case MINUS:
	case TIMES:
	case DIV:
	case REM:
		return 0;
	case NEQ:
		return 1;
	case 0:
		predidx = rule_atom->pred;
		pred = get_pred_entry(pred_table, predidx);
		default_value = pred_default_value(pred);
		if (default_value)
			return 1;
		else
			return 0;
	default:
		return -1;
	}
}

/*
 * [lazy only] Instantiate all rule instances relevant to an atom
 *
 * Note that substit_buffer.entries has been set up already
 */
void fixed_const_rule_instances(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index) {
//	clause_table_t *clause_table = &table->clause_table;
	pred_table_t *pred_table = &table->pred_table;
//	int prev_num_clauses;
	int32_t i, order;
	int32_t *ordered_lits; // the order of the lits in which they are checked

	if (get_verbosity_level() >= 5) {
		printf("[fixed_const_rule_instances] Instantiating for ");
		print_atom(table->atom_table.atom[atom_index], table);
		printf(": ");
		print_rule_substit(rule, substit_buffer.entries, table);
		printf("\n");
	}

	ordered_lits = (int32_t *) safe_malloc(sizeof(int32_t) * rule->num_lits);
	order = 0;
	for (i = 0; i < rule->num_lits; i++) {
		int32_t default_value = rule_atom_default_value(rule->literals[i]->atom, pred_table);
		if ((rule->literals[i]->neg && default_value == 0)
				|| (!rule->literals[i]->neg && default_value == 1)) {
			ordered_lits[order] = i;
			order++;
		}
	}

	/* termination symbol */
	ordered_lits[order] = -1;
	smart_rule_instances_rec(0, ordered_lits, rule, table, atom_index);

	safe_free(ordered_lits);
}

/*
 * [lazy only] Recursively enumerate the rule instances
 */
void smart_rule_instances_rec(int32_t order, int32_t *ordered_lits, samp_rule_t *rule,
		samp_table_t *table, int32_t atom_index) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t litidx = ordered_lits[order];

	/* when all non-default predicates have been considered,
	 * recursively enumerate the rest variables that haven't been
	 * assigned a constant */
	if (litidx < 0) {
//		if (get_verbosity_level() >= 5) {
//			printf("[smart_rule_instances_rec] Partially instantiated rule ");
//			print_rule_substit(rule, substit_buffer.entries, table);
//			printf(" ...\n");
//		}
		all_rule_instances_rec(0, rule, table, atom_index);
		return;
	}

	rule_atom_t *rule_atom = rule->literals[litidx]->atom;
	int32_t i, j, var_index;

	int arity = atom_arity(rule_atom, pred_table);

	/*
	 * The atom arguments that are consistent with the current substitution.
	 * Can be used to efficiently retrieve the active atoms
	 */
	// could use samp_atom_t *rule_atom_inst;
	int32_t *atom_arg_value = (int32_t *) safe_malloc(sizeof(int32_t) * arity);
	for (i = 0; i < arity; i++) {
		if (rule_atom->args[i].kind == variable) {
			/* make the value consistent with the substitution */
			atom_arg_value[i] = substit_buffer.entries[rule_atom->args[i].value];
		} else {
			atom_arg_value[i] = rule_atom->args[i].value;
		}
	}

	/*
	 * for an evidential predicate and builtinop, check all non-default valued atoms;
	 * for a non-evidential predicate, check all active atoms.
	 */
	if (rule_atom->builtinop > 0) {
		switch (rule_atom->builtinop) {
		case EQ:
		case NEQ:
			if (atom_arg_value[0] != INT32_MIN && atom_arg_value[1] != INT32_MIN) {
				if (atom_arg_value[0] == atom_arg_value[1]) {
					smart_rule_instances_rec(order + 1, ordered_lits, rule, table, atom_index);
				} else {
					return;
				}
			}
			else if (atom_arg_value[0] != INT32_MIN) {
				var_index = rule_atom->args[1].value; // arg[1] is unfixed
				substit_buffer.entries[var_index] = atom_arg_value[0];
				smart_rule_instances_rec(order + 1, ordered_lits, rule, table, atom_index);
				substit_buffer.entries[var_index] = INT32_MIN;
			}
			else if (atom_arg_value[1] != INT32_MIN) {
				var_index = rule_atom->args[0].value; // arg[0] is unfixed
				substit_buffer.entries[var_index] = atom_arg_value[1];
				smart_rule_instances_rec(order + 1, ordered_lits, rule, table, atom_index);
				substit_buffer.entries[var_index] = INT32_MIN;
			}
			else {
				var_index = rule_atom->args[0].value;
				// TODO: enumerate the sort
//				int32_t vsort = rule->vars[var_index]->sort_index;
			}
			break;
		default:
			smart_rule_instances_rec(order + 1, ordered_lits, rule, table, atom_index);
		}
	} else {
		pred_entry_t *pred_entry = get_pred_entry(pred_table, rule_atom->pred);
		bool compatible; // if compatible with the current substitution
		for (i = 0; i < pred_entry->num_atoms; i++) {
			// TODO only consider active atoms, maybe we can keep only
			// the active atoms in pred_entry->atoms
			samp_atom_t *atom = atom_table->atom[pred_entry->atoms[i]];
			compatible = true;
			for (j = 0; j < arity; j++) {
				if (atom_arg_value[j] != INT32_MIN &&
						atom_arg_value[j] != atom->args[j]) {
					compatible = false; // incompatible
				}
			}
			if (!compatible) {
				continue; // move to the next active atom;
			}
			for (j = 0; j < arity; j++) {
				if (atom_arg_value[j] != INT32_MIN) // value already set
					continue;
				// index of the variable in the rule's quantifier list
				var_index = rule_atom->args[j].value;
				substit_buffer.entries[var_index] = atom->args[j]; // for free variable
			}
			smart_rule_instances_rec(order + 1, ordered_lits, rule, table, atom_index);
			for (j = 0; j < arity; j++) { // backtracking, reset values
				if (atom_arg_value[j] != INT32_MIN)
					continue;
				substit_buffer.entries[var_index] = INT32_MIN;
			}
		}
	}

	safe_free(atom_arg_value);
}

/*
 * [lazy only] Activates the rules relevant to a given atom
 *
 * TODO: To debug.  A predicate has default value of true or false.  A clause
 * also has default value of true or false.
 *
 * When the default value of a clause is true: We initially calculate the
 * M-membership for active clauses only.  If some clauses are activated later
 * during the sample SAT, they need to pass the M-membership test to be put
 * into M.
 *
 *     e.g., Fr(x,y) and Sm(x) implies Sm(y) i.e., ~Fr(x,y) or ~Sm(x) or Sm(y)
 *
 *     If Sm(A) is activated (as true) at some stage, do we need to activate
 *     all the clauses related to it? No. We consider two cases:
 *
 *     If y\A, nothing changed, nothing will be activated.
 *
 *     If x\A, if there is some B, Fr(A,B) is also true, and Sm(B) is false
 *     (inactive or active but with false value), [x\A,y\B] will be activated.
 *
 *     How do we do it? We index the active atoms by each argument. e.g.,
 *     Fr(A,?) or Fr(?,B). If a literal is activated to a satisfied value,
 *     e.g., the y\A case above, do nothing. If a literal is activated to an
 *     unsatisfied value, e.g., the x\A case, we check the active atoms of
 *     Fr(x,y) indexed by Fr(A,y). Since Fr(x,y) has a default value of false
 *     that makes the literal satisfied, only the active atoms of Fr(x,y) may
 *     relate to a unsat clause.  Then we get only a small subset of
 *     substitution of y, which can be used to verify the last literal (Sm(y))
 *     easily.  (See the implementation details section in Poon and Domingos
 *     08)
 *
 * When the default value of a clause is false: Poon and Domingos 08 didn't
 * consider this case. In general, this case can't be solved lazily, because we
 * have to try to satisfy all the ground clauses within the sample SAT. This is
 * a rare case in real applications, cause they are usually sparse.
 *
 * What would be the case when we consider clauses with negative weight?  Or
 * more general, any formulas? First of all, MCSAT works for all FOL formulas,
 * not just clauses. So if a input formula has a negative weight, we could nagate the
 * formula and the weight. A Markov logic contains only clauses merely makes
 * the function activation of lazy MC-SAT easier.
 */
void activate_rules(int32_t atom_index, samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	pred_table_t *pred_table = &table->pred_table;
	rule_table_t *rule_table = &table->rule_table;
	pred_tbl_t *pred_tbl = &pred_table->pred_tbl;
	samp_atom_t *atom;
	samp_rule_t *rule_entry;
	int32_t i, j, predicate, arity, num_rules, *rules;
	bool rule_inst_true;

	atom = atom_table->atom[atom_index];
	predicate = atom->pred;
	arity = pred_arity(predicate, pred_table);
	num_rules = pred_tbl->entries[predicate].num_rules;
	rules = pred_tbl->entries[predicate].rules;

	/**
	 * TODO: for each ground clause related to the atom, if it is activated
	 * already, we do nothing.  Otherwise we activate it, and calculate whether
	 * it belongs to the subset of clauses for sample SAT in this round.
	 */
	for (i = 0; i < num_rules; i++) {
		rule_entry = rule_table->samp_rules[rules[i]];
		rule_inst_true = false;
		substit_buffer_resize(rule_entry->num_vars);
		for (j = 0; j < rule_entry->num_lits; j++) {
//			cprintf(6, "Checking whether to activate rule %"PRId32", literal %"PRId32"\n",
//					rules[i], j);
			if (match_atom_in_rule_atom(atom, rule_entry->literals[j], arity)) {
				//then substit_buffer contains the matching substitution
				fixed_const_rule_instances(rule_entry, table, atom_index);
			}
		}
	}
}

/* Adds an atom to internal atom list and then activates it */
int32_t add_and_activate_atom(samp_table_t *table, samp_atom_t *atom) {
	int32_t atom_index = add_internal_atom(table, atom, false);
	activate_atom(table, atom_index);
}

/* 
 * Activates an atom, i.e., sets the active flag and activates the relevant
 * clauses.
 */
int32_t activate_atom(samp_table_t *table, int32_t atom_index) {
	atom_table_t *atom_table = &table->atom_table;
	atom_table->active[atom_index] = true;

	activate_rules(atom_index, table);
}

