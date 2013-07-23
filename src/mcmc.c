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
#include "lazysat.h"
#include "samplesat.h"
#include "mcmc.h"

#include "weight_learning.h"
#include "training_data.h"

#ifdef _WIN32
#include <windows.h>
// #include <wincrypt.h>
#endif

#ifdef MINGW
/*
 * Need some version of random()
 * rand() exists on mingw but random() does not
 */
static inline int random(void) {
	return rand();
}
#endif

/* 
 * The next step is to define the main samplesat procedure.  Here we have placed
 * some clauses among the dead clauses, and are trying to satisfy the live ones.
 * The first step is to perform negative_unit propagation.  Then we pick a
 * random assignment to the remaining variables.

 * Scan the clauses, if there is a fixed-satisfied, it goes in sat_clauses.
 * If there is a fixed-unsat, we have a conflict.
 * If there is a sat clause, we place it in watched list.
 * If the sat clause has a fixed-propagation, then we mark the variable
 * as having a fixed truth value, and move the clause to sat_clauses, while
 * link-propagating this truth value.  When scanning, look for unit propagation in
 * unsat clause list.

 * We then randomly flip a variable, and move all the resulting satisfied clauses
 * to the sat/watched list using the scan_unsat_clauses operation.
 * When this is done, if we have no unsat clauses, we stop.
 * Otherwise, either pick a random unfixed variable and flip it,
 * or pick a random unsat clause and a flip either a random variable or the
 * minimal dcost variable and flip it.  Keep flipping until there are no unsat clauses.

 * First scan the dead clauses to move the satisfied ones to the appropriate
 * lists.  Then move all the unsatisfied clauses to the dead list.
 * Then select-and-move the satisfied clauses to the unsat list.
 * Pick a random truth assignment.  Then repeat the scan-propagate loop.
 * Then selectively, either pick an unfixed variable and flip and scan, or
 * pick an unsat clause and, selectively, a variable and flip and scan.
 */

clause_buffer_t rule_atom_buffer = { 0, NULL };

extern clause_buffer_t atom_buffer;

/* Pushes a clause to some clause linked list */
void inline push_clause(samp_clause_t *clause, samp_clause_t **list) {
	clause->link = *list;
	*list = clause;
}

/* Pushes a clause to the negative unit clause linked list */
void inline push_negative_or_unit_clause(clause_table_t *clause_table, uint32_t i) {
	push_clause(clause_table->samp_clauses[i], &clause_table->negative_or_unit_clauses);
//	clause_table->samp_clauses[i]->link = clause_table->negative_or_unit_clauses;
//	clause_table->negative_or_unit_clauses = clause_table->samp_clauses[i];
}

/* Pushes a clause to the unsat clause linked list */
void inline push_unsat_clause(clause_table_t *clause_table, uint32_t i) {
	push_clause(clause_table->samp_clauses[i], &clause_table->unsat_clauses);
//	clause_table->samp_clauses[i]->link = clause_table->unsat_clauses;
//	clause_table->unsat_clauses = clause_table->samp_clauses[i];
	clause_table->num_unsat_clauses++;
}

void inline push_sat_clause(clause_table_t *clause_table, uint32_t i) {
	push_clause(clause_table->samp_clauses[i], &clause_table->sat_clauses);
}

ivector_t new_intidx = {0, 0, NULL};

/* Parse an integer */
int32_t parse_int(char *str, int32_t *val) {
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

	atom_buffer_resize(&atom_buffer, arity);
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
	if (get_verbosity_level() >= 0) {
		printf("add_atom: Adding internal atom ");
		print_atom_now(atom, table);
		printf("\n");
	}
	int32_t result = add_internal_atom(table, atom, true);
	// Now we need to deal with the newly introduced integer constants
	for (i = 0; i < new_intidx.size; i++) {
		intval = atom->args[new_intidx.data[i]];
		intsig = psig[new_intidx.data[i]];
		create_new_const_atoms(intval, intsig, table);
		create_new_const_rule_instances(intval, intsig, table, 0);
		create_new_const_query_instances(intval, intsig, table, 0);
	}
	return result;
}

/*
 * Adds an atom to the atom_table, returns the index
 *
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

	//assert(valid_table(table));
	atom_table_resize(atom_table, clause_table);
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

	if (pred_epred(predicate)) { //closed world assumption
		atom_table->assignment[0][current_atom_index] = v_fixed_false;
		atom_table->assignment[1][current_atom_index] = v_fixed_false;
	} else {//set pmodel to 0
		atom_table->pmodel[current_atom_index] = 0; //atom_table->num_samples/2;
		atom_table->assignment[0][current_atom_index] = v_false;
		atom_table->assignment[1][current_atom_index] = v_false;
		atom_table->num_unfixed_vars++;
	}
	add_atom_to_pred(pred_table, predicate, current_atom_index);
	clause_table->watched[pos_lit(current_atom_index)] = NULL;
	clause_table->watched[neg_lit(current_atom_index)] = NULL;
	//assert(valid_table(table));

	if (top_p) {
		pred_entry_t *entry = get_pred_entry(pred_table, predicate);
		for (i = 0; i < entry->num_rules; i++) {
			rule = rule_table->samp_rules[entry->rules[i]];
			all_rule_instances_rec(0, rule, table, current_atom_index);
		}
	}

	return current_atom_index;
}

void add_source_to_assertion(char *source, int32_t atom_index,
		samp_table_t *table) {
	source_table_t *source_table = &table->source_table;
	source_entry_t *source_entry;
	int32_t numassn, srcidx;

	for (srcidx = 0; srcidx < source_table->num_entries; srcidx++) {
		if (strcmp(source, source_table->entry[srcidx]->name) == 0) {
			break;
		}
	}
	if (srcidx == source_table->num_entries) {
		source_table_extend(source_table);
		source_entry = (source_entry_t *) safe_malloc(sizeof(source_entry_t));
		source_table->entry[srcidx] = source_entry;
		source_entry->name = source;
		source_entry->assertion = (int32_t *) safe_malloc(2 * sizeof(int32_t));
		source_entry->assertion[0] = atom_index;
		source_entry->assertion[1] = -1;
		source_entry->clause = NULL;
		source_entry->weight = NULL;
	} else {
		source_entry = source_table->entry[srcidx];
		if (source_entry->assertion == NULL) {
			source_entry->assertion = (int32_t *) safe_malloc(
					2 * sizeof(int32_t));
			source_entry->assertion[0] = atom_index;
			source_entry->assertion[1] = -1;
		} else {
			for (numassn = 0; source_entry->assertion[numassn] != -1; numassn++) {
			}
			source_entry->assertion = (int32_t *) safe_realloc(
					source_entry->assertion, (numassn + 2) * sizeof(int32_t));
			source_entry->assertion[numassn] = atom_index;
			source_entry->assertion[numassn + 1] = -1;
		}
	}
}

int32_t assert_atom(samp_table_t *table, input_atom_t *current_atom, char *source) {
	pred_table_t *pred_table = &table->pred_table;
	char *in_predicate = current_atom->pred;
	int32_t pred_id = pred_index(in_predicate, pred_table);

	if (pred_id == -1) {
		mcsat_err("assert: Predicate %s not found\n", in_predicate);
		return -1;
	}
	pred_id = pred_val_to_index(pred_id);
	if (pred_id >= 0) {
		mcsat_err("assert: May not assert atoms with indirect predicate %s\n",
				in_predicate);
		return -1;
	}

	int32_t atom_index = add_atom(table, current_atom);
	if (atom_index == -1) {
		return -1;
	} else {
		table->atom_table.assignment[0][atom_index] = v_fixed_true;
		table->atom_table.assignment[1][atom_index] = v_fixed_true;
		if (source != NULL) {
			add_source_to_assertion(source, atom_index, table);
		}
		return atom_index;
	}
}

bool member_frozen_preds(samp_literal_t lit, int32_t *frozen_preds,
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

//add_clause is used to add an input clause with named atoms and constants.
//the disjunct array is assumed to be NULL terminated; in_clause is non-NULL.
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

			/*
			 * TODO: For lazy MC-SAT: The clause is always satisfied for the
			 * previous assignment.  Calculate if it belongs to the subset of
			 * clauses for the current round of sample SAT
			 */
			push_newly_activated_clause(index, table);

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
		// Clause only involves direct predicates - just set positive literal to true
		// TODO what is this scenario?
		samp_truth_value_t *assignment = atom_table->current_assignment;
		negs_all_true = true;
		posidx = -1;
		for (i = 0; i < num_lits; i++) {
			lit = clause[i];
			if (is_neg(lit)) {
				// Check that atom is true
				if (!assigned_true(assignment[var_of(lit)])) {
					negs_all_true = false;
					break;
				}
			} else {
				if (assigned_true(assignment[var_of(lit)])) {
					// Already true - exit
					break;
				} else {
					posidx = i;
				}
			}
		}
		if (negs_all_true && posidx != -1) {
			// Set assignment to fixed_true
			atom_table->assignment[0][var_of(clause[posidx])] = v_fixed_true;
			atom_table->assignment[1][var_of(clause[posidx])] = v_fixed_true;
		}
		return -1;
	}
}

void add_source_to_clause(char *source, int32_t clause_index, double weight,
		samp_table_t *table) {
	source_table_t *source_table = &table->source_table;
	source_entry_t *source_entry;
	int32_t numclauses, srcidx;

	for (srcidx = 0; srcidx < source_table->num_entries; srcidx++) {
		if (strcmp(source, source_table->entry[srcidx]->name) == 0) {
			break;
		}
	}
	if (srcidx == source_table->num_entries) {
		source_table_extend(source_table);
		source_entry = (source_entry_t *) safe_malloc(sizeof(source_entry_t));
		source_table->entry[srcidx] = source_entry;
		source_entry->name = source;
		source_entry->assertion = NULL;
		source_entry->clause = (int32_t *) safe_malloc(2 * sizeof(int32_t));
		source_entry->weight = (double *) safe_malloc(2 * sizeof(double));
		source_entry->clause[0] = clause_index;
		source_entry->clause[1] = -1;
		source_entry->weight[0] = weight;
		source_entry->weight[1] = -1;
	} else {
		source_entry = source_table->entry[srcidx];
		if (source_entry->clause == NULL) {
			source_entry->clause = (int32_t *) safe_malloc(2 * sizeof(int32_t));
			source_entry->weight = (double *) safe_malloc(2 * sizeof(double));
			source_entry->clause[0] = clause_index;
			source_entry->clause[1] = -1;
			source_entry->weight[0] = clause_index;
			source_entry->weight[1] = -1;
		} else {
			for (numclauses = 0; source_entry->assertion[numclauses] != -1; numclauses++) {
			}
			source_entry->clause = (int32_t *) safe_realloc(
					source_entry->clause, (numclauses + 2) * sizeof(int32_t));
			source_entry->weight = (double *) safe_realloc(
					source_entry->weight, (numclauses + 2) * sizeof(double));
			source_entry->clause[numclauses] = clause_index;
			source_entry->clause[numclauses + 1] = -1;
			source_entry->weight[numclauses] = weight;
			source_entry->weight[numclauses + 1] = -1;
		}
	}
}

/*
 * new_samp_rule sets up a samp_rule, initializing what it can, and leaving
 * the rest to typecheck_atom.  In particular, the variable sorts are set to -1,
 * and the literals are uninitialized.
 */
samp_rule_t * new_samp_rule(input_clause_t *rule) {
	int i;
	samp_rule_t *new_rule = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
	// Allocate the vars
	new_rule->num_vars = rule->varlen;
	var_entry_t **vars = (var_entry_t **) safe_malloc(
			rule->varlen * sizeof(var_entry_t *));
	for (i = 0; i < rule->varlen; i++) {
		vars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
		vars[i]->name = str_copy(rule->variables[i]);
		vars[i]->sort_index = -1;
	}
	new_rule->vars = vars;
	// Now the literals
	new_rule->num_lits = rule->litlen;
	rule_literal_t **lits = (rule_literal_t **) safe_malloc(
			rule->litlen * sizeof(rule_literal_t *));
	for (i = 0; i < rule->litlen; i++) {
		lits[i] = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
		lits[i]->neg = rule->literals[i]->neg;
	}
	new_rule->literals = lits;
	return new_rule;
}

/*
 * Given an atom, a pred_table, an array of variables, and a const_table
 * return true if:
 *   atom->pred is in the pred_table
 *   the arities match
 *   for each arg:
 *     the arg is either in the variable array or the const table
 *     and its sort is the same as for the pred.
 * Assumes that the variable array has already been checked for:
 *   no duplicates
 *   no clashes with the const_table (this is OK if shadowing is allowed,
 *      and the variables are checked for first, for this possibility).
 * The var_sorts array is the same length as the variables array, and
 *    initialized to all -1.  As sorts are determined from the pred, the
 *    array is set to the corresponding sort index.  If the sort is already
 *    set, it is checked to see that it is the same.  Thus an error is flagged
 *    if different occurrences of a variable are used with inconsistent sorts.
 * If all goes well, a samp_atom_t is returned, with the predicate and args
 *    replaced by indexes.  Note that variables are replaced with negative indices,
 *    i.e., for variable number n, the index is -(n + 1)
 */
rule_atom_t * typecheck_atom(input_atom_t *atom, samp_rule_t *rule,
		samp_table_t *samp_table) {
	pred_table_t *pred_table = &(samp_table->pred_table);
	const_table_t *const_table = &(samp_table->const_table);
	char *pred = atom->pred;
	int32_t pred_val = pred_index(pred, pred_table);
	if (pred_val == -1) {
		mcsat_err("Predicate %s not previously declared\n", pred);
		return (rule_atom_t *) NULL;
	}
	int32_t pred_idx = pred_val_to_index(pred_val);
	pred_entry_t pred_entry;
	if (pred_idx < 0) {
		pred_entry = pred_table->evpred_tbl.entries[-pred_idx];
	} else {
		pred_entry = pred_table->pred_tbl.entries[pred_idx];
	}
	int32_t arglen = 0;
	while (atom->args[arglen] != NULL) {
		arglen++;
	}
	if (pred_entry.arity != arglen) {
		mcsat_err(
				"Predicate %s has arity %"PRId32", but given %"PRId32" args\n",
				pred, pred_entry.arity, arglen);
		return (rule_atom_t *) NULL;
	}
	int argidx;
	//  int32_t size = pred_entry.arity * sizeof(int32_t);
	// Create a new atom - note that we will need to free it if there is an error
	rule_atom_t *new_atom = (rule_atom_t *) safe_malloc(sizeof(rule_atom_t));
	new_atom->args = (rule_atom_arg_t *) safe_malloc(
			pred_entry.arity * sizeof(rule_atom_arg_t));
	new_atom->pred = pred_idx;
	for (argidx = 0; argidx < arglen; argidx++) {
		int32_t varidx = -1;
		int j;
		for (j = 0; j < rule->num_vars; j++) {
			if (strcmp(atom->args[argidx], rule->vars[j]->name) == 0) {
				varidx = j;
				break;
			}
		}
		if (varidx != -1) {
			if (rule->vars[varidx]->sort_index == -1) {
				// Sort not set, we set it to the corresponding pred sort
				rule->vars[varidx]->sort_index = pred_entry.signature[argidx];
			}
			// Check that sort matches, else it's an error
			else if (rule->vars[varidx]->sort_index
					!= pred_entry.signature[argidx]) {
				mcsat_err("Variable %s used with multiple sorts\n",
						rule->vars[varidx]->name);
				safe_free(new_atom);
				return (rule_atom_t *) NULL;
			}
			new_atom->args[argidx].kind = variable;
			new_atom->args[argidx].value = varidx;
		} else {
			// Not a variable, should be in the const_table
			int32_t constidx = const_index(atom->args[argidx], const_table);
			if (constidx == -1) {
				mcsat_err("Argument %s not found\n", atom->args[argidx]);
				safe_free(new_atom);
				return (rule_atom_t *) NULL;
			} else
				// Have a constant, check the sort
				if (const_sort_index(constidx, const_table)
						!= pred_entry.signature[argidx]) {
					mcsat_err("Constant %s has wrong sort for predicate %s\n",
							atom->args[argidx], pred);
					safe_free(new_atom);
					return (rule_atom_t *) NULL;
				}
			new_atom->args[argidx].kind = constant;
			new_atom->args[argidx].value = constidx;
		}
	} // End of arg loop
	return new_atom;
}

/* Adds an input rule into the samp_table */
int32_t add_rule(input_clause_t *rule, double weight, char *source,
		samp_table_t *samp_table) {
	rule_table_t *rule_table = &(samp_table->rule_table);
	pred_table_t *pred_table = &(samp_table->pred_table);
	int32_t i;
	// Make sure rule has variables - else it is a ground clause
	assert(rule->varlen > 0);
	// Might as well make sure it also has literals
	assert(rule->litlen > 0);
	// Need to check that variables are all used, and used consistently
	// Need to check args against predicate signatures.
	// We will use the new_rule for the variables
	samp_rule_t *new_rule = new_samp_rule(rule);
	new_rule->weight = weight;
	for (i = 0; i < rule->litlen; i++) {
		input_atom_t *in_atom = rule->literals[i]->atom;
		rule_atom_t *atom = typecheck_atom(in_atom, new_rule, samp_table);
		if (atom == NULL) {
			// Free up the earlier atoms
			int32_t j;
			for (j = 0; j < i; j++) {
				safe_free(new_rule->literals[j]->atom);
			}
			safe_free(new_rule);
			return -1;
		}
		new_rule->literals[i]->atom = atom;
	}
	// New rule is OK, now add it to the rule_table.  For now, we ignore the
	// fact that it may already be there - a future optimization is to
	// recognize duplicate rules and add their weights.
	int32_t current_rule = rule_table->num_rules;
	rule_table_resize(rule_table);
	rule_table->samp_rules[current_rule] = new_rule;
	rule_table->num_rules += 1;
	// Now loop through the literals, adding the current rule to each pred
	for (i = 0; i < rule->litlen; i++) {
		int32_t pred = new_rule->literals[i]->atom->pred;
		add_rule_to_pred(pred_table, pred, current_rule);
	}
	return current_rule;
}

bool eql_samp_atom(samp_atom_t *atom1, samp_atom_t *atom2, samp_table_t *table) {
	int32_t i, arity;

	if (atom1->pred != atom2->pred) {
		return false;
	}
	arity = pred_arity(atom1->pred, &table->pred_table);
	for (i = 0; i < arity; i++) {
		if (atom1->args[i] != atom2->args[i]) {
			return false;
		}
	}
	return true;
}

bool eql_rule_atom(rule_atom_t *atom1, rule_atom_t *atom2, samp_table_t *table) {
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

bool eql_rule_literal(rule_literal_t *lit1, rule_literal_t *lit2,
		samp_table_t *table) {
	return ((lit1->neg == lit2->neg) && eql_rule_atom(lit1->atom, lit2->atom,
				table));
}

// Checks if the queries are the same - this is essentially a syntactic
// test, though it won't care about variable names.
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

bool eql_query_instance_lits(samp_literal_t **lit1, samp_literal_t **lit2) {
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

int32_t add_query(var_entry_t **vars, rule_literal_t ***lits,
		samp_table_t *table) {
	query_table_t *query_table = &table->query_table;
	samp_query_t *query;
	int32_t i, numvars, query_index;

	for (i = 0; i < query_table->num_queries; i++) {
		if (eql_query_entries(lits, query_table->query[i], table)) {
			return i;
		}
	}
	// Now save the query in the query_table
	query_index = query_table->num_queries;
	query_table_resize(query_table);
	query = (samp_query_t *) safe_malloc(sizeof(samp_query_t));
	query_table->query[query_table->num_queries++] = query;
	for (i = 0; lits[i] != NULL; i++) {
	}
	query->num_clauses = i;
	if (vars == NULL) {
		numvars = 0;
	} else {
		for (numvars = 0; vars[numvars] != NULL; numvars++) {
		}
	}
	query->num_vars = numvars;
	query->literals = lits;
	query->vars = vars;
	query->source_index = NULL;
	// Need to create instances and add to query_instance_table
	all_query_instances(query, table);
	return query_index;
}

void retract_source(char *source, samp_table_t *table) {
	source_table_t *source_table = &table->source_table;
	clause_table_t *clause_table;
	source_entry_t *source_entry;
	int32_t i, j, srcidx, sidx, clause_idx;
	samp_clause_t *clause;
	double wt;

	for (srcidx = 0; srcidx < source_table->num_entries; srcidx++) {
		if (strcmp(source, source_table->entry[srcidx]->name) == 0) {
			break;
		}
	}
	if (srcidx == source_table->num_entries) {
		mcsat_err("\nSource %s unknown\n", source);
		return;
	}
	source_entry = source_table->entry[srcidx];
	if (source_entry->assertion != NULL) {
		// Not sure what to do here
	}
	if (source_entry->clause != NULL) {
		clause_table = &table->clause_table;
		for (i = 0; source_entry->clause[i] != -1; i++) {
			clause_idx = source_entry->clause[i];
			clause = clause_table->samp_clauses[clause_idx];
			if (source_entry->weight[i] == DBL_MAX) {
				// Need to go through all other sources and add them up
				wt = 0.0;
				for (sidx = 0; sidx < source_table->num_entries; sidx++) {
					if (sidx != srcidx && source_table->entry[sidx]->clause
							!= NULL) {
						for (j = 0; source_table->entry[sidx]->clause[j] != -1; j++) {
							if (source_table->entry[sidx]->clause[j]
									== clause_idx) {
								wt += source_table->entry[sidx]->weight[j];
								// Assume clause occurs only once for given source,
								// as we can simply sum up all such occurrences.
								break;
							}
						}
					}
				}
				clause->weight = wt;
			} else if (clause->weight != DBL_MAX) {
				// Subtract the weight of this source from the clause
				clause->weight -= source_entry->weight[i];
			} // Nothing to do if current weight is DBL_MAX
		}
	}
}

/*
 * SELECTION OF LIVE CLAUSES
 */

/*
 * Randomly select live clauses from a list:
 * - link = start of the list and *link_ptr == link
 * - all clauses in the list must be satisfied in the current assignment
 * - a clause of weight W is killed with probability exp(-|W|)
 *   (if W == DBL_MAX then it's a hard clause and it's not killed).
 */
void kill_clause_list(samp_clause_t **link_ptr, samp_clause_t *link,
		clause_table_t *clause_table) {
	samp_clause_t *next;

	while (link != NULL) {
		if (link->weight == DBL_MAX 
				|| link->weight == DBL_MIN // FIXME this case will not happen
				                           // should be in negative_unit_clauses list
				|| choose() < 1 - exp(-fabs(link->weight))) {
			// keep it
			*link_ptr = link;
			link_ptr = &link->link;
			link = link->link;
		} else {
			// move it to the dead_clauses list
			next = link->link;
			push_clause(link, &clause_table->dead_clauses);
			link = next;
		}
	}
	*link_ptr = NULL;
}

/*
 * In MC-SAT, kills some currently satisfied clauses with
 * the probability equal to 1 - exp(-w).
 *
 * The satisfied clauses are either in sat_clauses or in the
 * watched list of each active literal.
 */
void kill_clauses(samp_table_t *table) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);
	int32_t i, lit;

	//unsat_clauses is empty; need to only kill satisfied clauses
	//kill sat_clauses
	kill_clause_list(&clause_table->sat_clauses, clause_table->sat_clauses,
			clause_table);

	//kill watched clauses
	for (i = 0; i < atom_table->num_vars; i++) {
		lit = pos_lit(i);
		kill_clause_list(&clause_table->watched[lit],
				clause_table->watched[lit], clause_table);
		lit = neg_lit(i);
		kill_clause_list(&clause_table->watched[lit],
				clause_table->watched[lit], clause_table);
	}

	//TEMPERORY
	//print_live_clauses(table);
}

/*
 * Sets all the clause lists in clause_table to NULL, except for
 * the list of all active clauses.
 */
void empty_clause_lists(samp_table_t *table) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);

	clause_table->sat_clauses = NULL;
	clause_table->unsat_clauses = NULL;
	clause_table->num_unsat_clauses = 0;
	clause_table->negative_or_unit_clauses = NULL;
	clause_table->dead_negative_or_unit_clauses = NULL;
	clause_table->dead_clauses = NULL;
	uint32_t i;
	int32_t num_unfixed = 0;
	for (i = 0; i < atom_table->num_vars; i++) {
		if (!fixed_tval(atom_table->current_assignment[i])) {
			num_unfixed++;
		}
		atom_table->num_unfixed_vars = num_unfixed;
		clause_table->watched[pos_lit(i)] = NULL;
		clause_table->watched[neg_lit(i)] = NULL;
	}
}

/*
 * Pushes each of the hard clauses into either negative_or_unit_clauses
 * if weight is negative or numlits is 1, or into unsat_clauses if weight
 * is non-negative.  Non-hard clauses go into dead_negative_or_unit_clauses
 * or dead_clauses, respectively.
 */
void init_clause_lists(clause_table_t *clause_table) {
	uint32_t i;
	samp_clause_t *clause;

	for (i = 0; i < clause_table->num_clauses; i++) {
		clause = clause_table->samp_clauses[i];
		if (clause->weight == DBL_MIN || clause->weight == DBL_MAX) {
			if (clause->weight < 0 || clause->numlits == 1) {
				push_negative_or_unit_clause(clause_table, i);
			}
			else {
				push_unsat_clause(clause_table, i);
			}
		}
		else {
			if (clause->weight < 0 || clause->numlits == 1) {
				push_clause(clause_table->samp_clauses[i],
						&(clause_table->dead_negative_or_unit_clauses));
			}
			else {
				push_clause(clause_table->samp_clauses[i],
						&(clause_table->dead_clauses));
			}
		}
	}
}

/*
 * Scan the list of dead clauses:
 * - move all dead clauses that are satisfied by assignment
 *   into an appropriate watched literal list
 */
void restore_sat_dead_clauses(clause_table_t *clause_table,
		samp_truth_value_t *assignment) {
	int32_t lit, val;
	samp_clause_t *link, *next;
	samp_clause_t **link_ptr;

	link_ptr = &clause_table->dead_clauses;
	link = *link_ptr;
	while (link != NULL) {
		val = eval_clause(assignment, link);
		if (val != -1) {
			cprintf(2, "---> restoring dead clause %p (val = %"PRId32")\n",
					link, val); //BD
			// swap disjunct[0] and disjunct[val]
			lit = link->disjunct[val];
			link->disjunct[val] = link->disjunct[0];
			link->disjunct[0] = lit;
			next = link->link;
			push_clause(link, &clause_table->watched[lit]);
			//link->link = clause_table->watched[lit];
			//clause_table->watched[lit] = link;
			link = next;
			assert(assigned_true_lit(assignment, clause_table->watched[lit]->disjunct[0]));
		} else {
			cprintf(2, "---> dead clause %p stays dead (val = %"PRId32")\n",
					link, val); //BD
			*link_ptr = link;
			link_ptr = &(link->link);
			link = link->link;
		}
	}
	*link_ptr = NULL;
}

/*
 * Scan the list of dead clauses that are unit clauses or have negative weight
 * - any clause satisfied by assignment is moved into the list of (not dead)
 *   unit or negative weight clauses.
 */
void restore_sat_dead_negative_unit_clauses(clause_table_t *clause_table,
		samp_truth_value_t *assignment) {
	bool restore;
	samp_clause_t *link, *next;
	samp_clause_t **link_ptr;

	link_ptr = &clause_table->dead_negative_or_unit_clauses;
	link = *link_ptr;
	while (link != NULL) {
		if (link->weight < 0) {
			restore = (eval_clause(assignment, link) == -1);
		} else { //unit clause
			restore = (eval_clause(assignment, link) != -1);
		}
		if (restore) {
			// push to negative_or_unit_clauses
			next = link->link;
			push_clause(link, &clause_table->negative_or_unit_clauses);
			link = next;
		} else {
			// remains in the current list, move to next
			*link_ptr = link;
			link_ptr = &(link->link);
			link = link->link;
		}
	}
	*link_ptr = NULL;
}

/* A very slow function to calculate the length of a linked list */
int32_t length_clause_list(samp_clause_t *link) {
	int32_t length = 0;
	while (link != NULL) {
		link = link->link;
		length++;
	}
	return length;
}

void move_sat_to_unsat_clauses(clause_table_t *clause_table) {
	int32_t length = length_clause_list(clause_table->sat_clauses);
	samp_clause_t **link_ptr = &(clause_table->unsat_clauses);
	samp_clause_t *link = clause_table->unsat_clauses;
	while (link != NULL) {
		link_ptr = &(link->link);
		link = link->link;
	}
	*link_ptr = clause_table->sat_clauses;
	clause_table->sat_clauses = NULL;
	clause_table->num_unsat_clauses += length;
}

/*
 * FIXME: This function is not used??
 */
void move_unsat_to_dead_clauses(clause_table_t *clause_table) {
	samp_clause_t **link_ptr = &(clause_table->dead_clauses);
	samp_clause_t *link = clause_table->dead_clauses;
	while (link != NULL) {
		link_ptr = &(link->link);
		link = link->link;
	}
	*link_ptr = clause_table->unsat_clauses;
	clause_table->unsat_clauses = NULL;
	clause_table->num_unsat_clauses = 0;
}

/*
 * Prepare for a new round in mcsat:
 * - select a new set of live clauses
 * - select a random assignment
 * - return -1 if a conflict is detected by unit propagation (in the fixed clauses)
 * - return 0 otherwise
 */
int32_t reset_sample_sat(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;

	cprintf(2, "[reset_sample_sat] started ...\n");

	clear_integer_stack(&(table->fixable_stack)); //clear fixable_stack

	/*
	 * move dead clauses that are satisfied into appropriate lists
	 * so that they can be selected as live clauses in this round.
	 */
	restore_sat_dead_negative_unit_clauses(clause_table, assignment);
	restore_sat_dead_clauses(clause_table, assignment);

	/*
	 * kill selected satisfied clauses: select which clauses will
	 * be live in this round.
	 *
	 * This is partial: live unit or negative weight clauses are
	 * selected within negative_unit_propagate
	 */
	kill_clauses(table);

	if (get_verbosity_level() >= 2) {
		printf("\n[reset_sample_sat] done. %d unsat_clauses\n",
				table->clause_table.num_unsat_clauses);
		print_assignment(table);
		print_clause_table(table, -1);
	}

	return 0;
}

/*
 * Updates the counts for the query formulas
 */
void update_query_pmodel(samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	query_instance_table_t *qinst_table = &table->query_instance_table;
	samp_query_instance_t *qinst;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t num_queries, i, j, k;
//	int32_t *apmodel;
//	bool fval;

	num_queries = qinst_table->num_queries;
//	apmodel = table->atom_table.pmodel;

	for (i = 0; i < num_queries; i++) {
		// Each query instance is an array of array of literals
		qinst = qinst_table->query_inst[i];
		// Debugging information
		// First loop through the conjuncts
		// for (j = 0; qinst->lit[j] != NULL; j++) {
		//   // Now the disjuncts
		//   for (k = 0; qinst->lit[j][k] != -1; k++) {
		// 	// Print lit and assignment
		// 	if (is_neg(qinst->lit[j][k])) output("~");
		// 	print_atom(atom_table->atom[var_of(qinst->lit[j][k])], table);
		// 	assigned_true_lit(assignment, qinst->lit[j][k]) ? output(": T ") : output(": F ");
		//   }
		// }
		// printf("\n");
		// fflush(stdout);

//		fval = false;
		// First loop through the conjuncts
		for (j = 0; qinst->lit[j] != NULL; j++) {
			// Now the disjuncts
			for (k = 0; qinst->lit[j][k] != -1; k++) {
				// If any literal is true, skip the rest
				if (assigned_true_lit(assignment, qinst->lit[j][k])) {
					goto cont;
				}
			}
			// None was true, kick out
			goto done;
cont: continue;
		}
		// Each conjunct was true, so the assignment is a model - increase the
		// pmodel counter
		if (qinst->lit[j] == NULL) {
			qinst->pmodel++;
		}
done: continue;
	}
}

/*
 * Updates the counts based on the current sample
 */
void update_pmodel(samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t num_vars = table->atom_table.num_vars;
	int32_t *pmodel = table->atom_table.pmodel;
	int32_t i;

	table->atom_table.num_samples++;
	for (i = 0; i < num_vars; i++) {
		if (assigned_true(assignment[i])) {
			if (get_verbosity_level() >= 3 && i > 0) {
				printf("Atom %d was assigned true\n", i);
				fflush(stdout);
			}
			pmodel[i]++;
		}
	}

	update_query_pmodel(table);

	// for the covariance matrix
	update_covariance_matrix_statistics(table);

	if (get_dump_samples_path() != NULL)
	{
		append_assignment_to_file(get_dump_samples_path(), table);
	}

	if (get_verbosity_level() >= 1) {
		printf("[update_pmodel] Model added:\n");
		print_assignment(table);
	}
}

/*
 * Indicates the first sample sat of the mc_sat, in which only the
 * hard clauses are considered.
 */
bool hard_only;

/**
 * Top-level MCSAT call
 *
 * Parameters for sample sat:
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
 * Parameters for mc_sat:
 * - max_samples = number of samples generated
 * - timeout = maximum running time for MCSAT
 * - burn_in_steps = number of burn-in steps
 * - samp_interval = sampling interval
 */
void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
		double samp_temperature, double rvar_probability, uint32_t max_flips,
		uint32_t max_extra_flips, uint32_t timeout,
		uint32_t burn_in_steps, uint32_t samp_interval) {
	int32_t conflict;
	uint32_t i;
	time_t fintime = 0;
	bool draw_sample; // whether we use the current round of MCMC as a sample

	cprintf(1, "[mc_sat] MC-SAT started ...\n");

	hard_only = true;

	first_sample_sat(table, lazy, sa_probability, samp_temperature,
			rvar_probability, max_flips);

	hard_only = false;

	if (conflict == -1) {
		mcsat_err("Found conflict in initialization.\n");
		return;
	}
	if (get_verbosity_level() >= 1) {
		printf("\n[mc_sat] initial assignment (all hard clauses are satisifed):\n");
		print_assignment(table);
		print_clause_table(table, -1);
	}
	//assert(valid_table(table));

	if (timeout != 0) {
		fintime = time(NULL) + timeout;
	}
	for (i = 0; i < burn_in_steps + max_samples * samp_interval; i++) {
		draw_sample = (i >= burn_in_steps && i % samp_interval == 0);
		if (draw_sample) {
			cprintf(1, "\n---- Sample[%"PRIu32"] ---\n",
					(i - burn_in_steps) / samp_interval);
		}

		//assert(valid_table(table));
		conflict = reset_sample_sat(table);

		sample_sat(table, lazy, sa_probability, samp_temperature,
				rvar_probability, max_flips, max_extra_flips);

		if (draw_sample) {
			update_pmodel(table);
		}

		if (get_verbosity_level() >= 2) {
			printf("\n[mc_sat] MC-SAT after round %d:\n", i);
			print_assignment(table);
		}
		//assert(valid_table(table));

		if (timeout != 0 && time(NULL) >= fintime) {
			printf("Timeout after %"PRIu32" samples\n", i);
			break;
		}
	}
}

// Recurse through the all the possible constants (except at j, which is fixed to the
// new constant being added) and add each resulting atom.
void new_const_atoms_at(int32_t idx, int32_t newcidx, int32_t arity,
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
		if (entry.constants == NULL) {
			// An integer
			if (entry.ints == NULL) {
				for (i = entry.lower_bound; i <= entry.upper_bound; i++) {
					atom->args[idx] = i;
					new_const_atoms_at(idx + 1, newcidx, arity, psig, atom, table);
				}
			} else {
				for (i = 0; i < entry.cardinality; i++) {
					atom->args[idx] = entry.ints[i];
					new_const_atoms_at(idx + 1, newcidx, arity, psig, atom, table);
				}
			}
		} else {
			for (i = 0; i < entry.cardinality; i++) {
				atom->args[idx] = entry.constants[i];
				new_const_atoms_at(idx + 1, newcidx, arity, psig, atom, table);
			}
		}
	}
}

// When new constants are introduced, may need to add new atoms
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
		atom_buffer_resize(&atom_buffer, arity);
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
		atom_buffer_resize(&atom_buffer, arity);
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

// Fills in the substit_buffer, creates atoms when arity is reached
void all_pred_instances_rec(int32_t vidx, int32_t *psig, int32_t arity,
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
		if (entry.constants == NULL) {
			// Have an integer
			if (entry.ints == NULL) {
				for (i = entry.lower_bound; i <= entry.upper_bound; i++) {
					atom->args[vidx] = i;
					all_pred_instances_rec(vidx + 1, psig, arity, atom, table);
				}
			} else {
				for (i = 0; i < entry.cardinality; i++) {
					atom->args[vidx] = entry.ints[i];
					all_pred_instances_rec(vidx + 1, psig, arity, atom, table);
				}
			}
		} else {
			for (i = 0; i < entry.cardinality; i++) {
				atom->args[vidx] = entry.constants[i];
				all_pred_instances_rec(vidx + 1, psig, arity, atom, table);
			}
		}
	}
}

void all_pred_instances(char *pred, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t predval, predidx, arity, *psig;
	samp_atom_t *atom;

	predidx = pred_index(pred, pred_table);
	predval = pred_val_to_index(predidx);
	arity = pred_arity(predval, pred_table);
	psig = pred_signature(predval, pred_table);
	atom_buffer_resize(&atom_buffer, arity);
	atom = (samp_atom_t *) atom_buffer.data;
	atom->pred = predval;
	all_pred_instances_rec(0, psig, arity, atom, table);
}

/**
 * all_rule_instances takes a rule and generates all ground instances
 * based on the atoms, their predicates and corresponding sorts
 *
 * For eager MC-SAT, it is called by all_rule_instances with atom_index equal to -1;
 * For lazy MC-SAT, the implementation is not efficient.
 */
void all_rule_instances_rec(int32_t vidx, samp_rule_t *rule, samp_table_t *table, int32_t atom_index) {

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
		}
	}
}

// Eager - called by MCSAT when a new rule is added.
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

// Called by MCSAT when a new constant is added.
// Generates all new instances of rules involving this constant.
void create_new_const_rule_instances(int32_t constidx, int32_t csort,
		samp_table_t *table, int32_t atom_index) {
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

/*
 * Queries are not like rules, as rules are simpler (can treat as separate
 * clauses), and have a weight.  substit on rules updates the clause_table,
 * whereas substit_query updates the query_instance_table.
 */
int32_t add_subst_query_instance(samp_literal_t **litinst, substit_entry_t *substs,
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
int32_t substit_query(samp_query_t *query, substit_entry_t *substs, samp_table_t *table) {
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
 * Similar to (and based on) check_clause_instance
 * TODO: How does it differ from check_clause_instance
 */
bool check_query_instance(samp_query_t *query, samp_table_t *table) {
	pred_table_t *pred_table = &(table->pred_table);
	//pred_tbl_t *pred_tbl = &(pred_table->pred_tbl);
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
			atom_buffer_resize(&rule_atom_buffer, arity);
			rule_inst_atom = (samp_atom_t *) rule_atom_buffer.data;
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
				add_and_activate_atom(table, rule_inst_atom);
				// check for witness predicate - fixed false if NULL atom_map
				atom_map = array_size_hmap_find(&(atom_table->atom_var_hash),
						arity + 1, (int32_t *) rule_inst_atom);
			}

			assert(atom_map != NULL);
			// check for witness predicate - fixed false if NULL atom_map
			//      if (atom_map == NULL) return false;//atom is inactive

			if (query->literals[i][j]->neg &&
					atom_table->current_assignment[atom_map->val] == v_fixed_false) {
				return false;//literal is fixed true
			}
			if (!query->literals[i][j]->neg &&
					atom_table->current_assignment[atom_map->val] == v_fixed_true) {
				return false;//literal is fixed true
			}
		}
	}
	return true;
}

/*
 * Recursively generates all groundings of a query
 */
void all_query_instances_rec(int32_t vidx, samp_query_t *query,
		samp_table_t *table, int32_t atom_index) {
	sort_table_t *sort_table;
	sort_entry_t entry;
	int32_t i, vsort;

	/* termination of recursion */
	if (vidx == query->num_vars) {
		substit_buffer.entries[vidx] = INT32_MIN;
		if (!lazy_mcsat() || check_query_instance(query, table)) {
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
			all_query_instances_rec(vidx + 1, query, table, atom_index);
		}
	}
}

/*
 * This is used in the simple case where the samp_query has no variables
 * So we simply convert it to a query_instance and add it to the tables.
 */
int32_t samp_query_to_query_instance(samp_query_t *query, samp_table_t *table) {
	pred_table_t *pred_table = &(table->pred_table);
	samp_atom_t *new_atom;
	int32_t i, j, k, arity, argidx, added_atom;
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
				argidx = new_atom->args[k];
			}
			if (get_verbosity_level() > 1) {
				printf("[samp_query_to_query_instance] Adding internal atom ");
				print_atom_now(new_atom, table);
				printf("\n");
			}
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

// Similar to all_rule_instances, but updates query_instance_table
// instead of clause_table
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
bool match_atom_in_rule_atom(samp_atom_t *atom, rule_literal_t *lit,
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

