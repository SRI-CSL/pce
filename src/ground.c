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

extern atom_buffer_t samp_atom_buffer;
extern atom_buffer_t rule_atom_buffer;
extern clause_buffer_t clause_buffer;
extern substit_buffer_t substit_buffer;

static ivector_t new_intidx = {0, 0, NULL};

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

	atom_buffer_resize(&samp_atom_buffer, arity);
	samp_atom_t *atom = (samp_atom_t *) samp_atom_buffer.data;
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

/*
 * Fills in the substit_buffer, creates atoms when arity is reached
 */
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

/* Instantiates all ground atoms for a predicate */
void all_pred_instances(char *pred, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	int32_t predval, predidx, arity, *psig;
	samp_atom_t *atom;

	predidx = pred_index(pred, pred_table);
	predval = pred_val_to_index(predidx);
	arity = pred_arity(predval, pred_table);
	psig = pred_signature(predval, pred_table);
	atom_buffer_resize(&samp_atom_buffer, arity);
	atom = (samp_atom_t *) samp_atom_buffer.data;
	atom->pred = predval;
	all_pred_instances_rec(0, psig, arity, atom, table);
}

/*
 * all_rule_instances takes a rule and generates all ground instances
 * based on the atoms, their predicates and corresponding sorts
 *
 * For eager MC-SAT, it is called by all_rule_instances with atom_index equal to -1;
 * For lazy MC-SAT, the implementation is not efficient.
 */
void all_rule_instances_rec(int32_t vidx, samp_rule_t *rule, samp_table_t *table, 
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
 * This is used in the simple case where the samp_query has no variables
 * So we simply convert it to a query_instance and add it to the tables.
 */
static int32_t samp_query_to_query_instance(samp_query_t *query, samp_table_t *table) {
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
		atom_buffer_resize(&samp_atom_buffer, arity);
		atom = (samp_atom_t *) samp_atom_buffer.data;
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
		atom_buffer_resize(&samp_atom_buffer, arity);
		atom = (samp_atom_t *) samp_atom_buffer.data;
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

