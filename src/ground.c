#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "memalloc.h"
#include "utils.h"
#include "tables.h"
#include "print.h"
#include "buffer.h"
#include "input.h"
#include "int_array_sort.h"
#include "yacc.tab.h"
#include "clause_list.h"
#include "mcmc.h"
#include "ground.h"

static ivector_t new_intidx = {0, 0, NULL};

/* Not used */
//static bool eql_samp_atom(samp_atom_t *atom1, samp_atom_t *atom2, samp_table_t *table) {
//	int32_t i, arity;
//
//	if (atom1->pred != atom2->pred) {
//		return false;
//	}
//	arity = pred_arity(atom1->pred, &table->pred_table);
//	for (i = 0; i < arity; i++) {
//		if (atom1->args[i] != atom2->args[i]) {
//			return false;
//		}
//	}
//	return true;
//}

static bool eql_rule_atom(rule_atom_t *atom1, rule_atom_t *atom2, samp_table_t *table) {
	int32_t i, arity;

	if (atom1->pred != atom2->pred) {
		return false;
	}
	arity = pred_arity(atom1->pred, &table->pred_table);
	for (i = 0; i < arity; i++) {
		if ((atom1->args[i].kind != atom2->args[i].kind)
				|| (atom1->args[i].value != atom2->args[i].value)) {
			return false;
		}
	}
	return true;
}

static bool eql_rule_literal(rule_literal_t *lit1, rule_literal_t *lit2,
		samp_table_t *table) {
	return ((lit1->neg == lit2->neg) && eql_rule_atom(lit1->atom, lit2->atom,
				table));
}

/*
 * Checks if the queries are the same - this is essentially a syntactic
 * test, though it won't care about variable names.
 */
bool eql_query_entries(rule_literal_t ***lits, samp_query_t *query,
		samp_table_t *table) {
	int32_t i, j;

	for (i = 0; i < query->num_clauses; i++) {
		if (lits[i] == NULL) {
			return false;
		}
		for (j = 0; query->literals[i][j] != NULL; j++) {
			if (lits[i][j] == NULL) {
				return false;
			}
			if (!eql_rule_literal(query->literals[i][j], lits[i][j], table)) {
				return false;
			}
		}
		// Check if lits[i] has more elements
		if (lits[i][j] != NULL) {
			return false;
		}
	}
	return (lits[i] == NULL);
}

/* check if two query instances (cnf) are the same */
static bool eql_query_instance_lits(samp_literal_t **lit1, samp_literal_t **lit2) {
	int32_t i, j;

	for (i = 0; lit1[i] != NULL; i++) {
		if (lit2[i] == NULL) {
			return false;
		}
		for (j = 0; lit1[i][j] != -1; j++) {
			// Note there is no need to test lit2 != -1
			if (lit2[i][j] != lit1[i][j]) {
				return false;
			}
		}
		// Check if lit2 has more lits
		if (lit2[i][j] != -1) {
			return false;
		}
	}
	// Check if lit2 has more clauses
	if (lit2[i] != NULL) {
		return false;
	}
	return true;
}

/*
 * Recurse through the all the possible constants (except at j, which is fixed to the
 * new constant being added) and add each resulting atom.
 */
static void new_const_atoms_at(int32_t idx, int32_t newcidx, int32_t arity,
		int32_t *psig, samp_atom_t *atom, samp_table_t *table) {
	sort_table_t *sort_table = &table->sort_table;
	sort_entry_t entry;
	int32_t i;

	if (idx == arity) {
		if (get_verbosity_level() > 2) {
			printf("new_const_atoms_at: Adding internal atom ");
			print_atom_now(atom, table);
			printf("\n");
		}
		add_internal_atom(table, atom, false);
	} else if (idx == newcidx) {
		// skip over the new constant - it's already set in atom args.
		new_const_atoms_at(idx + 1, newcidx, arity, psig, atom, table);
	} else {
		entry = sort_table->entries[psig[idx]];
		for (i = 0; i < entry.cardinality; i++) {
			atom->args[idx] = get_sort_const(&entry, i);
			new_const_atoms_at(idx + 1, newcidx, arity, psig, atom, table);
		}
	}
}

/*
 * Fills in the substit_buffer, creates atoms when arity is reached
 */
static void all_pred_instances_rec(int32_t vidx, int32_t *psig, int32_t arity,
		samp_atom_t *atom, samp_table_t *table) {
	sort_table_t *sort_table = &table->sort_table;
	sort_entry_t entry;
	int32_t i;

	if (vidx == arity) {
		if (get_verbosity_level() > 1) {
			printf("all_pred_instances_rec: Adding internal atom ");
			print_atom_now(atom, table);
			printf("\n");
		}
		add_internal_atom(table, atom, false);
	} else {
		entry = sort_table->entries[psig[vidx]];
		for (i = 0; i < entry.cardinality; i++) {
			atom->args[vidx] = get_sort_const(&entry, i);
			all_pred_instances_rec(vidx + 1, psig, arity, atom, table);
		}
	}
}

/* Instantiates all ground atoms for a predicate */
void all_pred_instances(char *pred, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t predval, predidx, arity, *psig;
	samp_atom_t *atom;

	predidx = pred_index(pred, pred_table);
	predval = pred_val_to_index(predidx);
	arity = pred_arity(predval, pred_table);
	psig = pred_signature(predval, pred_table);
	atom_buffer_resize(arity);
	atom = (samp_atom_t *) atom_buffer.data;
	atom->pred = predval;
	all_pred_instances_rec(0, psig, arity, atom, table);
}

/*
 * [lazy MC-SAT only] Returns the default value of a builtinop
 * or user defined predicate of a rule atom.
 *
 * 0: false; 1: true; -1: neither
 */
static int32_t rule_atom_default_value(rule_atom_t *rule_atom, pred_table_t *pred_table) {
	int32_t predidx;
	pred_entry_t *pred;
	bool default_value;

	switch (rule_atom->builtinop)  {
	/* default (majority) value is false */
	case EQ:
	case PLUS:
	case MINUS:
	case TIMES:
	case DIV:
	case REM:
		return 0;
	/* default (majority) value is true */
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
 * [lazy only] Returns the default value of a rule literal
 *
 * 0: false; 1: true; -1: neither 
 */
static int32_t rule_literal_default_value(rule_literal_t *rule_literal,
		pred_table_t *pred_table) {
	int32_t default_value = rule_atom_default_value(rule_literal->atom, pred_table);
	if (rule_literal->neg && default_value == 0) {
		return 1;
	}
	else if (rule_literal->neg && default_value == 1) {
		return 0;
	}
	else {
		return default_value;
	}
}

/*
 * Evaluates a builtin literal (a builtin operation with an optional negation)
 *
 * TODO: can be replaced by literal_falsifiable
 */
static bool builtin_inst_p(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t j, arity, *nargs, argidx;
	bool builtin_result, retval;

	assert(lit->atom->builtinop != 0);
	arity = rule_atom_arity(lit->atom, pred_table);
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

/* 
 * Tests whether a literal is falsifiable, i.e., is not fixed to be satisfied 
 */
static bool literal_falsifiable(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	rule_atom_t *atom = lit->atom;
	bool neg = lit->neg;
	int32_t predicate = atom->pred;
	int32_t arity = rule_atom_arity(atom, pred_table);
	int32_t i;

	atom_buffer_resize(arity);
	samp_atom_t *rule_inst_atom = (samp_atom_t *) atom_buffer.data;
	rule_inst_atom->pred = predicate;
	for (i = 0; i < arity; i++) { //copy each instantiated argument
		if (atom->args[i].kind == variable) {
			rule_inst_atom->args[i] = substs[atom->args[i].value];
		} else {
			rule_inst_atom->args[i] = atom->args[i].value;
		}
	}

	/* 0: fixed false; 1: fixed true; -1: unfixed */
	int32_t value;
	if (atom->builtinop > 0) {
		value = call_builtin(atom->builtinop, builtin_arity(atom->builtinop),
				rule_inst_atom->args) ? 1 : 0;
	}
	else {
		array_hmap_pair_t *atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
				arity + 1, (int32_t *) rule_inst_atom);
		if (atom_map == NULL || !atom_table->active[atom_map->val]) { /* if inactive */
			value = rule_atom_default_value(atom, pred_table);
		}
		else if (predicate <= 0) { /* for direct predicate */
			value = assigned_true(atom_table->assignment[atom_map->val]);
		} else { /* indirect predicate has unfixed value */
			value = -1; /* neither true nor false */
		}
	}
	return (neg ? (value != 0) : (value != 1));
}

/* Tests whether a clause is falsifiable, i.e., is not fixed to be satisfied */
static bool check_clause_falsifiable(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t i;
	char *atom_str;

	for (i = 0; i < rule->num_lits; i++) {
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

/* 
 * Checks whether a clause has been instantiated already.
 *
 * If it does not exist, add it into the hash set.
 */
static bool check_clause_duplicate(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table) {
	array_hmap_pair_t *rule_subst_map = array_size_hmap_find(&rule->subst_hash,
			rule->num_vars, (int32_t *) substs);
	if (rule_subst_map != NULL) {
		cprintf(5, "clause that has been checked or already exists!\n");
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
static int32_t substit_rule(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table) {
	const_table_t *const_table = &table->const_table;
	sort_table_t *sort_table = &table->sort_table;
	pred_table_t *pred_table = &table->pred_table;
	sort_entry_t *sort_entry;
	int32_t vsort, csort, csubst;
	// array_hmap_pair_t *atom_map;
	int32_t i, litidx;
	bool found_indirect;

	assert(valid_table(table));

	/* check if the constants are compatible with the sorts */
	for (i = 0; i < rule->num_vars; i++) {
		csubst = substs[i];
		// Not enough constants, i given, rule->num_vars required
		assert(csubst != INT32_MIN);
		vsort = rule->vars[i]->sort_index;
		sort_entry = &sort_table->entries[vsort];
		if (sort_entry->constants == NULL) { 
			// It's an integer; check that its value is within the sort
			assert(sort_entry->lower_bound <= csubst
					&& csubst <= sort_entry->upper_bound);
		} else {
			csort = const_sort_index(substs[i], const_table);
			// Constant/variable sorts do not match
			assert(subsort_p(csort, vsort, sort_table));
		}
	}
	// Too many constants, i given, rule->num_vars required
	assert(substs == NULL || substs[i] == INT32_MIN);

	/* check duplicates and insert to hashset */
	if (lazy_mcsat() && !check_clause_duplicate(rule, substs, table)) {
		return -1;
	}

	/* check if the value is fixed to be true, if so discard it */
	if (lazy_mcsat() && !check_clause_falsifiable(rule, substs, table)) {
		return -1;
	}

	assert(valid_table(table));

	// Everything is OK, do the substitution
	// We just use the clause_buffer - first make sure it's big enough
	clause_buffer_resize(rule->num_lits);
	litidx = 0;
	found_indirect = false;
	for (i = 0; i < rule->num_lits; i++) {
		rule_literal_t *lit = rule->literals[i];
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
			if (new_atom->pred > 0) {
				found_indirect = true;
			}
			safe_free(new_atom);
			clause_buffer.data[litidx++] = lit->neg ? neg_lit(added_atom)
				: pos_lit(added_atom);
		}
	}

	int32_t clsidx = add_internal_clause(table, clause_buffer.data, litidx,
			rule->frozen_preds, rule->weight, found_indirect, true);

	return clsidx;
}

/*
 * all_rule_instances takes a rule and generates all ground instances
 * based on the atoms, their predicates and corresponding sorts
 *
 * For eager MC-SAT, it is called by all_rule_instances with atom_index equal to -1;
 * For lazy MC-SAT, the implementation is not efficient.
 */
static void all_rule_instances_rec(int32_t vidx, samp_rule_t *rule, samp_table_t *table, 
		int32_t atom_index) {

	/* termination of the recursion */
	if (vidx == rule->num_vars) {
		substit_buffer.entries[vidx] = INT32_MIN;
		if (get_verbosity_level() >= 5) {
			printf("[all_rule_instances_rec] Rule ");
			print_rule_substit(rule, substit_buffer.entries, table);
			printf(" is being instantiated ...\n");
		}

		int32_t success = substit_rule(rule, substit_buffer.entries, table);

		if (get_verbosity_level() >= 5) {
			if (success < 0) {
				printf("[all_rule_instances_rec] Failed.\n");
			} else {
				printf("[all_rule_instances_rec] Succeeded.\n");
			}
		}
		return;
	}

	if (substit_buffer.entries[vidx] != INT32_MIN) {
		all_rule_instances_rec(vidx + 1, rule, table, atom_index);
	} else {
		sort_table_t *sort_table = &table->sort_table;
		int32_t vsort = rule->vars[vidx]->sort_index;
		sort_entry_t entry = sort_table->entries[vsort];
		int32_t i;
		for (i = 0; i < entry.cardinality; i++) {
			if (entry.constants == NULL) {
				if (entry.ints == NULL) {
					// Have a subrange
					substit_buffer.entries[vidx] = entry.lower_bound + i;
				} else {
					substit_buffer.entries[vidx] = entry.ints[i];
				}
			} else {
				substit_buffer.entries[vidx] = entry.constants[i];
			}
			all_rule_instances_rec(vidx + 1, rule, table, atom_index);
			substit_buffer.entries[vidx] = INT32_MIN; // backtrack
		}
	}
}

void all_rule_instances(int32_t rule_index, samp_table_t *table) {
	rule_table_t *rule_table = &table->rule_table;
	samp_rule_t *rule = rule_table->samp_rules[rule_index];
	substit_buffer_resize(rule->num_vars);
	int32_t i;
	for (i = 0; i < substit_buffer.size; i++) {
		substit_buffer.entries[i] = INT32_MIN;
	}
	all_rule_instances_rec(0, rule, table, -1);
}

/*
 * Returns an estimate of the number of active atoms for an rule_atom. For an
 * direct predicate or builtinop, check all non-default valued atoms; for
 * a indirect predicate, check all active atoms.
 */
static int32_t get_num_active_atoms(samp_rule_t *rule, int32_t litidx,
		samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	sort_table_t *sort_table = &table->sort_table;

	rule_atom_t *rule_atom = rule->literals[litidx]->atom;
	int32_t arity = rule_atom_arity(rule_atom, pred_table);
	int32_t *atom_arg_values = (int32_t *) safe_malloc(sizeof(int32_t) * arity);
	int32_t natoms = INT32_MAX; /* the number of active atoms */

	int32_t i, sortidx;
	for (i = 0; i < arity; i++) {
		if (rule_atom->args[i].kind == variable) {
			/* make the value consistent with the substitution */
			atom_arg_values[i] = substit_buffer.entries[rule_atom->args[i].value];
		} else {
			atom_arg_values[i] = rule_atom->args[i].value;
		}
	}

	if (rule_atom->builtinop > 0) {
		switch (rule_atom->builtinop) {
		case EQ:
		case NEQ:
			if (atom_arg_values[0] != INT32_MIN || atom_arg_values[1] != INT32_MIN) {
				natoms = 1;
			}
			else {
				sortidx = rule->vars[rule_atom->args[0].value]->sort_index;
				natoms = sort_table->entries[sortidx].cardinality;
			}
			break;
		default:
			break;
		}
	}
	else {
		pred_entry_t *pred_entry = get_pred_entry(pred_table, rule_atom->pred);
		natoms = pred_entry->num_atoms;
	}
	return natoms;
}

/*
 * Select a literal of the rule to ground. We select a literal that has the
 * minimum number of groundings (greedy strategy). For literals whose default
 * value is true, the number of groundings is equal to the number of active
 * atoms. For literals whose default value is false, the number of groundings
 * is (approximately) equal to the number of ALL instances.
 */
static int32_t select_literal_to_ground(samp_rule_t *rule, samp_table_t *table,
		bool *lits_grounded) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t litidx = -1, min_nsubsts = INT32_MAX;

	int32_t i, nsubsts;
	for (i = 0; i < rule->num_lits; i++) {
		if (lits_grounded[i]) continue;

		if (rule_literal_default_value(rule->literals[i], pred_table) == 1) {
			/* default value is true */
			nsubsts = get_num_active_atoms(rule, i, table);
		}
		else {
			/* all default value atoms need to be grounded */
			continue;
		}

		if (min_nsubsts > nsubsts) {
			min_nsubsts = nsubsts;
			litidx = i;
		}
	}
	return litidx;
}

/*
 * [lazy only] Recursively enumerate the rule instances
 *
 * TODO: change to non-recursive
 */
static void smart_disjunct_instances_rec(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index, bool *lits_grounded) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	sort_table_t *sort_table = &table->sort_table;

	int32_t litidx = select_literal_to_ground(rule, table, lits_grounded);

	/* when all non-default predicates have been considered,
	 * recursively enumerate the rest variables that haven't been
	 * assigned a constant */
	if (litidx < 0) {
		if (get_verbosity_level() >= 5) {
			printf("[smart_disjunct_instances_rec] partially instantiated rule ");
			print_rule_substit(rule, substit_buffer.entries, table);
			printf(" ...\n");
		}
		all_rule_instances_rec(0, rule, table, atom_index);
		return;
	}

	rule_atom_t *rule_atom = rule->literals[litidx]->atom;
	int32_t i, j, var_index;

	int arity = rule_atom_arity(rule_atom, pred_table);

	/* The atom arguments that are consistent with the current substitution.
	 * TODO the consistent active atoms could be efficiently retrieved */
	int32_t *atom_arg_values = (int32_t *) safe_malloc(sizeof(int32_t) * arity);
	for (i = 0; i < arity; i++) {
		if (rule_atom->args[i].kind == variable) {
			/* make the value consistent with the substitution */
			atom_arg_values[i] = substit_buffer.entries[rule_atom->args[i].value];
		} else {
			atom_arg_values[i] = rule_atom->args[i].value;
		}
	}

	lits_grounded[litidx] = true;

	/* for a direct predicate or builtinop, check all non-default value atoms;
	 * for a indirect predicate, check all active atoms. */
	if (rule_atom->builtinop > 0) {
		switch (rule_atom->builtinop) {
		case EQ:
		case NEQ:
			if (atom_arg_values[0] != INT32_MIN && atom_arg_values[1] != INT32_MIN) {
				if (atom_arg_values[0] == atom_arg_values[1]) {
					smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
				} else {
					return;
				}
			}
			else if (atom_arg_values[0] != INT32_MIN) {
				var_index = rule_atom->args[1].value; /* arg[1] is unfixed */
				substit_buffer.entries[var_index] = atom_arg_values[0];
				smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
				substit_buffer.entries[var_index] = INT32_MIN;
			}
			else if (atom_arg_values[1] != INT32_MIN) {
				var_index = rule_atom->args[0].value; /* arg[0] is unfixed */
				substit_buffer.entries[var_index] = atom_arg_values[1];
				smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
				substit_buffer.entries[var_index] = INT32_MIN;
			}
			else {
				int32_t varidx0 = rule_atom->args[0].value;
				int32_t varidx1 = rule_atom->args[1].value;
				int32_t sortidx = rule->vars[varidx0]->sort_index;
				sort_entry_t *sort_entry = &sort_table->entries[sortidx];
				for (i = 0; i < sort_entry->cardinality; i++) {
					int32_t cons = get_sort_const(sort_entry, i);
					substit_buffer.entries[varidx0] = cons;
					substit_buffer.entries[varidx1] = cons;
					smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
					substit_buffer.entries[varidx0] = INT32_MIN;
					substit_buffer.entries[varidx1] = INT32_MIN;
				}
			}
			break;
		default:
			smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
		}
	} else {
		pred_entry_t *pred_entry = get_pred_entry(pred_table, rule_atom->pred);
		bool compatible;
		
		for (i = 0; i < pred_entry->num_atoms; i++) {
			if (!atom_table->active[i]) continue;
			samp_atom_t *atom = atom_table->atom[pred_entry->atoms[i]];
			compatible = true;
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] != INT32_MIN &&
						atom_arg_values[j] != atom->args[j]) {
					compatible = false;
				}
			}
			if (!compatible) {
				continue;
			}
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] == INT32_MIN) {
					var_index = rule_atom->args[j].value;
					substit_buffer.entries[var_index] = atom->args[j];
				}
			}
			smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] == INT32_MIN) {
					var_index = rule_atom->args[j].value;
					substit_buffer.entries[var_index] = INT32_MIN;
				}
			}
		}
	}

	lits_grounded[litidx] = false;
	safe_free(atom_arg_values);
}

static int32_t select_clause_to_ground(samp_rule_t *rule, samp_table_t *table,
		bool *clauses_grounded) {
	pred_table_t *pred_table = &table->pred_table;
	rule_atom_t *rule_atom;
	int32_t clsidx = -1, min_nsubsts = INT32_MAX;

	int32_t i, nsubsts;
	for (i = 0; i < rule->num_lits; i++) {
		rule_atom = rule->literals[i]->atom;
		if (rule_atom->builtinop > 0 || rule_atom->pred <= 0) {
			if (rule_literal_default_value(rule->literals[i], pred_table) == 0) {
				/* default is fixed false */
				nsubsts = get_num_active_atoms(rule, i, table);
			}
			else {
				/* all default value atoms need to be grounded */
				continue;
			}
		}
		else {
			/* Still many groundings, taken care of later.
			 * Basically, for a conjunction like A(x) & B(x), the unsat
			 * clauses include the groundins where A(x) is false and
			 * B(x) is any value and where A(x) is any value and B(x)
			 * is false. */
			continue;
		}
		if (min_nsubsts > nsubsts) {
			min_nsubsts = nsubsts;
			clsidx = i;
		}
	}
	return clsidx;
}

/* 
 * The clauses that are fixed unsat by default have been grounded. The clauses
 * that are fixed sat will be considered at last by brute force. Now we
 * consider the clauses with unfixed values. If there exists an unfixed clauses
 * whose default value is unsat, we basically need to ground everything by
 * brute force. If all unfixed clauses have default value of sat, we each time
 * find the unsat instances of one clause and ground other clauses by brute
 * force.
 */
static void smart_conjunct_clause_instances(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index, bool *clauses_grounded) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	rule_literal_t *rule_literal;
	rule_atom_t *rule_atom;
	pred_entry_t *pred_entry;
	int32_t i, j, arity, var_index;

	for (i = 0; i < rule->num_lits; i++) {
		rule_literal = rule->literals[i];
		rule_atom = rule_literal->atom;

		/* grounded clauses should have fixed value */
		assert(!clauses_grounded[i] || rule_atom_is_direct(rule_atom));
		
		if (!rule_atom_is_direct(rule_atom) 
				&& rule_literal_default_value(rule_literal, pred_table) != 0) {
			break;
		}
	}
	/* There exists a unfixed clause with default value of unsat */
	if (i < rule->num_lits) {
		all_rule_instances_rec(0, rule, table, atom_index);
		return;
	}

	for (i = 0; i < rule->num_lits; i++) {
		rule_atom = rule->literals[i]->atom;
		arity = rule_atom_arity(rule_atom, pred_table);

		/* Skip grounded (fixed unsat by default) clauses and clauses fixed sat
		 * by default */
		if (rule_atom_is_direct(rule_atom))
			continue;

		int32_t *atom_arg_values = (int32_t *) safe_malloc(sizeof(int32_t) * arity);
		for (i = 0; i < arity; i++) {
			if (rule_atom->args[i].kind == variable) {
				/* make the value consistent with the substitution */
				atom_arg_values[i] = substit_buffer.entries[rule_atom->args[i].value];
			} else {
				atom_arg_values[i] = rule_atom->args[i].value;
			}
		}

		pred_entry = get_pred_entry(pred_table, rule_atom->pred);
		bool compatible; /* if compatible with the current substitution */

		for (i = 0; i < pred_entry->num_atoms; i++) {
			samp_atom_t *atom = atom_table->atom[pred_entry->atoms[i]];
			compatible = true;
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] != INT32_MIN &&
						atom_arg_values[j] != atom->args[j]) {
					compatible = false;
				}
			}
			if (!compatible) {
				continue;
			}
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] == INT32_MIN) {
					var_index = rule_atom->args[j].value;
					substit_buffer.entries[var_index] = atom->args[j];
				}
			}
			all_rule_instances_rec(0, rule, table, atom_index);
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] == INT32_MIN) {
					var_index = rule_atom->args[j].value;
					substit_buffer.entries[var_index] = INT32_MIN;
				}
			}
		}
		safe_free(atom_arg_values);
	}
}

/*
 * Instantiate the clauses that are fixed unsat by default (i.e. ALL literals
 * are fixed false by default), because most rules are always unsat and don't
 * need to be considered, so we only need to instantiate the clauses that are
 * fixed sat (the active clauses).
 */
static void smart_conjunct_instances_rec(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index, bool *clauses_grounded) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	sort_table_t *sort_table = &table->sort_table;

	int32_t clsidx = select_clause_to_ground(rule, table, clauses_grounded); 

	if (clsidx < 0) {
		smart_conjunct_clause_instances(rule, table, atom_index, clauses_grounded);
		return;
	}

	rule_atom_t *rule_atom;
	int32_t i, j, var_index, arity;

	rule_atom = rule->literals[clsidx]->atom;
	arity = rule_atom_arity(rule_atom, pred_table);

	int32_t *atom_arg_values = (int32_t *) safe_malloc(sizeof(int32_t) * arity);
	for (i = 0; i < arity; i++) {
		if (rule_atom->args[i].kind == variable) {
			atom_arg_values[i] = substit_buffer.entries[rule_atom->args[i].value];
		} else {
			atom_arg_values[i] = rule_atom->args[i].value;
		}
	}

	clauses_grounded[clsidx] = true;
	if (rule_atom->builtinop > 0) {
		switch (rule_atom->builtinop) {
		case EQ:
		case NEQ:
			if (atom_arg_values[0] != INT32_MIN && atom_arg_values[1] != INT32_MIN) {
				if (atom_arg_values[0] == atom_arg_values[1]) {
					smart_disjunct_instances_rec(rule, table, atom_index, clauses_grounded);
				} else {
					return;
				}
			}
			else if (atom_arg_values[0] != INT32_MIN) {
				var_index = rule_atom->args[1].value;
				substit_buffer.entries[var_index] = atom_arg_values[0];
				smart_disjunct_instances_rec(rule, table, atom_index, clauses_grounded);
				substit_buffer.entries[var_index] = INT32_MIN;
			}
			else if (atom_arg_values[1] != INT32_MIN) {
				var_index = rule_atom->args[0].value;
				substit_buffer.entries[var_index] = atom_arg_values[1];
				smart_disjunct_instances_rec(rule, table, atom_index, clauses_grounded);
				substit_buffer.entries[var_index] = INT32_MIN;
			}
			else {
				int32_t varidx0 = rule_atom->args[0].value;
				int32_t varidx1 = rule_atom->args[1].value;
				int32_t sortidx = rule->vars[varidx0]->sort_index;
				sort_entry_t *sort_entry = &sort_table->entries[sortidx];
				for (i = 0; i < sort_entry->cardinality; i++) {
					int32_t cons = get_sort_const(sort_entry, i);
					substit_buffer.entries[varidx0] = cons;
					substit_buffer.entries[varidx1] = cons;
					smart_disjunct_instances_rec(rule, table, atom_index, clauses_grounded);
					substit_buffer.entries[varidx0] = INT32_MIN;
					substit_buffer.entries[varidx1] = INT32_MIN;
				}
			}
			break;
		default:
			smart_disjunct_instances_rec(rule, table, atom_index, clauses_grounded);
		}
	} else {
		pred_entry_t *pred_entry = get_pred_entry(pred_table, rule_atom->pred);
		bool compatible;
		
		for (i = 0; i < pred_entry->num_atoms; i++) {
			if (!atom_table->active[i]) continue;
			samp_atom_t *atom = atom_table->atom[pred_entry->atoms[i]];
			compatible = true;
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] != INT32_MIN &&
						atom_arg_values[j] != atom->args[j]) {
					compatible = false;
				}
			}
			if (!compatible) {
				continue;
			}
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] == INT32_MIN) {
					var_index = rule_atom->args[j].value;
					substit_buffer.entries[var_index] = atom->args[j];
				}
			}
			smart_disjunct_instances_rec(rule, table, atom_index, clauses_grounded);
			for (j = 0; j < arity; j++) {
				if (atom_arg_values[j] == INT32_MIN) {
					var_index = rule_atom->args[j].value;
					substit_buffer.entries[var_index] = INT32_MIN;
				}
			}
		}
	}

	clauses_grounded[clsidx] = false;
	safe_free(atom_arg_values);
}

/*
 * [lazy only] Instantiate all rule instances relevant to an atom; similar to
 * all_rule_instances.
 *
 * Note that substit_buffer.entries has been set up already
 *
 * FIXME There are several cases:
 *
 * 1) weight is positive: If the literal is true when the predicate takes the
 * default value, we will ground all the non-default (activated) atoms of the
 * predicate; if the literal is false when the predicate takes the default
 * value, there will be TOO MANY groundings to consider so we skip it and
 * consider it later.
 * 
 * 2) weight is negative:
 * 
 * 2.1) The literal is a undirect predicate. If the literal is true when the
 * predicate takes the default value, we ground all non-default atoms of the
 * predicate, and all the other free variables can take any feasible value. If
 * the literal is false by default, we basically need to ground EVERYTHING for
 * this rule.  
 *
 * 2.2) The literal is a direct predicate (or built-in operator).  If the
 * literal is false when the predicate takes the default value, ground the
 * non-default valued atoms and continue on the next literal. If the literal is
 * true, most formulas are FIXED satisfied so we still ground the non-default
 * valued atoms and continue.
 *
 * 3) If the formula is a general cnf, it would be similar to case 2)
 */
static void fixed_const_rule_instances(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index) {

	if (get_verbosity_level() >= 5) {
		printf("[fixed_const_rule_instances] Instantiating ");
		print_rule_substit(rule, substit_buffer.entries, table);
		if (atom_index > 0) {
			printf(" for ");
			print_atom(table->atom_table.atom[atom_index], table);
		}
		printf(":\n");
	}

	bool *lits_grounded = (bool *) safe_malloc(sizeof(int32_t) * rule->num_lits);
	memset(lits_grounded, false, rule->num_lits);

	if (rule->weight < 0) {
		smart_conjunct_instances_rec(rule, table, atom_index, lits_grounded);
	}
	else {
		smart_disjunct_instances_rec(rule, table, atom_index, lits_grounded);
	}
	safe_free(lits_grounded);
}

/*
 * [lazy only] Instantiate all rule instances
 */
void smart_all_rule_instances(int32_t rule_index, samp_table_t *table) {
	rule_table_t *rule_table = &table->rule_table;
	samp_rule_t *rule = rule_table->samp_rules[rule_index];
	substit_buffer_resize(rule->num_vars);
	int32_t i;
	for (i = 0; i < substit_buffer.size; i++) {
		substit_buffer.entries[i] = INT32_MIN;
	}
	fixed_const_rule_instances(rule, table, -1);
}

/*
 * Queries are not like rules, as rules are simpler (can treat as separate
 * clauses), and have a weight.  substit on rules updates the clause_table,
 * whereas substit_query updates the query_instance_table.
 */
static int32_t add_subst_query_instance(samp_literal_t **litinst, substit_entry_t *substs,
		samp_query_t *query, samp_table_t *table) {
	query_instance_table_t *query_instance_table = &table->query_instance_table;
	query_table_t *query_table = &table->query_table;
	sort_table_t *sort_table = &table->sort_table;
	samp_query_instance_t *qinst;
	sort_entry_t *sort_entry;
	int32_t i, qindex, vsort;

	for (i = 0; i < query_instance_table->num_queries; i++) {
		if (eql_query_instance_lits(litinst,
					query_instance_table->query_inst[i]->lit)) {
			break;
		}
	}
	qindex = i;
	if (i < query_instance_table->num_queries) {
		// Already have this instance - just associate it with the query
		qinst = query_instance_table->query_inst[i];
		extend_ivector(&qinst->query_indices);
		// find the index of the current query
		for (i = 0; i < query_table->num_queries; i++) {
			if (query_table->query[i] == query) {
				break;
			}
		}
		qinst->query_indices.data[qinst->query_indices.size] = i;
		qinst->query_indices.size++;
		safe_free(litinst);
	} else {
		query_instance_table_resize(query_instance_table);
		qinst = (samp_query_instance_t *) safe_malloc(
				sizeof(samp_query_instance_t));
		query_instance_table->query_inst[qindex] = qinst;
		query_instance_table->num_queries += 1;
		for (i = 0; i < query_table->num_queries; i++) {
			if (query_table->query[i] == query) {
				break;
			}
		}
		//qinst->query_index = i;
		init_ivector(&qinst->query_indices, 1);
		qinst->query_indices.data[0] = i;
		qinst->query_indices.size++;
		qinst->sampling_num = table->atom_table.num_samples;
		qinst->pmodel = 0;
		// Copy the substs - used to return the formula instances
		qinst->subst = (int32_t *) safe_malloc(
				query->num_vars * sizeof(int32_t));
		qinst->constp = (bool *) safe_malloc(query->num_vars * sizeof(bool));
		for (i = 0; i < query->num_vars; i++) {
			qinst->subst[i] = substs[i];
			vsort = query->vars[i]->sort_index;
			sort_entry = &sort_table->entries[vsort];
			qinst->constp[i] = (sort_entry->constants != NULL);
		}
		qinst->lit = litinst;
	}
	return qindex;
}

/*
 * Substitute the variables in a quantified query with constants
 */
static int32_t substit_query(samp_query_t *query, substit_entry_t *substs, samp_table_t *table) {
	const_table_t *const_table = &table->const_table;
	sort_table_t *sort_table = &table->sort_table;
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	sort_entry_t *sort_entry;
	samp_atom_t *new_atom;
	int32_t i, j, vsort, csort, csubst, arity, added_atom, numlits, litnum;
	rule_literal_t ***lit;
	samp_literal_t **litinst;

	for (i = 0; i < query->num_vars; i++) {
		csubst = substs[i];
		if (csubst == INT32_MIN) {
			mcsat_err("substit: Not enough constants - %"PRId32" given, %"PRId32" required\n",
					i, query->num_vars);
			return -1;
		}
		vsort = query->vars[i]->sort_index;
		sort_entry = &sort_table->entries[vsort];
		if (sort_entry->constants == NULL) {
			// It's an integer; check that its value is within the sort
			if (!(sort_entry->lower_bound <= csubst && csubst
						<= sort_entry->upper_bound)) {
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
	if (substs != NULL && substs[i] != INT32_MIN) {
		mcsat_err("substit: Too many constants - %"PRId32" given, %"PRId32" required\n",
				i, query->num_vars);
		return -1;
	}
	// Everything is OK, do the substitution
	lit = query->literals;
	litinst = (samp_literal_t **) safe_malloc(
			(query->num_clauses + 1) * sizeof(samp_literal_t *));
	for (i = 0; i < query->num_clauses; i++) {
		numlits = 0;
		for (j = 0; lit[i][j] != NULL; j++) {
			if (lit[i][j]->atom->builtinop != 0) {
				// Have a builtin - just test.  If false, keep going, else done
				// with this query clause
				if (builtin_inst_p(lit[i][j], substs, table)) {
					numlits = 0;
					break;
				}
			} else {
				numlits++;
			}
		}
		if (numlits > 0) { // Can ignore clauses for which numlits == 0
			litinst[i] = (samp_literal_t *) safe_malloc(
					(numlits + 1) * sizeof(samp_literal_t));
			litnum = 0;
			for (j = 0; lit[i][j] != NULL; j++) {
				if (lit[i][j]->atom->builtinop == 0) {
					arity = pred_arity(lit[i][j]->atom->pred, pred_table);
					new_atom = rule_atom_to_samp_atom(lit[i][j]->atom, substs, pred_table);
					if (lazy_mcsat()) {
						array_hmap_pair_t *atom_map;
						atom_map = array_size_hmap_find(
								&(atom_table->atom_var_hash), arity + 1, //+1 for pred
								(int32_t *) new_atom);
						if (atom_map != NULL) {
							added_atom = atom_map->val;
						} else {
							added_atom = -1;
						}
					} else {
						if (get_verbosity_level() > 1) {
							printf("substit_query: Adding internal atom ");
							print_atom_now(new_atom, table);
							printf("\n");
						}
						added_atom = add_internal_atom(table, new_atom, false);
					}
					if (added_atom == -1) {
						// This shouldn't happen, but if it does, we need to free up space
						free_samp_atom(new_atom);
						mcsat_err("substit: Bad atom\n");
						return -1;
					}
					free_samp_atom(new_atom);
					litinst[i][litnum++] = lit[i][j]->neg ? neg_lit(added_atom)
						: pos_lit(added_atom);
				}
			}
			litinst[i][litnum++] = -1; // end-marker - NULL doesn't work
		}
	}
	litinst[i] = NULL;
	return add_subst_query_instance(litinst, substs, query, table);
}

/*
 * check_clause_instance has been removed ...
 * Similar to (and based on) check_clause_instance
 * TODO: How does it differ from check_clause_instance
 */
static bool check_query_instance(samp_query_t *query, samp_table_t *table) {
	pred_table_t *pred_table = &(table->pred_table);
	atom_table_t *atom_table = &(table->atom_table);
	int32_t predicate, arity, i, j, k;
	rule_atom_t *atom;
	samp_atom_t *rule_inst_atom;
	array_hmap_pair_t *atom_map;

	for (i = 0; i < query->num_clauses; i++) { //for each clause
		for (j = 0; query->literals[i][j] != NULL; j++) { //for each literal
			atom = query->literals[i][j]->atom;
			predicate = atom->pred;
			arity = pred_arity(predicate, pred_table);
			atom_buffer_resize(arity);
			rule_inst_atom = (samp_atom_t *) atom_buffer.data;
			rule_inst_atom->pred = predicate;
			for (k = 0; k < arity; k++) {//copy each instantiated argument
				if (atom->args[k].kind == variable) {
					rule_inst_atom->args[k]
						= substit_buffer.entries[atom->args[k].value];
				} else {
					rule_inst_atom->args[k] = atom->args[k].value;
				}
			}
			//find the index of the atom
			atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
					arity + 1, (int32_t *) rule_inst_atom);
			// if atom is inactive it needs to be activated
			if (atom_map == NULL) {
				if (get_verbosity_level() >= 0) {
					printf("Activating atom (at the initialization of query) ");
					print_atom(rule_inst_atom, table);
					printf("\n");
					fflush(stdout);
				}
				int32_t atom_index = add_internal_atom(table, rule_inst_atom, false);

				// check for witness predicate - fixed false if NULL atom_map
				atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
						arity + 1, (int32_t *) rule_inst_atom);
				assert(atom_map != NULL);

				/* FIXME substit_buffer is being used by query instantiating,
				 * will cause a conflict */
				//activate_atom(table, atom_index);
			}

			if (query->literals[i][j]->neg &&
					atom_table->assignment[atom_map->val] == v_fixed_false) {
				return false;//literal is fixed true
			}
			if (!query->literals[i][j]->neg &&
					atom_table->assignment[atom_map->val] == v_fixed_true) {
				return false;//literal is fixed true
			}
		}
	}
	return true;
}

/*
 * This is used in the simple case where the samp_query has no variables
 * So we simply convert it to a query_instance and add it to the tables.
 */
static int32_t samp_query_to_query_instance(samp_query_t *query, samp_table_t *table) {
	pred_table_t *pred_table = &(table->pred_table);
	samp_atom_t *new_atom;
	int32_t i, j, k, arity, added_atom;
	//int32_t argidx;
	rule_literal_t ***lit;
	samp_literal_t **litinst;

	lit = query->literals;
	litinst = (samp_literal_t **) safe_malloc(
			(query->num_clauses + 1) * sizeof(samp_literal_t *));
	for (i = 0; i < query->num_clauses; i++) {
		for (j = 0; lit[i][j] != NULL; j++) {
		}
		litinst[i] = (samp_literal_t *) safe_malloc(
				(j + 1) * sizeof(samp_literal_t));
		for (j = 0; lit[i][j] != NULL; j++) {
			arity = pred_arity(lit[i][j]->atom->pred, pred_table);
			new_atom = rule_atom_to_samp_atom(lit[i][j]->atom, NULL, pred_table);
			for (k = 0; k < arity; k++) {
				//argidx = new_atom->args[k];
			}
			//if (get_verbosity_level() > 1) {
			//	printf("[samp_query_to_query_instance] Adding internal atom ");
			//	print_atom_now(new_atom, table);
			//	printf("\n");
			//}
			added_atom = add_internal_atom(table, new_atom, false);
			if (added_atom == -1) {
				// This shouldn't happen, but if it does, we need to free up space
				mcsat_err("[samp_query_to_query_instance] Bad atom\n");
				return -1;
			}
			litinst[i][j] = lit[i][j]->neg ? neg_lit(added_atom) : pos_lit(
					added_atom);
		}
		litinst[i][j] = -1; // end-marker - NULL doesn't work
	}
	litinst[i] = NULL;
	return add_subst_query_instance(litinst, NULL, query, table);
}

/*
 * Recursively generates all groundings of a query
 */
static void all_query_instances_rec(int32_t vidx, samp_query_t *query,
		samp_table_t *table, int32_t atom_index) {
	sort_table_t *sort_table;
	sort_entry_t entry;
	int32_t i, vsort;

	/* termination of recursion */
	if (vidx == query->num_vars) {
		substit_buffer.entries[vidx] = INT32_MIN;
		if (!lazy_mcsat() 
				|| check_query_instance(query, table)
				) {
			substit_query(query, substit_buffer.entries, table);
		}
		return;
	}

	if (substit_buffer.entries[vidx] != INT32_MIN) {
		all_query_instances_rec(vidx + 1, query, table, atom_index);
	} else {
		sort_table = &table->sort_table;
		vsort = query->vars[vidx]->sort_index;
		entry = sort_table->entries[vsort];
		for (i = 0; i < entry.cardinality; i++) {
			substit_buffer.entries[vidx] = get_sort_const(&entry, i);
			all_query_instances_rec(vidx + 1, query, table, atom_index);
			substit_buffer.entries[vidx] = INT32_MIN;
		}
	}
}

/*
 * Similar to all_rule_instances, but updates query_instance_table
 * instead of clause_table
 */
void all_query_instances(samp_query_t *query, samp_table_t *table) {
	int32_t i;

	if (query->num_vars == 0) {
		// Already instantiated, but needs to be converted from rule_literals to
		// samp_clauses.  Do this here since we don't know if variables are
		// involved earlier.
		samp_query_to_query_instance(query, table);
	} else {
		substit_buffer_resize(query->num_vars);
		for (i = 0; i < substit_buffer.size; i++) {
			substit_buffer.entries[i] = INT32_MIN;
		}
		all_query_instances_rec(0, query, table, -1);
	}
}

// Note that substit_buffer.entries has been set up already
void fixed_const_query_instances(samp_query_t *query, samp_table_t *table,
		int32_t atom_index) {
	all_query_instances_rec(0, query, table, atom_index);
}

/*
 * When new constants are introduced, may need to add new atoms
 */
void create_new_const_atoms(int32_t cidx, int32_t csort, samp_table_t *table) {
	sort_table_t *sort_table = &table->sort_table;
	pred_table_t *pred_table = &table->pred_table;
	pred_entry_t *pentry;
	int32_t i, j, pval, arity;
	samp_atom_t *atom;

	if (lazy_mcsat()) {
		return;
	}
	for (i = 0; i < pred_table->evpred_tbl.num_preds; i++) {
		pentry = &pred_table->evpred_tbl.entries[i];
		pval = 2 * i;
		arity = pentry->arity;
		atom_buffer_resize(arity);
		atom = (samp_atom_t *) atom_buffer.data;
		atom->pred = pred_val_to_index(pval);
		for (j = 0; j < arity; j++) {
			if (subsort_p(csort, pentry->signature[j], sort_table)) {
				atom->args[j] = cidx;
				new_const_atoms_at(0, j, arity, pentry->signature, atom, table);
			}
		}
	}
	for (i = 0; i < pred_table->pred_tbl.num_preds; i++) {
		pentry = &pred_table->pred_tbl.entries[i];
		pval = (2 * i) + 1;
		arity = pentry->arity;
		atom_buffer_resize(arity);
		atom = (samp_atom_t *) atom_buffer.data;
		atom->pred = pred_val_to_index(pval);
		for (j = 0; j < arity; j++) {
			if (subsort_p(csort, pentry->signature[j], sort_table)) {
				atom->args[j] = cidx;
				new_const_atoms_at(0, j, arity, pentry->signature, atom, table);
			}
		}
	}
}

/*
 * Called by MCSAT when a new constant is added.
 * Generates all new instances of rules involving this constant.
 */
void create_new_const_rule_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index) {
	// FIXME atom_index is always 0?
	assert(atom_index == 0);
	sort_table_t *sort_table = &(table->sort_table);
	rule_table_t *rule_table = &(table->rule_table);
	int32_t i, j, k;

	for (i = 0; i < rule_table->num_rules; i++) {
		samp_rule_t *rule = rule_table->samp_rules[i];
		for (j = 0; j < rule->num_vars; j++) {
			if (subsort_p(csort, rule->vars[j]->sort_index, sort_table)) {
				// Set the substit_buffer - no need to resize, as there are no new rules
				substit_buffer.entries[j] = constidx;
				// Make sure all other entries are empty
				for (k = 0; k < rule->num_vars; k++) {
					if (k != j) {
						substit_buffer.entries[j] = INT32_MIN;
					}
				}
				fixed_const_rule_instances(rule, table, atom_index);
			}
		}
	}
}

void create_new_const_query_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index) {
	query_table_t *query_table = &(table->query_table);
	samp_query_t *query;
	int32_t i, j, k;

	for (i = 0; i < query_table->num_queries; i++) {
		query = query_table->query[i];
		for (j = 0; j < query->num_vars; j++) {
			if (query->vars[j]->sort_index == csort) {
				// Set the substit_buffer - first resize if necessary
				substit_buffer_resize(query->num_vars);
				substit_buffer.entries[j] = constidx;
				// Make sure all other entries are empty
				for (k = 0; k < query->num_vars; k++) {
					if (k != j) {
						substit_buffer.entries[j] = INT32_MIN;
					}
				}
				fixed_const_query_instances(query, table, atom_index);
			}
		}
	}
}

/*
 * Given a ground atom and a literal from a rule, this returns true if the
 * literal matches - ignores sign, and finds consistent binding for the rule
 * variables - we don't want p(a, b) to match p(V, V).
 * Side effect: substit_buffer has the (partial) matching substitution
 * Of course, this is only useful if this function returns true.
 */
static bool match_atom_in_rule_atom(samp_atom_t *atom, rule_literal_t *lit,
		int32_t arity) {
	// Don't know the rule this literal came from, so we don't actually know
	// the number of variables - we just use the whole substit buffer, which
	// is assumed to be large enough.
	int32_t i, varidx;

	// First check that the preds match
	if (atom->pred != lit->atom->pred) {
		return false;
	}
	// Initialize the substit_buffer
	for (i = 0; i < substit_buffer.size; i++) {
		substit_buffer.entries[i] = INT32_MIN;
	}
	// Now go through comparing the args of the atoms
	for (i = 0; i < arity; i++) {
		if (lit->atom->args[i].kind == constant
				|| lit->atom->args[i].kind == integer) {
			// Constants must be equal
			if (atom->args[i] != lit->atom->args[i].value) {
				return false;
			}
		} else {
			// It's a variable
			varidx = lit->atom->args[i].value;
			assert(substit_buffer.size > varidx); // Just in case
			if (substit_buffer.entries[varidx] == INT32_MIN) {
				// Variable not yet set, simply set it
				substit_buffer.entries[varidx] = atom->args[i];
			} else {
				// Already set, make sure it's the same value
				if (substit_buffer.entries[varidx] != atom->args[i]) {
					return false;
				}
			}
		}
	}
	return true;
}

/* 
 * Activates an atom, i.e., sets the active flag and activates the relevant
 * clauses.
 */
int32_t activate_atom(samp_table_t *table, int32_t atom_index) {
	atom_table_t *atom_table = &table->atom_table;
	assert(!atom_table->active[atom_index]);
	atom_table->active[atom_index] = true;

	activate_rules(atom_index, table);
	return 0;
}

/*
 * [lazy only] Activates the rules relevant to a given atom
 *
 * e.g., Fr(x,y) and Sm(x) implies Sm(y), i.e., ~Fr(x,y) or ~Sm(x) or Sm(y).
 * If Sm(A) is activated (to be true) at some stage, do we need to activate all
 * the clauses related to it? No. We consider two cases:
 *
 * If y\A, nothing changed, nothing will be activated.
 *
 * If x\A, if there is some B, so that both ~Fr(A,B) and Sm(B) are falsifiable,
 * that is, Fr(A,B) is active or true by default, and Sm(B) is active or false
 * by default, [x\A,y\B] will be activated.
 *
 * How do we do it? We index (TODO haven't implemented yet) the active atoms by
 * each argument. e.g., Fr(A,?) or Fr(?,B). If a literal is activated to a
 * satisfied value, e.g., the y\A case above, do nothing. If a literal is
 * activated to an unsatisfied value, e.g., the x\A case, we check the active
 * atoms of Fr(x,y) indexed by Fr(A,y).  Since Fr(x,y) has a default value of
 * false that makes the literal satisfied, only the active atoms of Fr(x,y) may
 * relate to a unsat clause.  Then we get only a small subset of substitution
 * of y, which can be used to verify the last literal (Sm(y)) easily.  (See the
 * implementation details section in Poon and Domingos 08)
 *
 * In the beginning of each step of MC-SAT, we reselect the live clauses from
 * the currently ACTIVE sat clauses based on their weights. If a clause is
 * later activated during the sample SAT, a random test is conducted to decide
 * whether it should be put into the live clause set.

 * What would be the case of clauses with negative weight?  Or more general,
 * any formulas with positive or negative weights?  MC-SAT actually works for
 * any FOL formulas, not just clauses. So if a input formula has a negative
 * weight, we could nagate the formula and the weight at the same time, and 
 * then deal with the positive weight formula (in cnf form).
 */

void activate_rules(int32_t atom_index, samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	pred_table_t *pred_table = &table->pred_table;
	rule_table_t *rule_table = &table->rule_table;
	pred_tbl_t *pred_tbl = &pred_table->pred_tbl;
	samp_atom_t *atom;
	samp_rule_t *rule_entry;
	int32_t i, j, predicate, arity, num_rules, *rules;

	atom = atom_table->atom[atom_index];
	predicate = atom->pred;
	//pred_entry_t *pred_entry = get_pred_entry(pred_table, predicate);
	arity = pred_arity(predicate, pred_table);
	num_rules = pred_tbl->entries[predicate].num_rules;
	rules = pred_tbl->entries[predicate].rules;

	assert(valid_table(table));

	for (i = 0; i < num_rules; i++) {
		rule_entry = rule_table->samp_rules[rules[i]];
		substit_buffer_resize(rule_entry->num_vars);
		for (j = 0; j < rule_entry->num_lits; j++) {
//			cprintf(6, "Checking whether to activate rule %"PRId32", literal %"PRId32"\n",
//					rules[i], j);
			if (match_atom_in_rule_atom(atom, rule_entry->literals[j], arity)
					/* if the literal was unsat by default, do nothing */
					/* TODO this condition is not right when looking for initial unsat clauses */
					//&& is_neg(rule_entry->literals[j]) != pred_default_value(pred_entry)
					) {
				/* then substit_buffer contains the matching substitution */
				fixed_const_rule_instances(rule_entry, table, atom_index);
			}
		}
	}
}

/*
 * Adds an atom to the atom_table, returns the index.
 * TODO: what is top_p. Guess: whether to instantiate related clauses
 */
int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p) {
	atom_table_t *atom_table = &(table->atom_table);
	pred_table_t *pred_table = &(table->pred_table);
	clause_table_t *clause_table = &(table->clause_table);
	rule_table_t *rule_table = &(table->rule_table);
	int32_t i;
	int32_t predicate = atom->pred;
	int32_t arity = pred_arity(predicate, pred_table);
	samp_bvar_t current_atom_index;
	array_hmap_pair_t *atom_map;
	samp_rule_t *rule;

	assert(valid_table(table));

	atom_map = array_size_hmap_find(&(atom_table->atom_var_hash), arity + 1, //+1 for pred
			(int32_t *) atom);

	if (atom_map != NULL) {
		return atom_map->val; // already exists;
	}

	if (get_verbosity_level() >= 5) {
		printf("[add_internal_atom] Adding internal atom ");
		print_atom_now(atom, table);
		printf("\n");
	}

	assert(valid_table(table));
	atom_table_resize(atom_table, clause_table);
	assert(valid_table(table));

	current_atom_index = atom_table->num_vars++;
	samp_atom_t * current_atom = (samp_atom_t *) safe_malloc((arity + 1) * sizeof(int32_t));
	current_atom->pred = predicate;
	for (i = 0; i < arity; i++) {
		current_atom->args[i] = atom->args[i];
	}
	atom_table->atom[current_atom_index] = current_atom;
	// Adding an atom and activating atom are two different operations now
	atom_table->active[current_atom_index] = false;
	// Keep track of the number of samples when atom was introduced - these
	// will be subtracted in computing probs
	atom_table->sampling_nums[current_atom_index] = atom_table->num_samples;

	// insert the atom into the hash table
	atom_map = array_size_hmap_get(&(atom_table->atom_var_hash), arity + 1,
			(int32_t *) current_atom);
	atom_map->val = current_atom_index;

	if (pred_epred(predicate)) { // closed world assumption
		atom_table->assignments[0][current_atom_index] = v_db_false;
		atom_table->assignments[1][current_atom_index] = v_db_false;
	} else { // set pmodel to 0
		atom_table->pmodel[current_atom_index] = 0; //atom_table->num_samples/2;
		atom_table->assignments[0][current_atom_index] = v_false;
		atom_table->assignments[1][current_atom_index] = v_false;
		atom_table->num_unfixed_vars++;
	}
	add_atom_to_pred(pred_table, predicate, current_atom_index);
	init_clause_list(&clause_table->watched[pos_lit(current_atom_index)]);
	init_clause_list(&clause_table->watched[neg_lit(current_atom_index)]);
	assert(valid_table(table));

	if (top_p) {
		pred_entry_t *entry = get_pred_entry(pred_table, predicate);
		for (i = 0; i < entry->num_rules; i++) {
			rule = rule_table->samp_rules[entry->rules[i]];
			all_rule_instances_rec(0, rule, table, current_atom_index);
		}
	}

	return current_atom_index;
}

/*
 * Parse an integer
 * TODO what's the difference with str2int? 
 */
static int32_t parse_int(char *str, int32_t *val) {
	char *p = str;
	for (p = (p[0] == '+' || p[0] == '-') ? &p[1] : p; isdigit(*p); p++);
	if (*p == '\0') {
		*val = str2int(str);
	} else {
		mcsat_err("\nInvalid integer %s.", str);
		return -1;
	}
	return 0;
}

/* Adds an input atom to the table */
int32_t add_atom(samp_table_t *table, input_atom_t *current_atom) {
	const_table_t *const_table = &table->const_table;
	pred_table_t *pred_table = &table->pred_table;
	sort_table_t *sort_table = &table->sort_table;
	sort_entry_t entry;

	char * in_predicate = current_atom->pred;
	int32_t pred_id = pred_index(in_predicate, pred_table);
	if (pred_id == -1) {
		mcsat_err("\nPredicate %s is not declared", in_predicate);
		return -1;
	}
	int32_t predicate = pred_val_to_index(pred_id);
	int32_t i;
	int32_t *psig, intval, intsig;
	int32_t arity = pred_arity(predicate, pred_table);

	atom_buffer_resize(arity);
	samp_atom_t *atom = (samp_atom_t *) atom_buffer.data;
	atom->pred = predicate;
	assert(atom->pred == predicate);
	psig = pred_signature(predicate, pred_table);
	new_intidx.size = 0;
	for (i = 0; i < arity; i++) {
		entry = sort_table->entries[psig[i]];
		if (entry.constants == NULL) {
			// Should be an integer
			if (parse_int(current_atom->args[i], &intval) < 0) {
				return -1;
			}
			if (intval < entry.lower_bound || intval > entry.upper_bound) {
				mcsat_err("\nInteger %s is out of bounds.", current_atom->args[i]);
				return -1;
			}
			atom->args[i] = intval;
			if (psig != NULL && entry.ints != NULL) {
				if (add_int_const(intval, &sort_table->entries[psig[i]], sort_table)) {
					// Keep track of the newly added int constants
					ivector_push(&new_intidx, i);
				}
			}
		} else {
			atom->args[i] = const_index(current_atom->args[i], const_table);
			if (atom->args[i] == -1) {
				mcsat_err("\nConstant %s is not declared.", current_atom->args[i]);
				return -1;
			}
		}
	}
	int32_t result = add_internal_atom(table, atom, true);

	/* Now we need to deal with the newly introduced integer constants */
	for (i = 0; i < new_intidx.size; i++) {
		intval = atom->args[new_intidx.data[i]];
		intsig = psig[new_intidx.data[i]];
		create_new_const_atoms(intval, intsig, table);
		create_new_const_rule_instances(intval, intsig, table, 0);
		create_new_const_query_instances(intval, intsig, table, 0);
	}
	return result;
}

/* TODO what is this */
static bool member_frozen_preds(samp_literal_t lit, int32_t *frozen_preds,
		samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	samp_atom_t *atom;
	int32_t i, lit_pred;

	if (frozen_preds == NULL) {
		return false;
	}
	atom = atom_table->atom[var_of(lit)];
	lit_pred = atom->pred;
	for (i = 0; frozen_preds[i] != -1; i++) {
		if (frozen_preds[i] == lit_pred) {
			return true;
		}
	}
	return false;
}

/*
 * add_internal_clause is an internal operation used to add a clause.  The
 * external operation is add_rule, where the rule can be ground or quantified.
 * These clauses must already be simplified so that they do not contain any
 * ground evidence literals.
 */
int32_t add_internal_clause(samp_table_t *table, int32_t *clause,
		int32_t num_lits, int32_t *frozen_preds, double weight, bool indirect,
		bool add_weights) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	uint32_t i, posidx;
	samp_clause_t **samp_clauses, *entry;
	samp_literal_t lit;
	array_hmap_pair_t *clause_map;
	bool negs_all_true;

	if (indirect) {
		int_array_sort(clause, num_lits);
		clause_map = array_size_hmap_find(&(clause_table->clause_hash), num_lits,
				(int32_t *) clause);
		if (clause_map == NULL) {
			clause_table_resize(clause_table, num_lits);
			int32_t index = clause_table->num_clauses++;
			entry = (samp_clause_t *)
				safe_malloc(sizeof(samp_clause_t) + sizeof(bool *) + num_lits * sizeof(int32_t));
			entry->frozen = (bool *) safe_malloc(num_lits * sizeof(bool));
			samp_clauses = clause_table->samp_clauses;
			samp_clauses[index] = entry;
			entry->numlits = num_lits;
			entry->weight = weight;
			entry->link = NULL;
			for (i = 0; i < num_lits; i++) {
				entry->frozen[i] = member_frozen_preds(clause[i], frozen_preds, table);
				entry->disjunct[i] = clause[i];
			}
			clause_map = array_size_hmap_get(&clause_table->clause_hash,
					num_lits, (int32_t *) entry->disjunct);
			clause_map->val = index;
			//cprintf(5, "Added clause %"PRId32"\n", index);

			if (lazy_mcsat()) {
				push_newly_activated_clause(index, table);
			}

			return index;
		} else {
			if (add_weights) {
				//Add the weight to the existing clause
				samp_clauses = clause_table->samp_clauses;
				if (DBL_MAX - weight >= samp_clauses[clause_map->val]->weight) {
					samp_clauses[clause_map->val]->weight += weight;
				} else {
					if (samp_clauses[clause_map->val]->weight != DBL_MAX) {
						mcsat_warn("Weight overflows");
					}
					samp_clauses[clause_map->val]->weight = DBL_MAX;
				}
				// TODO: do we need a similar check for negative weights?
			}
			return clause_map->val;
		}
	} else {
		/* Clause only involves direct predicates - just set positive literal to true
		 * FIXME what is this scenario? */
		assert(false);
		negs_all_true = true;
		posidx = -1;
		for (i = 0; i < num_lits; i++) {
			lit = clause[i];
			if (is_neg(lit)) {
				// Check that atom is true
				if (!assigned_true(atom_table->assignment[var_of(lit)])) {
					negs_all_true = false;
					break;
				}
			} else {
				if (assigned_true(atom_table->assignment[var_of(lit)])) {
					// Already true - exit
					break;
				} else {
					posidx = i;
				}
			}
		}
		if (negs_all_true && posidx != -1) {
			// Set assignment to fixed_true
			atom_table->assignments[0][var_of(clause[posidx])] = v_fixed_true;
			atom_table->assignments[1][var_of(clause[posidx])] = v_fixed_true;
		}
		return -1;
	}
}

/*
 * add_clause is used to add an input clause with named atoms and constants.
 * the disjunct array is assumed to be NULL terminated; in_clause is non-NULL.
 */
int32_t add_clause(samp_table_t *table, input_literal_t **in_clause,
		double weight, char *source, bool add_weights) {
	atom_table_t *atom_table = &table->atom_table;
	int32_t i, atom, length, clause_index;
	bool found_indirect;

	length = 0;
	while (in_clause[length] != NULL) {
		length++;
	}

	clause_buffer_resize(length);

	found_indirect = false;
	for (i = 0; i < length; i++) {
		atom = add_atom(table, in_clause[i]->atom);

		if (atom == -1) {
			mcsat_err("\nBad atom");
			return -1;
		}
		if (in_clause[i]->neg) {
			clause_buffer.data[i] = neg_lit(atom);
		} else {
			clause_buffer.data[i] = pos_lit(atom);
		}
		if (atom_table->atom[atom]->pred > 0) {
			found_indirect = true;
		}
	}
	clause_index = add_internal_clause(table, clause_buffer.data, length, NULL,
			weight, found_indirect, add_weights);
	if (clause_index != -1) {
		add_source_to_clause(source, clause_index, weight, table);
	}
	return clause_index;
}

