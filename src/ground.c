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
 * 0: false; 1: true; FIXME -1: both (e.g., >)
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
 * 0: false; 1: true; FIXME -1: both (e.g., >)
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
 * 0: false; 1: true
 */
static int32_t rule_clause_default_value(rule_clause_t *rule_clause,
		pred_table_t *pred_table) {
	int32_t default_value = 0, i;
	for (i = 0; i < rule_clause->num_lits; i++) {
		int32_t literal_value = rule_literal_default_value(rule_clause->literals[i], 
				pred_table);
		if (literal_value == 1) {
			default_value = 1;
		}
	}
	return default_value;
}

/*
 * -1: indirect active atom; 
 *  0: direct or inactive false; 
 *  1: direct or inactive true.
 *
 * The returned value is different from the classification of truth values of
 * an samp_atom, because here we have not ground the atom yet, we usually care
 * about only active/inactive status of the atom, or the direct value if the
 * atom is a direct predicate.
 */
static int32_t get_rule_atom_tval(rule_atom_t *ratom, substit_entry_t *substs,
		samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t predicate = ratom->pred;
	int32_t arity = rule_atom_arity(ratom, pred_table);

	atom_buffer_resize(arity);
	samp_atom_t *satom = (samp_atom_t *) atom_buffer.data;
	rule_atom_to_samp_atom(satom, ratom, arity, substs);

	/* rule_inst_atom not added into the table yet */
	bool atom_value;
	if (ratom->builtinop > 0) {
		atom_value = call_builtin(ratom->builtinop, builtin_arity(ratom->builtinop),
				satom->args);
	}
	else {
		array_hmap_pair_t *atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
				arity + 1, (int32_t *)satom);
		if (atom_map == NULL || !atom_table->active[atom_map->val]) { /* if inactive */
			atom_value = rule_atom_default_value(ratom, pred_table);
		}
		else if (predicate <= 0) { /* for direct predicate */
			atom_value = assigned_true(atom_table->assignment[atom_map->val]);
		} else { /* indirect predicate has unfixed value */
			return -1; /* neither true nor false */
		}
	}
	return atom_value ? 1 : 0;
}

/*
 * -1: indirect active atom; 
 *  0: direct or inactive false; 
 *  1: direct or inactive true.
 */
static int32_t get_rule_literal_tval(rule_literal_t *rlit, substit_entry_t *substs,
		samp_table_t *table) {
	int32_t ratom_tval = get_rule_atom_tval(rlit->atom, substs, table);
	if (rlit->neg) { 
		if (ratom_tval == 0)
			return 1;
		else if (ratom_tval == 1)
			return 0;
	}
	return ratom_tval;
}

/* 
 * Converts a rule_atom to a samp_atom. If ratom is a builtinop, return NULL.
 */
samp_atom_t *rule_atom_to_samp_atom(samp_atom_t *satom, rule_atom_t *ratom, 
		int32_t arity, substit_entry_t *substs) {
	satom->pred = ratom->pred;
	int32_t i;
	for (i = 0; i < arity; i++) {
		if (ratom->args[i].kind == variable) {
			satom->args[i] = substs[ratom->args[i].value];
		} else {
			satom->args[i] = ratom->args[i].value;
		}
	}
	return satom;
}

/* Converts a rule_literal to a samp_literal */
static samp_literal_t rule_lit_to_samp_lit(rule_literal_t *rlit, substit_entry_t *substs,
		samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	bool neg = rlit->neg;
	rule_atom_t *ratom = rlit->atom;
	int32_t arity = rule_atom_arity(ratom, pred_table);
	atom_buffer_resize(arity);
	samp_atom_t *satom = (samp_atom_t *) atom_buffer.data;
	rule_atom_to_samp_atom(satom, ratom, arity, substs);
	//int32_t atom_index = samp_atom_index(satom, table);
	int32_t atom_index = add_internal_atom(table, satom, false);
	return neg? neg_lit(atom_index) : pos_lit(atom_index);
}

static samp_clause_t *rule_clause_to_samp_clause(rule_clause_t *rclause, substit_entry_t *substs,
		samp_table_t *table) {
	clause_buffer_resize(rclause->num_lits);
	int32_t num_lits = 0, i;
	for (i = 0; i < rclause->num_lits; i++) {
		rule_literal_t *rlit = rclause->literals[i];
		if (rule_atom_is_direct(rlit->atom)) {
			int32_t tval = get_rule_literal_tval(rlit, substs, table);
			assert(tval != -1);
			if (tval == 0) {
				continue; /* direct false literal */
			} else {
				num_lits = -1;
				break; /* direct true literal */
			}
		}
		samp_literal_t slit = rule_lit_to_samp_lit(rlit, substs, table);
		clause_buffer.literals[num_lits] = slit;
		num_lits++;
	}

	if (num_lits == -1) {
		return NULL;
	}

	assert(!lazy_mcsat() || num_lits > 0); /* no direct unsat clause */

	samp_clause_t *sclause = (samp_clause_t *) safe_malloc(sizeof(samp_clause_t)
			+ num_lits * sizeof(samp_literal_t));
	sclause->num_lits = num_lits;
	for (i = 0; i < num_lits; i++) {
		sclause->disjunct[i] = clause_buffer.literals[i];
	}
	return sclause;
}

static rule_inst_t *samp_rule_to_rule_instance(samp_rule_t *rule, substit_entry_t *substs,
		samp_table_t *table) {
	rule_buffer_resize(rule->num_clauses);
	int32_t num_clauses = 0, i;
	for (i = 0; i < rule->num_clauses; i++) {
		rule_clause_t *rclause = rule->clauses[i];
		samp_clause_t *sclause = rule_clause_to_samp_clause(rclause, substs, table);
		if (sclause == NULL) continue; /* direct sat clause */
		if (sclause->num_lits > 0) {
			rule_buffer.clauses[num_clauses] = sclause;
			num_clauses++;
		} else {
			safe_free(sclause);
		}
	}
	if (num_clauses == 0)
		return NULL;

	rule_inst_t *rinst = (rule_inst_t *) safe_malloc(sizeof(rule_inst_t)
			+ num_clauses * sizeof(samp_clause_t *));
	rinst->num_clauses = num_clauses;
	rinst->weight = rule->weight;
	for (i = 0; i < num_clauses; i++) {
		rinst->conjunct[i] = rule_buffer.clauses[i];
	}
	return rinst;
}

/*
 * Evaluates a builtin literal (a builtin operation with an optional negation)
 *
 * TODO: can be replaced by literal_falsifiable
 */
static bool builtin_inst_p(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t arity;
	bool builtin_result, retval;

	assert(lit->atom->builtinop != 0);
	arity = rule_atom_arity(lit->atom, pred_table);
	atom_buffer_resize(arity);
	samp_atom_t *rule_inst_atom = (samp_atom_t *) atom_buffer.data;
	rule_atom_to_samp_atom(rule_inst_atom, lit->atom, arity, substs);

	builtin_result = call_builtin(lit->atom->builtinop, arity, rule_inst_atom->args);
	retval = builtin_result ? (lit->neg ? false : true) : (lit->neg ? true : false);
	return retval;
}

/* 
 * Tests whether a literal is falsifiable, i.e., is not fixed to be satisfied 
 *
 * 0: not falsifiable
 * 1: falsifiable
 * 2: fixed unsat
 */
static int32_t literal_falsifiable(rule_literal_t *rlit, substit_entry_t *substs,
		samp_table_t *table) {
	int32_t tval = get_rule_literal_tval(rlit, substs, table);
	if (tval == -1) {
		return 1;
	} else {
		if (rule_atom_is_direct(rlit->atom)) {
			return (tval == 1) ? 0 : 2;
		} else {
			return (tval == 1) ? 0 : 1;
		}
	}
}

/* 
 * Tests whether a clause is falsifiable, i.e., is not fixed to be satisfied 
 *
 * 0: not falsifiable
 * 1: falsifiable
 * 2: fixed unsat
 */
static bool check_clause_falsifiable(rule_clause_t *clause, substit_entry_t *substs, 
		samp_table_t *table) {
	int32_t i;

	int32_t clause_val = 2;
	for (i = 0; i < clause->num_lits; i++) {
		rule_literal_t *lit = clause->literals[i];
		int32_t lit_val = literal_falsifiable(lit, substs, table);
		if (lit_val == 0) {
			return 0;
		} else if (lit_val == 1) {
			clause_val = 1;
		}
	}
	return clause_val;
}

static bool check_rule_instance_falsifiable(samp_rule_t *rule, substit_entry_t *substs,
		samp_table_t *table) {
	int32_t i, clause_val;
	bool rinst_val = false;
	for (i = 0; i < rule->num_clauses; i++) {
		clause_val = check_clause_falsifiable(rule->clauses[i], substs, table);
		if (clause_val == 2) {
			return false;
		} else if (clause_val == 1) {
			rinst_val = true;
		}
	}
	return rinst_val;
}

/* 
 * Checks whether a clause has been instantiated already.
 *
 * If it does not exist, add it into the hash set.
 */
static bool check_rule_instance_duplicate(samp_rule_t *rule, substit_entry_t *substs) {
	array_hmap_pair_t *rule_subst_map = array_size_hmap_find(&rule->subst_hash,
			rule->num_vars, (int32_t *) substs);
	if (rule_subst_map != NULL) {
		cprintf(5, "clause that has been checked or already exists!\n");
		return false;
	}
	return true;
}

static void add_rule_instance_to_rule(samp_rule_t *rule, substit_entry_t *substs) {
	int32_t *new_substs = (int32_t *) clone_memory(substs, sizeof(int32_t) * rule->num_vars);
	/* insert the subst to the hash set */
	array_size_hmap_get(&rule->subst_hash, rule->num_vars, (int32_t *) new_substs);
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
	sort_entry_t *sort_entry;
	int32_t vsort, csort, csubst;
	int32_t i;
	//bool found_indirect;

	assert(valid_table(table));

	if (get_verbosity_level() >= 5) {
		printf("[substit_rule] instantiating rule ");
		print_rule_substit(rule, substit_buffer.entries, table);
		printf("\n");
	}

	/* check if the constants are compatible with the sorts */
	for (i = 0; i < rule->num_vars; i++) {
		csubst = substs[i];
		/* Not enough constants, i given, rule->num_vars required */
		assert(csubst != INT32_MIN);
		vsort = rule->vars[i]->sort_index;
		sort_entry = &sort_table->entries[vsort];
		if (sort_entry->constants == NULL) { 
			/* It's an integer; check that its value is within the sort */
			assert(sort_entry->lower_bound <= csubst
					&& csubst <= sort_entry->upper_bound);
		} else {
			csort = const_sort_index(substs[i], const_table);
			/* Constant/variable sorts do not match */
			assert(subsort_p(csort, vsort, sort_table));
		}
	}
	/* Too many constants, i given, rule->num_vars required */
	assert(substs == NULL || substs[i] == INT32_MIN);

	if (lazy_mcsat() && !check_rule_instance_duplicate(rule, substs)) {
		return -1;
	}

	if (lazy_mcsat() && !check_rule_instance_falsifiable(rule, substs, table)) {
		return -1;
	}

	assert(valid_table(table));

	if (lazy_mcsat()) {
		add_rule_instance_to_rule(rule, substs);
	}

	rule_inst_t *rinst = samp_rule_to_rule_instance(rule, substs, table);
	if (rinst == NULL)
		return -1;
	
	int32_t clsidx = add_internal_rule_instance(table, rinst, true, true);
	if (get_verbosity_level() >= 5) {
		if (clsidx >= 0) {
			printf("[substit_rule] succeeded.\n");
		} else {
			printf("[substit_rule] failed.\n");
		}
	}

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
		substit_rule(rule, substit_buffer.entries, table);
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
 * direct predicate or builtinop, the number of non-default valued atoms; for a
 * indirect predicate, the number of active atoms.
 *
 * TODO can be more precise
 */
static int32_t estimate_num_samp_atoms(samp_rule_t *rule, rule_atom_t *rule_atom,
		samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	sort_table_t *sort_table = &table->sort_table;

	int32_t arity = rule_atom_arity(rule_atom, pred_table);
	int32_t natoms = INT32_MAX; /* the number of active atoms */
	
	atom_buffer_resize(arity);
	samp_atom_t *rule_inst_atom = (samp_atom_t *) atom_buffer.data;
	rule_atom_to_samp_atom(rule_inst_atom, rule_atom, arity, substit_buffer.entries);

	if (rule_atom->builtinop > 0) {
		switch (rule_atom->builtinop) {
		case EQ:
		case NEQ:
			if (rule_inst_atom->args[0] != INT32_MIN 
					|| rule_inst_atom->args[1] != INT32_MIN) {
				natoms = 1;
			}
			else {
				int32_t sortidx = rule->vars[rule_atom->args[0].value]->sort_index;
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
 * FIXME 
 */
static int32_t estimate_num_clause_instances(samp_rule_t *rule, rule_clause_t *rule_clause,
		samp_table_t *table) {
	int32_t ncinsts = 1, natoms, i;
	for (i = 0; i < rule_clause->num_lits; i++) {
		natoms = estimate_num_samp_atoms(rule, rule_clause->literals[i]->atom, table);
		if (natoms == INT32_MAX) {
			ncinsts = INT32_MAX;
			break;
		}
		ncinsts = ncinsts * natoms;
	}
	return ncinsts;
}

/*
 * Select a literal of the rule to ground. We select a literal that has the
 * minimum number of groundings (greedy strategy). For literals whose default
 * value is true, the number of groundings is equal to the number of active
 * atoms. For literals whose default value is false, the number of groundings
 * is (approximately) equal to the number of ALL instances.
 */
static int32_t select_literal_to_ground(samp_rule_t *rule, rule_clause_t *rule_clause,
		samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t litidx = -1, min_nsubsts = INT32_MAX;

	int32_t i, nsubsts;
	for (i = 0; i < rule_clause->num_lits; i++) {
		rule_literal_t *rule_literal = rule_clause->literals[i];
		if (rule_literal->grounded) continue;

		if (rule_literal_default_value(rule_literal, pred_table) == 1) {
			/* default value is true */
			nsubsts = estimate_num_samp_atoms(rule, rule_literal->atom, table);
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

static bool subst_next_samp_atom(samp_rule_t *rule, rule_atom_t *rule_atom, 
		samp_atom_t *rinst_atom, substit_entry_t *substs, 
		samp_table_t *table, int32_t *index) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	sort_table_t *sort_table = &table->sort_table;
	
	int32_t j, arity;
	/* for a direct predicate or builtinop, check all non-default value atoms;
	 * for a indirect predicate, check all active atoms. */
	if (rule_atom->builtinop > 0) {
		switch (rule_atom->builtinop) {
		case EQ:
		case NEQ:
			if (rinst_atom->args[0] != INT32_MIN && rinst_atom->args[1] != INT32_MIN) {
				if (rinst_atom->args[0] == rinst_atom->args[1]) {
					return (*index == 0);
					//rule_literal->grounded = true;
					//smart_atom_instance_rec(rule, rule_clause, table, atom_index, 
					//		ground_unsat_clauses);
					//rule_literal->grounded = false;
				} else {
					return false;
				}
			}
			else if (rinst_atom->args[0] != INT32_MIN) {
				substs[rule_atom->args[1].value] = rinst_atom->args[0];
				return (*index == 0);
				//rule_literal->grounded = true;
				//smart_atom_instance_rec(rule, rule_clause, table, atom_index, ground_unsat_clauses);
				//rule_literal->grounded = false;
				//substit_buffer.entries[var_index] = INT32_MIN;
			}
			else if (rinst_atom->args[1] != INT32_MIN) {
				substs[rule_atom->args[0].value] = rinst_atom->args[1];
				return (*index == 0);
				//rule_literal->grounded = true;
				//smart_atom_instance_rec(rule, rule_clause, table, atom_index, ground_unsat_clauses);
				//rule_literal->grounded = false;
				//substit_buffer.entries[var_index] = INT32_MIN;
			}
			else {
				int32_t varidx0 = rule_atom->args[0].value;
				int32_t varidx1 = rule_atom->args[1].value;
				int32_t sortidx = rule->vars[varidx0]->sort_index;
				sort_entry_t *sort_entry = &sort_table->entries[sortidx];
				if (*index >= sort_entry->cardinality)
					return false;
				int32_t cons = get_sort_const(sort_entry, *index);
				substs[varidx0] = cons;
				substs[varidx1] = cons;
				return true;
				//for (i = 0; i < sort_entry->cardinality; i++) {
				//	int32_t cons = get_sort_const(sort_entry, i);
				//	substit_buffer.entries[varidx0] = cons;
				//	substit_buffer.entries[varidx1] = cons;
				//	rule_literal->grounded = true;
				//	smart_atom_instance_rec(rule, rule_clause, table, atom_index, ground_unsat_clauses);
				//	rule_literal->grounded = false;
				//	substit_buffer.entries[varidx0] = INT32_MIN;
				//	substit_buffer.entries[varidx1] = INT32_MIN;
				//}
			}
			break;
		default:
			return (*index == 0);
			//rule_literal->grounded = true;
			//smart_atom_instance_rec(rule, rule_clause, table, atom_index, ground_unsat_clauses);
			//rule_literal->grounded = false;
		}
	} else {
		pred_entry_t *pred_entry = get_pred_entry(pred_table, rule_atom->pred);
		arity = rule_atom_arity(rule_atom, pred_table);
		if (*index >= pred_entry->num_atoms)
			return false;

		samp_atom_t *satom;
		for (;;) {
			if (atom_table->active[pred_entry->atoms[*index]]) {
				satom = atom_table->atom[pred_entry->atoms[*index]];
				bool compatible = true;
				for (j = 0; j < arity; j++) {
					if (rinst_atom->args[j] != INT32_MIN &&
							rinst_atom->args[j] != satom->args[j]) {
						compatible = false;
					}
				}
				if (compatible) break;
			}
			(*index)++;
			if (*index >= pred_entry->num_atoms)
				return false;
		}
		for (j = 0; j < arity; j++) {
			if (rinst_atom->args[j] == INT32_MIN) {
				substs[rule_atom->args[j].value] = satom->args[j];
			}
		}
		return true;
		//rule_literal->grounded = true;
		//smart_atom_instance_rec(rule, rule_clause, table, atom_index, ground_unsat_clauses);
		//rule_literal->grounded = false;
		//for (j = 0; j < arity; j++) {
		//	if (rinst_atom->args[j] == INT32_MIN) {
		//		var_index = rule_atom->args[j].value;
		//		substit_buffer.entries[var_index] = INT32_MIN;
		//	}
		//}
		//}
	}
	return false;
}

static void smart_clause_instance_rec(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index);

/* Instantiate direct clauses in a rule */
static void smart_atom_instances(samp_rule_t *rule, rule_clause_t *rule_clause,
		samp_table_t *table, int32_t atom_index) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t i, j, var_index;
	for (i = 0; i < rule_clause->num_lits; i++) {
		rule_literal_t *rule_literal = rule_clause->literals[i];
		rule_atom_t *rule_atom = rule_literal->atom;

		int arity = rule_atom_arity(rule_atom, pred_table);

		samp_atom_t *rinst_atom = (samp_atom_t *) safe_malloc(sizeof(samp_atom_t)
				+ arity * sizeof(int32_t));
		rule_atom_to_samp_atom(rinst_atom, rule_atom, arity, substit_buffer.entries);

		int32_t index = 0;
		while (subst_next_samp_atom(rule, rule_atom, rinst_atom, 
					substit_buffer.entries, table, &index)) {
			rule_literal->grounded = true;
			smart_clause_instance_rec(rule, table, atom_index);
			rule_literal->grounded = false;
			index++;
		}

		/* backtrack */
		for (j = 0; j < arity; j++) {
			if (rinst_atom->args[j] == INT32_MIN) {
				var_index = rule_atom->args[j].value;
				substit_buffer.entries[var_index] = INT32_MIN;
			}
		}
	}
}

/*
 * [lazy only] Recursively instantiate atoms in a clause
 *
 * TODO: change to non-recursive
 */
static void smart_atom_instance_rec(samp_rule_t *rule, rule_clause_t *rule_clause,
		samp_table_t *table, int32_t atom_index) {
	pred_table_t *pred_table = &table->pred_table;

	int32_t litidx = select_literal_to_ground(rule, rule_clause, table);

	/* when all non-default predicates have been considered,
	 * recursively enumerate the rest variables that haven't been
	 * assigned a constant */
	if (litidx < 0) {
		if (get_verbosity_level() >= 5) {
			printf("[smart_atom_instance_rec] partially instantiated rule ");
			print_rule_substit(rule, substit_buffer.entries, table);
			printf(" ...\n");
		}

		all_rule_instances_rec(0, rule, table, atom_index);
		return;
	}

	rule_literal_t *rule_literal = rule_clause->literals[litidx];
	rule_atom_t *rule_atom = rule_literal->atom;
	int32_t j, var_index;

	int arity = rule_atom_arity(rule_atom, pred_table);

	samp_atom_t *rinst_atom = (samp_atom_t *) safe_malloc(sizeof(samp_atom_t)
			+ arity * sizeof(int32_t));
	rule_atom_to_samp_atom(rinst_atom, rule_atom, arity, substit_buffer.entries);

	int32_t index = 0;
	while (subst_next_samp_atom(rule, rule_atom, rinst_atom, 
				substit_buffer.entries, table, &index)) {
		rule_literal->grounded = true;
		smart_atom_instance_rec(rule, rule_clause, table, atom_index);
		rule_literal->grounded = false;
		index++;
	}

	/* backtrack */
	for (j = 0; j < arity; j++) {
		if (rinst_atom->args[j] == INT32_MIN) {
			var_index = rule_atom->args[j].value;
			substit_buffer.entries[var_index] = INT32_MIN;
		}
	}
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
static void smart_clause_instances(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index) {
	pred_table_t *pred_table = &table->pred_table;
	rule_clause_t *rule_clause;
	int32_t i;

	if (get_verbosity_level() >= 5) {
		printf("[smart_clause_instances] partially instantiated rule ");
		print_rule_substit(rule, substit_buffer.entries, table);
		printf(" ...\n");
	}

	for (i = 0; i < rule->num_clauses; i++) {
		rule_clause = rule->clauses[i];

		/* clauses having been grounded should have fixed value */
		assert(!rule_clause->grounded || rule_clause_is_direct(rule_clause));
		
		if (!rule_clause_is_direct(rule_clause) 
				&& rule_clause_default_value(rule_clause, pred_table) != 1) {
			break;
		}
	}
	/* There exists an unfixed clause with default value of unsat */
	if (i < rule->num_clauses) {
		all_rule_instances_rec(0, rule, table, atom_index);
		return;
	}

	for (i = 0; i < rule->num_clauses; i++) {
		rule_clause = rule->clauses[i];

		/* Skip grounded (fixed unsat by default) clauses and clauses fixed sat
		 * by default */
		if (rule_clause_is_direct(rule_clause)) continue;

		rule_clause->grounded = true;
		smart_atom_instance_rec(rule, rule_clause, table, atom_index);
		rule_clause->grounded = false;
	}
}

/*
 * Select a direct (i.e. db-fixed) clause
 */
static int32_t select_clause_to_ground(samp_rule_t *rule, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	rule_clause_t *rule_clause;
	int32_t clsidx = -1, min_nsubsts = INT32_MAX;

	int32_t i, nsubsts;
	for (i = 0; i < rule->num_clauses; i++) {
		rule_clause = rule->clauses[i];
		if (rule_clause->grounded) continue;
		if (rule_clause_is_direct(rule_clause)) {
			if (rule_clause_default_value(rule_clause, pred_table) == 0) {
				/* default is fixed false */
				nsubsts = estimate_num_clause_instances(rule, rule_clause, table);
			}
			else {
				/* all default value atoms need to be grounded */
				continue;
			}
		}
		else {
			/* Still many groundings, taken care of later.
			 * Basically, for a conjunction like A(x) & B(x), the unsat
			 * clauses include the groundings where A(x) is false and
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
 * Instantiate the clauses that are fixed unsat by default (i.e. ALL literals
 * are fixed false by default), because most rules are always unsat and don't
 * need to be considered, so we only need to instantiate the clauses that are
 * fixed sat (the active clauses).
 */
static void smart_clause_instance_rec(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index) {

	int32_t clsidx = select_clause_to_ground(rule, table); 

	if (clsidx < 0) {
		smart_clause_instances(rule, table, atom_index);
		return;
	}

	rule_clause_t *rule_clause = rule->clauses[clsidx];
	rule_clause->grounded = true;
	smart_atom_instances(rule, rule_clause, table, atom_index);
	rule_clause->grounded = false;
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
		printf("[fixed_const_rule_instances] instantiating ");
		print_rule_substit(rule, substit_buffer.entries, table);
		if (atom_index > 0) {
			printf(" for ");
			print_atom(table->atom_table.atom[atom_index], table);
		}
		printf(":\n");
	}

	smart_clause_instance_rec(rule, table, atom_index);
}

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
 * clauses), and have a weight.  substit on rules updates the rule_inst_table,
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
					atom_buffer_resize(arity);
					new_atom = (samp_atom_t *) atom_buffer.data;
					rule_atom_to_samp_atom(new_atom, lit[i][j]->atom, arity, substs);
					if (lazy_mcsat()) {
						array_hmap_pair_t *atom_map;
						atom_map = array_size_hmap_find(&atom_table->atom_var_hash, 
								arity + 1, (int32_t *) new_atom);
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
				assert(atom_index >= 0);

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
			atom_buffer_resize(arity);
			new_atom = (samp_atom_t *) atom_buffer.data;
			rule_atom_to_samp_atom(new_atom, lit[i][j]->atom, arity, NULL);
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
 * instead of rule_inst_table
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

/* Note that substit_buffer.entries has been set up already */
void fixed_const_query_instances(samp_query_t *query, samp_table_t *table,
		int32_t atom_index) {
	all_query_instances_rec(0, query, table, atom_index);
}

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
	/* Don't know the rule this literal came from, so we don't actually know
	 * the number of variables - we just use the whole substit buffer, which
	 * is assumed to be large enough. */
	int32_t i, varidx;

	/* First check that the preds match */
	if (atom->pred != lit->atom->pred) {
		return false;
	}
	/* Initialize the substit_buffer */
	for (i = 0; i < substit_buffer.size; i++) {
		substit_buffer.entries[i] = INT32_MIN;
	}
	/* Now go through comparing the args of the atoms */
	for (i = 0; i < arity; i++) {
		if (lit->atom->args[i].kind == constant
				|| lit->atom->args[i].kind == integer) {
			/* Constants must be equal */
			if (atom->args[i] != lit->atom->args[i].value) {
				return false;
			}
		} else {
			/* It's a variable */
			varidx = lit->atom->args[i].value;
			assert(substit_buffer.size > varidx); /* Just in case */
			if (substit_buffer.entries[varidx] == INT32_MIN) {
				/* Variable not yet set, simply set it */
				substit_buffer.entries[varidx] = atom->args[i];
			} else {
				/* Already set, make sure it's the same value */
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
	samp_rule_t *rule_entry;
	samp_atom_t *atom;
	int32_t i, j, k, predicate, arity, num_rules, *rules;

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
		for (j = 0; j < rule_entry->num_clauses; j++) {
			rule_clause_t *rule_clause = rule_entry->clauses[j];
			for (k = 0; k < rule_clause->num_lits; k++) {
				rule_literal_t *rule_literal = rule_clause->literals[k];
				if (match_atom_in_rule_atom(atom, rule_literal, arity)) {
					/* then substit_buffer contains the matching substitution */
					rule_literal->grounded = true;
					fixed_const_rule_instances(rule_entry, table, atom_index);
					rule_literal->grounded = false;
				}
			}
		}
	}
}

int32_t add_internal_atom(samp_table_t *table, samp_atom_t *atom, bool top_p) {
	atom_table_t *atom_table = &table->atom_table;
	pred_table_t *pred_table = &table->pred_table;
	rule_table_t *rule_table = &table->rule_table;
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
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
	atom_table_resize(atom_table, rule_inst_table);
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
	init_clause_list(&rule_inst_table->watched[pos_lit(current_atom_index)]);
	init_clause_list(&rule_inst_table->watched[neg_lit(current_atom_index)]);
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

//static bool member_frozen_preds(samp_literal_t lit, int32_t *frozen_preds,
//		samp_table_t *table) {
//	atom_table_t *atom_table = &table->atom_table;
//	samp_atom_t *atom;
//	int32_t i, lit_pred;
//
//	if (frozen_preds == NULL) {
//		return false;
//	}
//	atom = atom_table->atom[var_of(lit)];
//	lit_pred = atom->pred;
//	for (i = 0; frozen_preds[i] != -1; i++) {
//		if (frozen_preds[i] == lit_pred) {
//			return true;
//		}
//	}
//	return false;
//}

/*
 * Don't need to check duplication, because we can allow the same rule
 * instances with additive weight. The rule instance is created by the caller
 * and will be added into the table, so the caller does not need to free the
 * rule instance.
 */
int32_t add_internal_rule_instance(samp_table_t *table, rule_inst_t *entry,
		bool indirect, bool add_weights) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	//atom_table_t *atom_table = &table->atom_table;
	//uint32_t i, posidx;
	//samp_literal_t lit;
	//array_hmap_pair_t *clause_map;
	//bool negs_all_true;

	//if (indirect) {
	//int_array_sort(clause, num_lits);
	//clause_map = array_size_hmap_find(&(clause_table->clause_hash), num_lits,
	//		(int32_t *) clause);
	//if (clause_map == NULL) {
	rule_inst_table_resize(rule_inst_table);
	int32_t index = rule_inst_table->num_rule_insts++;
	rule_inst_table->rule_insts[index] = entry;
	//entry = (rule_inst_t *)
	//	safe_malloc(sizeof(samp_clause_t) + sizeof(bool *) + num_lits * sizeof(int32_t));
	//entry->frozen = (bool *) safe_malloc(num_lits * sizeof(bool));
	//samp_clauses = clause_table->samp_clauses;
	//samp_clauses[index] = entry;
	//entry->numlits = num_lits;
	//entry->weight = weight;
	//entry->link = NULL;
	//for (i = 0; i < num_lits; i++) {
	//	entry->frozen[i] = member_frozen_preds(clause[i], frozen_preds, table);
	//	entry->disjunct[i] = clause[i];
	//}
	//clause_map = array_size_hmap_get(&clause_table->clause_hash,
	//		num_lits, (int32_t *) entry->disjunct);
	//clause_map->val = index;

	cprintf(5, "Added rule instance %"PRId32"\n", index);

	if (lazy_mcsat()) {
		push_newly_activated_rule_instance(index, table);
	}

	return index;
	//} else {
	//	if (add_weights) {
	//		//Add the weight to the existing clause
	//		samp_clauses = clause_table->samp_clauses;
	//		if (DBL_MAX - weight >= samp_clauses[clause_map->val]->weight) {
	//			samp_clauses[clause_map->val]->weight += weight;
	//		} else {
	//			if (samp_clauses[clause_map->val]->weight != DBL_MAX) {
	//				mcsat_warn("Weight overflows");
	//			}
	//			samp_clauses[clause_map->val]->weight = DBL_MAX;
	//		}
	//		/* TODO: do we need a similar check for negative weights? */
	//	}
	//	return clause_map->val;
	//}
	//} else {
	//	/* Clause only involves direct predicates - just set positive literal to true
	//	 * FIXME what is this scenario? */
	//	negs_all_true = true;
	//	posidx = -1;
	//	for (i = 0; i < num_lits; i++) {
	//		lit = clause[i];
	//		if (is_neg(lit)) {
	//			// Check that atom is true
	//			if (!assigned_true(atom_table->assignment[var_of(lit)])) {
	//				negs_all_true = false;
	//				break;
	//			}
	//		} else {
	//			if (assigned_true(atom_table->assignment[var_of(lit)])) {
	//				// Already true - exit
	//				break;
	//			} else {
	//				posidx = i;
	//			}
	//		}
	//	}
	//	if (negs_all_true && posidx != -1) {
	//		// Set assignment to fixed_true
	//		atom_table->assignments[0][var_of(clause[posidx])] = v_fixed_true;
	//		atom_table->assignments[1][var_of(clause[posidx])] = v_fixed_true;
	//	}
	//	return -1;
	//}
}

