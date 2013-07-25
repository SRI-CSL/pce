#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>
#include "memalloc.h"
#include "utils.h"
#include "parser.h"
#include "input.h"
#include "print.h"
#include "vectors.h"
#include "weight_learning.h"
#include "tables.h"

#include "mcmc.h"
#include "ground.h"
#include "cnf.h"

pvector_t ask_buffer = { 0, 0, NULL };

extern samp_table_t samp_table;

static int32_t vars_len(var_entry_t **vars) {
	int32_t vlen;

	if (vars == NULL) {
		return 0;
	} else {
		for (vlen = 0; vars[vlen] != NULL; vlen++) {
		}
		return vlen;
	}
}

rule_atom_t *copy_rule_atom(rule_atom_t *atom, samp_table_t *table) {
	pred_table_t *pred_table = &samp_table.pred_table;
	rule_atom_t *catom;
	int32_t i, arity;

	arity = atom_arity(atom, pred_table);
	catom = (rule_atom_t *) safe_malloc(sizeof(rule_atom_t));
	catom->pred = atom->pred;
	catom->builtinop = atom->builtinop;
	catom->args = (rule_atom_arg_t *) safe_malloc(
			arity * sizeof(rule_atom_arg_t));
	for (i = 0; i < arity; i++) {
		catom->args[i].kind = atom->args[i].kind;
		catom->args[i].value = atom->args[i].value;
	}
	return catom;
}

rule_literal_t *copy_rule_literal(rule_literal_t *lit, samp_table_t *table) {
	rule_literal_t *clit;
	clit = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
	clit->neg = lit->neg;
	clit->atom = copy_rule_atom(lit->atom, table);
	return clit;
}

static rule_literal_t *atom_to_rule_literal(input_atom_t *iatom,
		var_entry_t **vars, bool neg) {
	samp_table_t *table = &samp_table;
	pred_table_t *pred_table = &table->pred_table;
	sort_table_t *sort_table = &table->sort_table;
	const_table_t *const_table = &table->const_table;
	sort_entry_t *sort_entry = NULL;
	int32_t i, j, num_args, pred_val, pred_idx, const_idx, *psig, vsig, subsig,
			vlen, intval;
	char *pred, *cname;
	rule_atom_arg_t *args;
	rule_atom_t *atom;
	rule_literal_t *lit;

	pred = iatom->pred;
	// Get number of args
	for (num_args = 0; iatom->args[num_args] != NULL; num_args++) {
	}
	// check if we've seen the predicate before and complain if not.
	if (iatom->builtinop == 0) {
		pred_val = pred_index(pred, pred_table);
		if (pred_val == -1) {
			mcsat_err("Predicate %s not declared", pred);
			return NULL;
		}
		pred_idx = pred_val_to_index(pred_val);
		if (pred_arity(pred_idx, pred_table) != num_args) {
			mcsat_err("Wrong number of args");
			return NULL;
		}
		psig = pred_signature(pred_idx, pred_table);
	} else {
		// A builtinop
		pred_idx = INT32_MAX; // Make it unusable
		if (builtin_arity(iatom->builtinop) != num_args) {
			mcsat_err("Wrong number of args");
			return NULL;
		}
		// We let the other atoms in the rule determine variable signatures
		psig = NULL;
	}
	args = (rule_atom_arg_t *) safe_malloc(num_args * sizeof(rule_atom_arg_t));
	atom = (rule_atom_t *) safe_malloc(sizeof(rule_atom_t));
	atom->args = args;
	atom->pred = pred_idx;
	atom->builtinop = iatom->builtinop;
	vlen = vars_len(vars);
	for (i = 0; i < num_args; i++) {
		for (j = 0; j < vlen; j++) {
			if (strcmp(vars[j]->name, iatom->args[i]) == 0) {
				break;
			}
		}
		if (j < vlen) {
			// We have a variable.  Variables are tagged in rule_atom_arg_t
			// Check the signature, and adjust to the least common supersort
			vsig = vars[j]->sort_index;
			if (psig != 0) {
				if (vsig == -1) {
					vars[j]->sort_index = psig[i];
				} else {
					subsig = greatest_common_subsort(vsig, psig[i], sort_table);
					if (subsig != vsig) {
						if (subsig == -1) {
							mcsat_err(
									"Variable %s used with incompatible sorts %s and %s",
									vars[j]->name,
									sort_table->entries[vsig].name,
									sort_table->entries[psig[i]].name);
							return NULL;
						}
						vars[j]->sort_index = subsig;
					}
				}
			}
			atom->args[i].kind = variable;
			atom->args[i].value = j;
		} else {
			// Must be a constant or integer
			cname = iatom->args[i];
			// May not have a signature, if it is a builtin (e.g., = or /=)
			if (psig != NULL) {
				sort_entry = &sort_table->entries[psig[i]];
			}
			char *p;
			for (p = (cname[0] == '+' || cname[0] == '-') ? &cname[1] : cname;
					isdigit(*p); p++)
				;
			if (*p == '\0') {
				// Have an integer - check that it is correct
				if (psig == NULL || sort_entry->constants == NULL) {
					// Maybe an integer
					intval = str2int(cname);
					atom->args[i].kind = integer;
					atom->args[i].value = intval;
					if (psig != NULL && sort_entry->ints != NULL) {
						if (add_int_const(intval, sort_entry, sort_table)) {
							create_new_const_atoms(intval, psig[i], table);
							create_new_const_rule_instances(intval, psig[i],
									table, 0);
							create_new_const_query_instances(intval, psig[i],
									table, 0);
						}
					}
				} else {
					mcsat_err("Have integer where %s expected",
							sort_entry->name);
					return NULL;
				}
			} else {
				// See if constant is known
				const_idx = stbl_find(&const_table->const_name_index, cname);
				if (const_idx == -1) {
					if (!strict_constants()) {
						// Add the constant - note that the sort is determined by the
						// predicate, and may be wrong if psig[i] has supersorts -
						// warn in such a case.
						add_const_internal(cname, psig[i], &samp_table);
						if (sort_table->entries[psig[i]].supersorts != NULL) {
							printf(
									"Warning: constant %s assigned to subsort %s\n",
									cname, sort_table->entries[psig[i]].name);
						}
						const_idx = stbl_find(&const_table->const_name_index,
								cname);
					} else {
						mcsat_err("Constant %s not previously declared\n",
								cname);
						return NULL;
					}
				} else {
					if (psig != NULL) {
						if (sort_entry->constants == NULL) {
							mcsat_err(
									"Expected an integer of sort %s, but given %s",
									sort_entry->name, cname);
							return NULL;
						}
						bool foundit = false;
						for (j = 0; j < sort_entry->cardinality; j++) {
							if (sort_entry->constants[j] == const_idx) {
								foundit = true;
								break;
							}
						}
						if (!foundit) {
							mcsat_err(
									"Sort error: constant %s of sort %s\n does not match pred %s arg %"PRId32" sort %s\n",
									cname,
									const_sort_name(const_idx, &samp_table),
									pred, i + 1, sort_entry->name);
							return NULL;
						}
					}
				}
				atom->args[i].kind = constant;
				atom->args[i].value = const_idx;
			}
		}
	}
	//print_atom(atom, &samp_table);
	//fflush(stdout);
	lit = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
	lit->neg = neg;
	lit->atom = atom;
	return lit;
}

static rule_literal_t *** cnf_literal(input_atom_t *atom, var_entry_t **vars,
		bool neg) {
	rule_literal_t *lit;
	rule_literal_t **conj_lit;
	rule_literal_t ***cnf_lit;

	lit = atom_to_rule_literal(atom, vars, neg);
	if (lit == NULL) {
		return NULL;
	}
	conj_lit = (rule_literal_t **) safe_malloc(2 * sizeof(rule_literal_t *));
	cnf_lit = (rule_literal_t ***) safe_malloc(2 * sizeof(rule_literal_t **));
	cnf_lit[0] = conj_lit;
	cnf_lit[1] = NULL;
	conj_lit[0] = lit;
	conj_lit[1] = NULL;
	return cnf_lit;
}

// Given two CNFs - essentially lists of clauses, which are lists of
// literals.  We construct the product.  Here the CNFs are represented by
// (NULL-terminated) arrays of arrays of literals.
//   ((a b) (c d)) X ((x y z) (w)) == ((a b x y z) (a b w) (c d x y z) (c d w))
// For now, no simplification is done: so
// ((a b) (c)) X ((a ~c)) == ((a b a ~c) (c a ~c)), which could simplify to
//  ((a b ~c))

static rule_literal_t ***cnf_product(rule_literal_t ***c1, rule_literal_t ***c2) {
	int32_t psize, i, j, ii, jj, len1, len2, cnflen1, cnflen2, idx;
	rule_literal_t ***productcnf;
	rule_literal_t **conjunct;

	if (c1 == NULL || c2 == NULL) {
		return NULL;
	}
	for (cnflen1 = 0; c1[cnflen1] != NULL; cnflen1++) {
	}
	for (cnflen2 = 0; c2[cnflen2] != NULL; cnflen2++) {
	}
	psize = (cnflen1 * cnflen2) + 1;
	productcnf = (rule_literal_t ***) safe_malloc(
			psize * sizeof(rule_literal_t **));
	idx = 0;
	for (i = 0; c1[i] != NULL; i++) {
		for (len1 = 0; c1[i][len1] != NULL; len1++) {
		}
		for (j = 0; c2[j] != NULL; j++) {
			for (len2 = 0; c2[j][len2] != NULL; len2++) {
			}
			conjunct = (rule_literal_t **) safe_malloc(
					(len1 + len2 + 1) * sizeof(rule_literal_t *));
			for (ii = 0; ii < len1; ii++) {
				conjunct[ii] = copy_rule_literal(c1[i][ii], &samp_table);
			}
			for (jj = 0; jj < len2; jj++) {
				conjunct[len1 + jj] = copy_rule_literal(c2[j][jj], &samp_table);
			}
			conjunct[len1 + len2] = NULL;
			productcnf[idx++] = conjunct;
		}
	}
	productcnf[idx] = NULL;
	for (i = 0; c1[i] != NULL; i++) {
		free_rule_literals(c1[i]);
	}
	safe_free(c1);
	for (j = 0; c2[j] != NULL; j++) {
		free_rule_literals(c2[j]);
	}
	safe_free(c2);
	return productcnf;
}

// Given two CNFs, return the union.
// ((a b) (c d)) U ((x y z) (w)) == ((a b) (c d) (x y z) (w))
// Again, no simplification involved.

static rule_literal_t ***cnf_union(rule_literal_t ***c1, rule_literal_t ***c2) {
	int32_t i, cnflen1, cnflen2;
	rule_literal_t ***unioncnf;

	if (c1 == NULL || c2 == NULL) {
		return NULL;
	}
	for (cnflen1 = 0; c1[cnflen1] != NULL; cnflen1++)
		;
	for (cnflen2 = 0; c2[cnflen2] != NULL; cnflen2++)
		;
	unioncnf = (rule_literal_t ***) safe_malloc(
			(cnflen1 + cnflen2 + 1) * sizeof(rule_literal_t **));
	for (i = 0; c1[i] != NULL; i++) {
		unioncnf[i] = c1[i];
	}
	for (i = 0; c2[i] != NULL; i++) {
		unioncnf[cnflen1 + i] = c2[i];
	}
	unioncnf[cnflen1 + i] = NULL;
	safe_free(c1);
	safe_free(c2);
	return unioncnf;
}

rule_literal_t ***cnf_neg(input_fmla_t *fmla, var_entry_t **vars);

rule_literal_t ***cnf_pos(input_fmla_t *fmla, var_entry_t **vars) {
	int32_t op;
	input_atom_t *atom;
	input_comp_fmla_t *cfmla;

	if (fmla->atomic) {
		atom = fmla->ufmla->atom;
		return cnf_literal(atom, vars, false);
	} else {
		cfmla = fmla->ufmla->cfmla;
		op = cfmla->op;
		if (op == NOT) {
			return cnf_neg(cfmla->arg1, vars);
		} else if (op == OR) {
			return cnf_product(cnf_pos(cfmla->arg1, vars),
					cnf_pos(cfmla->arg2, vars));
		} else if (op == IMPLIES) {
			return cnf_product(cnf_neg(cfmla->arg1, vars),
					cnf_pos(cfmla->arg2, vars));
		} else if (op == AND) {
			return cnf_union(cnf_pos(cfmla->arg1, vars),
					cnf_pos(cfmla->arg2, vars));
		} else if (op == XOR) {
			// CNF(A xor B) = CNF(A /\ not(B)) X CNF(not(A) /\ B)
			return cnf_product(
					cnf_union(cnf_pos(cfmla->arg1, vars),
							cnf_neg(cfmla->arg2, vars)),
					cnf_union(cnf_neg(cfmla->arg1, vars),
							cnf_pos(cfmla->arg2, vars)));
		} else if (op == IFF) {
			// CNF(A iff B) = CNF(A X not(B)) /\ CNF(not(A) X B)
			return cnf_union(
					cnf_product(cnf_pos(cfmla->arg1, vars),
							cnf_neg(cfmla->arg2, vars)),
					cnf_product(cnf_neg(cfmla->arg1, vars),
							cnf_pos(cfmla->arg2, vars)));
		} else {
			mcsat_err("cnfpos error: op %"PRId32" unexpected\n", op);
			return NULL;
		}
	}
}

rule_literal_t ***cnf_neg(input_fmla_t *fmla, var_entry_t **vars) {
	int32_t op;
	input_atom_t *atom;
	input_comp_fmla_t *cfmla;

	if (fmla->atomic) {
		atom = fmla->ufmla->atom;
		return cnf_literal(atom, vars, true);
	} else {
		cfmla = fmla->ufmla->cfmla;
		op = cfmla->op;
		if (op == NOT) {
			return cnf_pos(cfmla->arg1, vars);
		} else if (op == OR) {
			return cnf_union(cnf_neg(cfmla->arg1, vars),
					cnf_neg(cfmla->arg2, vars));
		} else if (op == IMPLIES) {
			return cnf_union(cnf_pos(cfmla->arg1, vars),
					cnf_neg(cfmla->arg2, vars));
		} else if (op == AND) {
			return cnf_product(cnf_neg(cfmla->arg1, vars),
					cnf_neg(cfmla->arg2, vars));
		} else if (op == XOR) {
			// CNF(not(A xor B)) = CNF(not(A) \/ B) U CNF(A \/ not(B))
			return cnf_union(
					cnf_product(cnf_neg(cfmla->arg1, vars),
							cnf_pos(cfmla->arg2, vars)),
					cnf_product(cnf_pos(cfmla->arg1, vars),
							cnf_neg(cfmla->arg2, vars)));
		} else if (op == IFF) {
			// CNF(not(A iff B)) = CNF(A \/ B) U CNF(not(A) \/ not(B))
			return cnf_union(
					cnf_product(cnf_pos(cfmla->arg1, vars),
							cnf_pos(cfmla->arg2, vars)),
					cnf_product(cnf_neg(cfmla->arg1, vars),
							cnf_neg(cfmla->arg2, vars)));
		} else {
			mcsat_err("cnfpos error: op %"PRId32" unexpected\n", op);
			return NULL;
		}
	}
}

var_entry_t **copy_variables(var_entry_t **vars) {
	var_entry_t **cvars;
	int32_t i, numvars;

	if (vars == NULL)
		return NULL;
	for (numvars = 0; vars[numvars] != NULL; numvars++) {
	}
	cvars = (var_entry_t **) safe_malloc(numvars * sizeof(var_entry_t *));
	for (i = 0; i < numvars; i++) {
		cvars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
		cvars[i]->name = str_copy(vars[i]->name);
		cvars[i]->sort_index = vars[i]->sort_index;
	}
	return cvars;
}

// Sets the variables in the clause, removing those not mentioned
void set_fmla_clause_variables(samp_rule_t *clause, var_entry_t **vars) {
	pred_table_t *pred_table = &samp_table.pred_table;
	rule_atom_t *atom;
	var_entry_t **cvars;
	int32_t i, j, numvars, arity, vidx, delta, *mapping;
	bool usedvar, unusedvar;

	// First copy the vars, then tag all with negated signatures
	// assumes all sigs are nonnegative.
	for (numvars = 0; vars[numvars] != NULL; numvars++) {
	}
	cvars = copy_variables(vars);
	for (i = 0; i < numvars; i++) {
		assert(cvars[i]->sort_index >= 0);
		cvars[i]->sort_index = -(cvars[i]->sort_index + 1);
	}
	// Now go through the literals, untagging the found variables
	for (i = 0; i < clause->num_lits; i++) {
		atom = clause->literals[i]->atom;
		arity = atom_arity(atom, pred_table);
		// Loop through the args of the atom
		for (j = 0; j < arity; j++) {
			if (atom->args[j].kind == variable) {
				vidx = atom->args[j].value;
				if (cvars[vidx]->sort_index < 0) {
					cvars[vidx]->sort_index = -(cvars[vidx]->sort_index + 1);
				}
			}
		}
	}
	usedvar = false;
	unusedvar = false;
	for (i = 0; i < numvars; i++) {
		if (cvars[i]->sort_index >= 0) {
			usedvar = true;
		} else {
			unusedvar = true;
		}
	}
	if (usedvar) {
		delta = 0;
		if (unusedvar) {
			// Some variable was not used - create an int array mapping
			// old var index to new, remove unused vars from cvars, and
			// reset the literals using the mapping.
			mapping = (int32_t *) safe_malloc(numvars * sizeof(int32_t));
			j = 0;
			for (i = 0; i < numvars; i++) {
				if (cvars[i]->sort_index < 0) {
					delta++;
					mapping[i] = -1;
				} else {
					cvars[i - delta] = cvars[i];
					mapping[i] = i - delta;
				}
			}
			// This leads to valgrind errors - don't know why
			//safe_realloc(cvars, (numvars-delta) * sizeof(var_entry_t *));
			for (i = 0; i < clause->num_lits; i++) {
				atom = clause->literals[i]->atom;
				arity = atom->builtinop == 0 ?
						pred_arity(atom->pred, pred_table) :
						builtin_arity(atom->builtinop);
				// Loop through the args of the atom
				for (j = 0; j < arity; j++) {
					if (atom->args[j].kind == variable) {
						// We have a variable - vidx is the index into vars
						vidx = atom->args[j].value;
						atom->args[j].value = mapping[vidx];
					}
				}
			}
		}
		clause->num_vars = numvars - delta;
		clause->vars = cvars;
	} else {
		clause->num_vars = 0;
		clause->vars = NULL;
		free_var_entries(cvars);
	}
}

bool vars_equal(var_entry_t *v1, var_entry_t *v2) {
	return (v1->sort_index == v2->sort_index)
			&& (strcmp(v1->name, v2->name) == 0);
}

bool clause_vars_equal(int32_t nvars, var_entry_t **v1, var_entry_t **v2) {
	int32_t i;

	for (i = 0; i < nvars; i++) {
		if (!vars_equal(v1[i], v2[i])) {
			return false;
		}
	}
	return true;
}

bool rule_atoms_equal(rule_atom_t *a1, rule_atom_t *a2) {
	int32_t i, arity;

	if (a1->builtinop == a2->builtinop && a1->pred == a2->pred) {
		arity = atom_arity(a1, &samp_table.pred_table);
		for (i = 0; i < arity; i++) {
			if ((a1->args[i].kind != a2->args[i].kind)
					|| (a1->args[i].value != a2->args[i].value)) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
}

bool lits_resolve(rule_literal_t *l1, rule_literal_t *l2) {
	return (l1->neg != l2->neg) && rule_atoms_equal(l1->atom, l2->atom);
}

bool lits_equal(rule_literal_t *l1, rule_literal_t *l2) {
	return (l1->neg == l2->neg) && rule_atoms_equal(l1->atom, l2->atom);
}

bool clause_lits_equal(int32_t nlits, rule_literal_t **l1, rule_literal_t **l2) {
	int32_t i;

	for (i = 0; i < nlits; i++) {
		if (!lits_equal(l1[i], l2[i])) {
			return false;
		}
	}
	return true;
}

bool clauses_equal(samp_rule_t *c1, samp_rule_t *c2) {
	return (c1->num_vars == c2->num_vars)
			&& (clause_vars_equal(c1->num_vars, c1->vars, c2->vars))
			&& (c1->num_lits == c2->num_lits)
			&& (clause_lits_equal(c1->num_lits, c1->literals, c2->literals));
}

// Checks clauses to see that they all involve at least one indirect atom
// or exactly one positive direct atom (head normal form)
// Will not return if there is an error.
bool all_clauses_involve_indirects_or_hnf(rule_literal_t ***lits, double weight) {
	int32_t i, j;
	bool pos;

	for (i = 0; lits[i] != NULL; i++) {
		for (j = 0; lits[i][j] != NULL; j++) {
			if (!pred_epred(lits[i][j]->atom->pred)) {
				// Found a hidden (indirect) predicate
				return true;
			}
		}
		// All preds are direct - check that the weight is DBL_MAX and there
		// is exactly one positive
		if (weight != DBL_MAX) {
			mcsat_err(
					"cnf error: clause contains only observable predicates, should not be given weight:");
			return false;
		}
		pos = false;
		for (j = 0; lits[i][j] != NULL; j++) {
			if (!lits[i][j]->neg) {
				if (pos) {
					// Have more than one pos, complain
					mcsat_err(
							"cnf error: clause contains only observable predicates, with more than one positive:");
					return false;
				}
				pos = true;
			}
		}
		if (!pos) {
			mcsat_err(
					"cnf error: clause contains only observable predicates, with none positive:");
			return false;
		}
	}
	return true;
}

// Generates the CNF form of a formula, and updates the clause table (if no
// variables) or the rule table.
void add_cnf(char **frozen, input_formula_t *formula, double weight,
		char *source, bool add_weights) {
	rule_table_t *rule_table = &samp_table.rule_table;
	pred_table_t *pred_table = &samp_table.pred_table;
	clause_table_t * clause_table = &samp_table.clause_table;
	int32_t i, j, num_frozen, fpred_val, fpred_idx, *frozen_preds, current_rule,
			num_lits, num_vars, atom_idx;
	rule_literal_t ***lits, *lit;
	samp_rule_t *clause, *found;
	bool found_indirect;

	num_frozen = 0;

	// Returns the literals, and sets the sort of the variables
	lits = cnf_pos(formula->fmla, formula->vars);
	if (!lits) {
		return;
	}

	// First check if clauses all either involve indirect preds, or are in head normal form
	// We check here in order to abort the add_cnf before adding some clauses
	// If there is an error, this function will not return
	if (!all_clauses_involve_indirects_or_hnf(lits, weight)) {
		return;
	}

	if (frozen != NULL) {
		for (num_frozen = 0; frozen[num_frozen] != NULL; num_frozen++) {
		};
		frozen_preds = (int32_t *) safe_malloc(
				(num_frozen + 1) * sizeof(int32_t));
		j = 0;
		for (i = 0; frozen[i] != NULL; i++) {
			fpred_val = pred_index(frozen[i], pred_table);
			if (fpred_val == -1) {
				mcsat_warn("Predicate %s not declared; will be ignored",
						frozen[i]);
			} else {
				fpred_idx = pred_val_to_index(fpred_val);
				if (pred_epred(fpred_idx)) {
					// A direct predicate - no sense in trying to fix it complain, but go on anyway
					mcsat_warn("Predicate %s is observable, hence already frozen",
							frozen[i]);
				} else {
					frozen_preds[j++] = fpred_idx;
				}
			}
		}
		num_frozen = j;
		frozen_preds[j] = -1;
	} else {
		frozen_preds = NULL;
	}

	int32_t *clause_lits;
	if (formula->vars == NULL) {
		// No variables, just assert the clauses

		for (i = 0; lits[i] != NULL; i++) {
			for (num_lits = 0; lits[i][num_lits] != NULL; num_lits++) {
			}
			clause_lits = (int32_t *) safe_malloc(sizeof(int32_t) * num_lits);
			found_indirect = false;
			for (j = 0; j < num_lits; j++) {
				lit = lits[i][j];
				if (lit->atom->pred > 0) {
					found_indirect = true;
				}
				samp_atom_t *satom = rule_atom_to_samp_atom(lit->atom, NULL,
						pred_table);
				// FIXME what is top_p? should it be true or false?
				//atom_idx = add_internal_atom(&samp_table, satom, true);
				atom_idx = add_internal_atom(&samp_table, satom, false);
				free_samp_atom(satom);
				clause_lits[j] = lit->neg ? neg_lit(atom_idx) : pos_lit(atom_idx);
			}
			int32_t clause_index = add_internal_clause(&samp_table, 
					clause_lits, num_lits, frozen_preds, weight, 
					found_indirect, add_weights);
			safe_free(clause_lits);

			if (get_verbosity_level() >= 1) {
				print_clause(clause_table->samp_clauses[clause_index], &samp_table);
				printf("\n");
			}
		}
	} else { /* contains universally quantified variables */

		for (num_vars = 0; formula->vars[num_vars] != NULL; num_vars++) {
		}
		for (i = 0; i < num_vars; i++) {
			// Check that all variables have been referenced
			if (formula->vars[i]->sort_index == -1) {
				mcsat_err("cnf error: variable %s not used in formula\n",
						formula->vars[i]->name);
				safe_free(lits);
				return;
			}
		}
		// Now create the samp_rule_t, and add it to the rule_table
		for (i = 0; lits[i] != NULL; i++) {
			clause = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
			for (num_lits = 0; lits[i][num_lits] != NULL; num_lits++) {
			}
			clause->num_lits = num_lits;
			clause->literals = lits[i];
			clause->num_frozen = num_frozen;
			clause->frozen_preds = frozen_preds;
			//set_clause_variables(clause);
			clause->num_vars = num_vars;
			init_array_hmap(&clause->subst_hash, ARRAY_HMAP_DEFAULT_SIZE);
			set_fmla_clause_variables(clause, formula->vars);
			found = NULL;

			if (get_verbosity_level() >= 1) {
				print_rule(clause, &samp_table, 0);
				printf("\n");
			}

			// Now check if clause is in the table
			for (j = 0; j < rule_table->num_rules; j++) {
				if (clauses_equal(rule_table->samp_rules[j], clause)) {
					found = rule_table->samp_rules[j];
					break;
				}
			}
			if (found == NULL) {
				clause->weight = weight;
				rule_table_resize(rule_table);
				current_rule = rule_table->num_rules;
				rule_table->samp_rules[current_rule] = clause;
				rule_table->num_rules++;
				for (j = 0; j < num_lits; j++) {
					if (lits[i][j]->atom->builtinop == 0) {
						int32_t pred = lits[i][j]->atom->pred;
						add_rule_to_pred(pred_table, pred, current_rule);
					}
				}
				if (!lazy_mcsat()) {
					all_rule_instances(current_rule, &samp_table);
				} else {
					smart_rule_instances(current_rule, &samp_table);
				}
			} else {
				if (found->weight != DBL_MAX) {
					mcsat_warn("Rule was seen before, adding weights\n");
					if (weight == DBL_MAX) {
						found->weight = DBL_MAX;
					} else {
						found->weight += weight;
					}
					// Need to update the clause table
				} else {
					mcsat_warn("Rule was seen before with MAX weight: unchanged\n");
				}
			}
		}
		safe_free(lits);
	}
}

static int cmp_pmodels(const void *p1, const void *p2) {
	samp_query_instance_t **q1, **q2;
	int32_t n1, n2;

	q1 = (samp_query_instance_t **) p1;
	q2 = (samp_query_instance_t **) p2;
	n1 = (*q1)->pmodel;
	n2 = (*q2)->pmodel;
	return n1 < n2 ? 1 : n1 > n2 ? -1 : 0;
}

// Similar to ask_cnf, but the role of it is to add queries one by one
// to allow sampling of them at the same time by a later call to mc_sat()
void add_cnf_query(input_formula_t *formula) {
	rule_literal_t ***lits;
	samp_query_t *query;
	int32_t i;

	ask_buffer.size = 0;
	// Returns the literals, and sets the sort of the variables
	lits = cnf_pos(formula->fmla, formula->vars);
	if (lits == NULL) {
		return;
	}
	// Add all instances of the query to the query_instance_table
	query = (samp_query_t *) safe_malloc(sizeof(samp_query_t));
	for (i = 0; lits[i] != NULL; i++) {
	}
	query->num_clauses = i;
	if (formula->vars == NULL) {
		query->num_vars = 0;
	} else {
		for (i = 0; formula->vars[i] != NULL; i++) {
		}
		query->num_vars = i;
	}
	query->vars = copy_variables(formula->vars);
	query->literals = lits;
	// add query to query table
	query_table_resize(&samp_table.query_table);
	samp_table.query_table.query[samp_table.query_table.num_queries++] = query;
	// Get the instances into the query_instance_table
	all_query_instances(query, &samp_table);

}

// Generates the CNF form of a formula, and updates the query and
// query_instance tables. These are analogous to the rule and clause
// tables, respectively. Thus queries without variables are immediately
// added to the query_instance table, while queries with variables are
// added to the query table, and instances of them are generated as needed
// and added to the query_instance table.
// Result is a sorted list of samp_query_instance_t's in the ask_buffer.
void ask_cnf(input_formula_t *formula, double threshold, int32_t maxresults) {
	query_instance_table_t *query_instance_table;
	rule_literal_t ***lits;
	samp_query_t *query;
	samp_query_instance_t *qinst;
	int32_t i;

	ask_buffer.size = 0;
	// Returns the literals, and sets the sort of the variables
	lits = cnf_pos(formula->fmla, formula->vars);
	if (lits == NULL) {
		return;
	}
	// Add all instances of the query to the query_instance_table
	query = (samp_query_t *) safe_malloc(sizeof(samp_query_t));
	for (i = 0; lits[i] != NULL; i++) {
	}
	query->num_clauses = i;
	if (formula->vars == NULL) {
		query->num_vars = 0;
	} else {
		for (i = 0; formula->vars[i] != NULL; i++) {
		}
		query->num_vars = i;
	}
	query->vars = copy_variables(formula->vars);
	query->literals = lits;
	// Get the instances into the query_instance_table
	all_query_instances(query, &samp_table);

	// Run the specified number of samples
	mc_sat(&samp_table, lazy_mcsat(), get_max_samples(), get_sa_probability(),
			get_samp_temperature(), get_rvar_probability(), get_max_flips(),
			get_max_extra_flips(), get_mcsat_timeout(), get_burn_in_steps(),
			get_samp_interval());

	query_instance_table = &samp_table.query_instance_table;
	// Collect those above the threshold, sort, and print
	if (ask_buffer.data == NULL) {
		init_pvector(&ask_buffer, 16);
	} else {
		pvector_reset(&ask_buffer);
	}
	for (i = 0; i < query_instance_table->num_queries; i++) {
		qinst = query_instance_table->query_inst[i];
		if (query_probability(qinst, &samp_table) >= threshold) {
			pvector_push(&ask_buffer, qinst);
		}
	}
	qsort(&ask_buffer.data[0], ask_buffer.size, sizeof(samp_query_instance_t *),
			cmp_pmodels);
	if (maxresults > 0 && ask_buffer.size > maxresults) {
		ask_buffer.size = maxresults;
	}
	free_samp_query(query);
	// Callers have to manage the query_instance table
	// To clear it out for the next query, call the following
	// reset_query_instance_table(query_instance_table);
}

