#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include <ctype.h>

#include "memalloc.h"
#include "prng.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "utils.h"
#include "buffer.h"
#include "print.h"
#include "yacc.tab.h"
#include "ground.h"

/*  
 * These copy operations assume that size input to safe_malloc can't 
 * overflow since there are existing structures of the given size. 
 */
char *str_copy(char *name){
	int32_t len = strlen(name);
	char * new_name = (char *) safe_malloc(++len * sizeof(char));
	memcpy(new_name, name, len);
	return new_name;
}

samp_atom_t *atom_copy(samp_atom_t *atom, int32_t arity){
	arity++;
	int32_t size = arity * sizeof(int32_t);
	samp_atom_t * new_atom = (samp_atom_t *) safe_malloc(size);
	memcpy(new_atom, atom, size);
	return new_atom;
}

int32_t *intarray_copy(int32_t *signature, int32_t length){
	int32_t size = length * sizeof(int32_t);
	int32_t *new_signature = (int32_t *) safe_malloc(size);
	memcpy(new_signature, signature, size);
	return new_signature;
}

/* Frees an array of strings */
void free_strings (char **string) {
	int32_t i = 0;
	if (string != NULL) {
		while (string[i] != NULL) {
			if (strcmp(string[i], "") != 0) {
				safe_free(string[i]);
			}
			i++;
		}
	}
}

int32_t str2int(char *cnst) {
	int32_t i;
	long int lcnst;

	for (i = (cnst[0] == '+' || cnst[0] == '-') ? 1 : 0; cnst[i] != '\0'; i++) {
		if (!isdigit(cnst[i])) {
			mcsat_err("%s is not a valid integer\n", cnst);
		}
	}
	lcnst = strtol(cnst, NULL, 10);
	if (lcnst > INT32_MAX) {
		mcsat_err("Integer %s is too big, above %"PRId32"", cnst, INT32_MAX);
	}
	if (lcnst < INT32_MIN) {
		mcsat_err("Integer %s is too small, below %"PRId32"", cnst, INT32_MIN);
	}
	return (int32_t) lcnst;
}

bool assigned_true_lit(samp_truth_value_t *assignment,
		samp_literal_t lit){
  // Note: var_of(x) is a right shift by 1 bit.  If lit is an index (a
  // literal number), then lit >> 1 corresponds to a variable number.
  // For some reason, we are overrunning the assignment array:
	return is_pos(lit) ?
		assigned_true(assignment[var_of(lit)]) :
		assigned_false(assignment[var_of(lit)]);
}

bool assigned_false_lit(samp_truth_value_t *assignment,
		samp_literal_t lit){
	return is_pos(lit) ?
		assigned_false(assignment[var_of(lit)]) :
		assigned_true(assignment[var_of(lit)]);
}

bool assigned_fixed_true_lit(samp_truth_value_t *assignment,
		samp_literal_t lit){
	return fixed_tval(assignment[var_of(lit)]) &&
		assigned_true_lit(assignment, lit);
}

bool assigned_fixed_false_lit(samp_truth_value_t *assignment,
		samp_literal_t lit){
	return fixed_tval(assignment[var_of(lit)]) &&
		assigned_false_lit(assignment, lit);
}

/* Returns the sort name of a constant */
char *const_sort_name(int32_t const_idx, samp_table_t *table) {
	sort_table_t *sort_table = &table->sort_table;
	const_table_t *const_table = &table->const_table;
	int32_t const_sig;

	const_sig = const_table->entries[const_idx].sort_index;
	return sort_table->entries[const_sig].name;
}

bool subsort_p(int32_t sig1, int32_t sig2, sort_table_t *sort_table) {
	int32_t i;
	sort_entry_t *ent1;
	if (sig1 == sig2) {
		return true;
	}
	ent1 = &sort_table->entries[sig1];
	if (ent1->supersorts != NULL) {
		for (i = 0; ent1->supersorts[i] != -1; i++) {
			if (ent1->supersorts[i] == sig2) {
				return true;
			}
		}
	}
	return false;
}

// Return a (not necessarily unique) least common supersort, or -1 if there is none
int32_t least_common_supersort(int32_t sig1, int32_t sig2, sort_table_t *sort_table) {
	int32_t i, j, lcs;
	sort_entry_t *ent1, *ent2;

	if (sig1 == sig2) {
		return sig1;
	}
	// First check if one is a subsort of the other
	if (subsort_p(sig1, sig2, sort_table)) {
		return sig2;
	} else if (subsort_p(sig2, sig1, sort_table)) {
		return sig1;
	} else {
		ent1 = &sort_table->entries[sig1];
		ent2 = &sort_table->entries[sig2];
		// Now the tricky one
		if (ent1->supersorts == NULL || ent2->supersorts == NULL) {
			return -1;
		}
		lcs = -1;
		for (i = 0; ent1->supersorts[i] != -1; i++) {
			for (j = 0; ent2->supersorts[j] != -1; j++) {
				if (ent1->supersorts[i] == ent2->supersorts[j]) {
					if (lcs == -1 || subsort_p(ent1->supersorts[i], lcs, sort_table)) {
						lcs = ent1->supersorts[i];
					}
				}
			}
		}
		return lcs;
	}
}

// Return a (not necessarily unique) greatest common subsort, or -1 if there is none
int32_t greatest_common_subsort(int32_t sig1, int32_t sig2, sort_table_t *sort_table) {
	int32_t i, j, gcs;
	sort_entry_t *ent1, *ent2;

	if (sig1 == sig2) {
		return sig1;
	}
	// First check if one is a subsort of the other
	if (subsort_p(sig1, sig2, sort_table)) {
		return sig1;
	} else if (subsort_p(sig2, sig1, sort_table)) {
		return sig2;
	} else {
		ent1 = &sort_table->entries[sig1];
		ent2 = &sort_table->entries[sig2];
		// Now the tricky one
		if (ent1->subsorts == NULL || ent2->subsorts == NULL) {
			return -1;
		}
		gcs = -1;
		for (i = 0; ent1->subsorts[i] != -1; i++) {
			for (j = 0; ent2->subsorts[j] != -1; j++) {
				if (ent1->subsorts[i] == ent2->subsorts[j]) {
					if (gcs == -1 || subsort_p(gcs, ent1->subsorts[i], sort_table)) {
						gcs = ent1->subsorts[i];
					}
				}
			}
		}
		return gcs;
	}
}

/* Retrieves a predicate structure by the index */
pred_entry_t *get_pred_entry(pred_table_t *pred_table, int32_t predicate){
	if (predicate <= 0){
		return &(pred_table->evpred_tbl.entries[-predicate]);
	} else {
		return &(pred_table->pred_tbl.entries[predicate]); 
	}
}

int32_t rule_atom_arity(rule_atom_t *atom, pred_table_t *pred_table) {
	return atom->builtinop == 0
		? pred_arity(atom->pred, pred_table)
		: builtin_arity(atom->builtinop);
}

/* Returns the index of an atom, if not exist, returns -1 */
int32_t samp_atom_index(samp_atom_t *atom, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t arity = pred_arity(atom->pred, pred_table);
	array_hmap_pair_t *atom_map = array_size_hmap_find(&atom_table->atom_var_hash,
			arity + 1, (int32_t *) atom);
	if (atom_map == NULL) {
		return -1;
	} else {
		return atom_map->val;
	}
}

// insertion sort
void isort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
	uint32_t i, j;
	int32_t x, y;
	double u, v;

	for (i = 1; i < n; i++) {
		x = a[i];
		u = p[i];
		j = 0;
		while (p[j] > u)
			j++;
		while (j < i) {
			y = a[j];
			v = p[j];
			a[j] = x;
			p[j] = u;
			x = y;
			u = v;
			j++;
		}
		a[j] = x;
		p[j] = u;
	}
}

inline void sort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
	if (n <= 10) {
		isort_query_atoms_and_probs(a, p, n);
	} else {
		qsort_query_atoms_and_probs(a, p, n);
	}
}

// quick sort: requires n > 1
void qsort_query_atoms_and_probs(int32_t *a, double *p, uint32_t n) {
	uint32_t i, j;
	int32_t x, y;
	double u, v;

	// u = random pivot
	// i = random_uint(n);
	i = genrand_uint(n);
	x = a[i];
	u = p[i];

	// swap x and a[0], u and p[0]
	a[i] = a[0];
	p[i] = p[0];
	a[0] = x;
	p[0] = u;

	i = 0;
	j = n;

	do {
		j--;
	} while (p[j] < u);
	do {
		i++;
	} while (i <= j && p[i] > u);

	while (i < j) {
		y = a[i];
		v = p[i];
		a[i] = a[j];
		p[i] = p[j];
		a[j] = y;
		p[j] = v;

		do {
			j--;
		} while (p[j] < u);
		do {
			i++;
		} while (p[i] > u);
	}

	// pivot goes into a[j]
	a[0] = a[j];
	p[0] = p[j];
	a[j] = x;
	p[j] = u;

	// sort a[0...j-1] and a[j+1 .. n-1]
	sort_query_atoms_and_probs(a, p, j);
	j++;
	sort_query_atoms_and_probs(a + j, p + j, n - j);
}

int32_t eval_clause(samp_truth_value_t *assignment, samp_clause_t *clause){
	int32_t i;
	for (i = 0; i < clause->num_lits; i++){
		if (assigned_true_lit(assignment, clause->disjunct[i]))
			return i;
	}
	return -1;
}

int32_t eval_rule_inst(samp_truth_value_t *assignment, rule_inst_t *rinst) {
	int32_t i;
	for (i = 0; i < rinst->num_clauses; i++) {
		if (eval_clause(assignment, rinst->conjunct[i]) == -1) {
			return i;
		}
	}
	return -1;
}

/* Make a copy of the current assignment as a backup */
void copy_assignment_array(atom_table_t *atom_table) {
	samp_truth_value_t *old_assignment = atom_table->assignment;
	atom_table->assignment_index ^= 1; // flip low order bit: 1 --> 0, 0 --> 1
	atom_table->assignment = atom_table->assignments[atom_table->assignment_index];
	memcpy(atom_table->assignment, old_assignment, 
			atom_table->num_vars * sizeof(samp_truth_value_t));
}

/* Restore the backup assignment */
void restore_assignment_array(atom_table_t *atom_table) {
	atom_table->assignment_index ^= 1; // flip low order bit: 1 --> 0, 0 --> 1
	atom_table->assignment = atom_table->assignments[atom_table->assignment_index];
	int32_t i;
	atom_table->num_unfixed_vars = 0;
	for (i = 0; i < atom_table->num_vars; i++) {
		if (unfixed_tval(atom_table->assignment[i])) {
			atom_table->num_unfixed_vars++;
		}
	}
}

bool eq (int32_t i, int32_t j) {
	return i == j;
}

bool neq (int32_t i, int32_t j) {
	return i != j;
}

bool lt (int32_t i, int32_t j) {
	return i < j;
}

bool le (int32_t i, int32_t j) {
	return i <= j;
}

bool gt (int32_t i, int32_t j) {
	return i > j;
}

bool ge (int32_t i, int32_t j) {
	return i >= j;
}

bool plusp (int32_t i, int32_t j, int32_t k) {
	if ((i > 0 && j > 0 && j > INT32_MAX - i)
			|| (i < 0 && j < 0 && j < INT32_MIN - i)) {
		// Overflow
		return false;
	} else {
		return i + j == k;
	}
}

bool minusp (int32_t i, int32_t j, int32_t k) {
	return plusp(i, -j, k);
}

bool timesp (int32_t i, int32_t j, int32_t k) {
	if ((i > 0 && j > 0 && (i > INT32_MAX / j || j > INT32_MAX / i))
			|| (i < 0 && j < 0 && (-i > INT32_MAX / -j || -j > INT32_MAX / -i))) {
		// Overflow
		return false;
	} else {
		return i * j == k;
	}
}

bool divp (int32_t i, int32_t j, int32_t k) {
	return j != 0 && i / j == k;
}

bool remp (int32_t i, int32_t j, int32_t k) {
	return j != 0 && i % j == k;
}

bool (*builtin2[]) (int32_t x, int32_t y) = {eq, neq, lt, le, gt, ge};

bool (*builtin3[]) (int32_t x, int32_t y, int32_t z) = {plusp, minusp, timesp, divp, remp};

extern int32_t builtin_arity (int32_t op) {
	switch (op) {
	case EQ:
	case NEQ:
	case LT:
	case LE:
	case GT:
	case GE:
		return 2;
	case PLUS:
	case MINUS:
	case TIMES:
	case DIV:
	case REM:
		return 3;
	default:
		mcsat_err("Bad operator"); return 0;
	}
}

extern bool call_builtin (int32_t op, int32_t arity, int32_t *args) {
	switch (arity) {
	case 2:
		switch (op) {
		case EQ: return (builtin2[0])(args[0], args[1]);
		case NEQ: return (builtin2[1])(args[0], args[1]);
		case LT: return (builtin2[2])(args[0], args[1]);
		case LE: return (builtin2[3])(args[0], args[1]);
		case GT: return (builtin2[4])(args[0], args[1]);
		case GE: return (builtin2[5])(args[0], args[1]);
		default: mcsat_err("Bad binary operator"); return true;
		}
	case 3:
		switch (op) {
		case PLUS: return (builtin3[0])(args[0], args[1], args[2]);
		case MINUS: return (builtin3[1])(args[0], args[1], args[2]);
		case TIMES: return (builtin3[2])(args[0], args[1], args[2]);
		case DIV: return (builtin3[3])(args[0], args[1], args[2]);
		case REM: return (builtin3[4])(args[0], args[1], args[2]);
		default: mcsat_err("Bad operator"); return true;
		}
	default:
		mcsat_err("Wrong number of args to builtin");
		return true;
	}
}

extern char* builtinop_string (int32_t op) {
	switch (op) {
	case EQ: return "=";
	case NEQ: return "~=";
	case LT: return "<";
	case LE: return "<=";
	case GT: return ">";
	case GE: return ">=";
	case PLUS: return "+";
	case MINUS: return "-";
	case TIMES: return "*";
	case DIV: return "/";
	case REM: return "%";
	default: mcsat_err("Bad operator"); return "???";
	}
}

