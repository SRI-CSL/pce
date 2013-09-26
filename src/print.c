#include <inttypes.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "memalloc.h"
#include "utils.h"
#include "tables.h"
#include "clause_list.h"
#include "buffer.h"
#include "print.h"

pthread_mutex_t pmutex = PTHREAD_MUTEX_INITIALIZER;

static bool output_to_string = false;

static string_buffer_t output_buffer = {0, 0, NULL};
static string_buffer_t error_buffer = {0, 0, NULL};
static string_buffer_t warning_buffer = {0, 0, NULL};

/*
 * verbosity of the output information
 *
 * 0: only the final results;
 * 1: info of every sample;
 * 2: info of every step of MCMC;
 * 3: info of every step of sample SAT;
 * 4: detailed info of sample SAT, such as unit_propagate;
 * 5: info of activating atoms and clauses for lazy MC-SAT;
 * 6: detailed info of lazy MC-SAT;
 * 7: useless info.
 */
static int32_t verbosity_level = 0;

void set_verbosity_level(int32_t v) {
	verbosity_level = v;
}

int32_t get_verbosity_level() {
	return verbosity_level;
}

// Conditional print - only prints if level <= verbosity_level
void cprintf(int32_t level, const char *fmt, ...){
	if(level <= verbosity_level){
		va_list argp;
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	}
}

/*
 * Output to stdout or output_buffer
 *
 * when output_to_string is true, print to output_buffer,
 * otherwise print to stdout
 */
void output(const char *fmt, ...) {
	int32_t out_size, index;
	va_list argp;

	if (output_to_string) {
		pthread_mutex_lock(&pmutex);
		// Find out how big it will be
		va_start(argp, fmt);
		out_size = vsnprintf(NULL, 0, fmt, argp); // Number of chars not include trailing '\0'
		va_end(argp);
		string_buffer_resize(&output_buffer, out_size);
		index = output_buffer.size;
		va_start(argp, fmt);
		vsnprintf(&output_buffer.string[index], out_size+1, fmt, argp);
		va_end(argp);
		output_buffer.size += out_size;
		pthread_mutex_unlock(&pmutex);
	}
	if (!output_to_string) {
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	}
}

/* Errors to stderr or error_buffer */
void mcsat_err(const char *fmt, ...) {
	va_list argp;
	int32_t out_size, index;

	if (output_to_string) {
		pthread_mutex_lock(&pmutex);
		va_start(argp, fmt);
		out_size = vsnprintf(NULL, 0, fmt, argp); // Number of chars not include trailing '\0'
		va_end(argp);
		string_buffer_resize(&error_buffer, out_size);
		index = error_buffer.size;
		va_start(argp, fmt);
		vsnprintf(&error_buffer.string[index], out_size+1, fmt, argp);
		va_end(argp);
		error_buffer.size += out_size;
		pthread_mutex_unlock(&pmutex);
	}
	if (!output_to_string || verbosity_level > 1) {
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
		fprintf(stderr, "\n");
	}
}

/* Errors to stdout or warning_buffer */
void mcsat_warn(const char *fmt, ...) {
	va_list argp;
	int32_t out_size, index;

	if (output_to_string) {
		pthread_mutex_lock(&pmutex);
		va_start(argp, fmt);
		out_size = vsnprintf(NULL, 0, fmt, argp); // Number of chars not include trailing '\0'
		va_end(argp);
		string_buffer_resize(&warning_buffer, out_size);
		index = warning_buffer.size;
		va_start(argp, fmt);
		vsnprintf(&warning_buffer.string[index], out_size+1, fmt, argp);
		va_end(argp);
		warning_buffer.size += out_size;
		pthread_mutex_unlock(&pmutex);
	}
	if (!output_to_string || verbosity_level > 1) {
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	}
}

char *samp_truth_value_string(samp_truth_value_t val){
	return
		val == v_undef ? "U" :
		val == v_false ? "F" :
		val == v_true ?  "T" :
		val == v_up_false ? "fx F" :
		val == v_up_true ? "fx T" :
		val == v_db_false ? "db F" :
		val == v_db_true ? "db T" : "ERROR";
}

static double atom_probability(int32_t atom_index, samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	samp_atom_t *atom;
	double diff;

	atom = atom_table->atom[atom_index];
	if (atom->pred >= 0) { // an indirect predicate
		diff = (double)
			(atom_table->num_samples - atom_table->sampling_nums[atom_index]);
		if (diff > 0) {
			return (double) (atom_table->pmodel[atom_index]/diff);
		} else {
			// No samples - we know nothing
			return .5;
		}
	} else {
		return atom_table->assignment[atom_index] == v_up_true ? 1.0 : 0.0;
	}
}

double query_probability(samp_query_instance_t *qinst, samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	double diff;

	// diff subtracts off the sampling number when the query was introduced
	cprintf(2,"num_samples = %d, sampling_num = %d\n",
			atom_table->num_samples, qinst->sampling_num);
	diff = (double) (atom_table->num_samples - qinst->sampling_num);
	if (diff > 0) {
		return (double) (qinst->pmodel/diff);
	} else {
		// No samples - we know nothing
		return .5;
	}
}
/* Print all the predicates */
void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table){
	char *buffer;
	int32_t i, j; 
	output("num of evpreds: %"PRId32"\n", pred_table->evpred_tbl.num_preds);
	for (i = 0; i < pred_table->evpred_tbl.num_preds; i++){
		output("evpred[%"PRId32"] = ", i);
		buffer = pred_table->evpred_tbl.entries[i].name;
		output("index(%"PRId32"); ", pred_index(buffer, pred_table));
		output( "%s(", buffer);
		for (j = 0; j < pred_table->evpred_tbl.entries[i].arity; j++){
			if (j != 0) output(", ");
			output("%s", sort_table->entries[pred_table->evpred_tbl.entries[i].signature[j]].name);
		}
		output(")\n");
	}
	output("num of preds: %"PRId32"\n", pred_table->pred_tbl.num_preds);
	for (i = 0; i < pred_table->pred_tbl.num_preds; i++){
		output("\n pred[%"PRId32"] = ", i);
		buffer = pred_table->pred_tbl.entries[i].name;
		output("index(%"PRId32"); ", pred_index(buffer, pred_table));
		output("%s(", buffer);
		for (j = 0; j < pred_table->pred_tbl.entries[i].arity; j++){
			if (j != 0) output(", ");
			output("%s", sort_table->entries[pred_table->pred_tbl.entries[i].signature[j]].name);
		}
		output(")\n");
	}
}

void print_atom(samp_atom_t *atom, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	const_table_t *const_table = &table->const_table;
	sort_table_t *sort_table = &table->sort_table;
	sort_entry_t *entry;
	int32_t *psig, i;

	psig = pred_signature(atom->pred, pred_table);  
	output("%s", pred_name(atom->pred, pred_table));
	for (i = 0; i < pred_arity(atom->pred, pred_table); i++){
		i == 0 ? output("(") : output(", ");
		entry = &sort_table->entries[psig[i]];
		if (entry->constants == NULL) {
			// It's an integer
			output("%"PRId32"", atom->args[i]);
		} else {
			if (atom->args[i] < 0) {
				// A variable - print X followed by index
				output("X%"PRId32"", -(atom->args[i]+1));
			} else {
				output("%s", const_name(atom->args[i], const_table));
			}
		}
	}
	output(")");
}

void print_atom_now(samp_atom_t *atom, samp_table_t *table) {
	print_atom(atom, table);
	fflush(stdout);
}

void print_assignment(samp_table_t *table){
	atom_table_t *atom_table = &(table->atom_table);
	int32_t i;

	for (i = 0; i < atom_table->num_vars; i++) {
		if (atom_table->atom[i]->pred <= 0) continue;
		atom_table->active[i] ? output("* ") : output ("  ");
		print_atom(atom_table->atom[i], table);
		output(": %s\n", samp_truth_value_string(atom_table->assignment[i]));
	}
}

void print_atoms(samp_table_t *table){
	atom_table_t *atom_table = &table->atom_table;
	uint32_t nvars = atom_table->num_vars;
	char d[10];
	int nwdth = sprintf(d, "%"PRId32"", nvars);
	int i;

	output("-------------------------------------------------------------------------------\n");
	output("| %*s | #occurs | #samples |  prob  | %-*s |\n", nwdth, "i", 42-nwdth, "atom");
	output("-------------------------------------------------------------------------------\n");
	for (i = 0; i < nvars; i++){
		if (get_print_exp_p()) {
			output("| %-*u | %7d | %8d | %.4e | ", nwdth, i, 
					atom_table->pmodel[i],
					atom_table->num_samples - atom_table->sampling_nums[i],
					atom_probability(i, table));
		} else {
			output("| %-*u | %7d | %8d | %6.4f | ", nwdth, i, 
					atom_table->pmodel[i],
					atom_table->num_samples - atom_table->sampling_nums[i],
					atom_probability(i, table));
		}
		print_atom(atom_table->atom[i], table);
		output("\n");
	}
	output("-------------------------------------------------------------------------------\n");
}

static void print_literal(samp_literal_t lit, samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	if (is_neg(lit)) output("~");
	print_atom(atom_table->atom[var_of(lit)], table);
}

void print_clause(samp_clause_t *clause, samp_table_t *table) {
	int32_t i;
	for (i = 0; i < clause->num_lits; i++) {
		if (i != 0) output(" | ");
		print_literal(clause->disjunct[i], table);
	}
}

void print_rule_instance(rule_inst_t *rinst, samp_table_t *table) {
	int32_t i;
	for (i = 0; i < rinst->num_clauses; i++) {
		if (i != 0) output(" & ");
		if (rinst->num_clauses > 1 && rinst->conjunct[i]->num_lits > 1)
			output("(");
		print_clause(rinst->conjunct[i], table);
		if (rinst->num_clauses > 1 && rinst->conjunct[i]->num_lits > 1)
			output(")");
	}
}

void print_rule_instances(samp_table_t *table){
	rule_inst_table_t *rule_inst_table = &(table->rule_inst_table);
	int32_t nrinsts = rule_inst_table->num_rule_insts;
	char d[20];
	int nwdth = sprintf(d, "%"PRId32"", nrinsts);
	uint32_t i;
	output("-------------------------------------------------------------------------------\n");
	output("|   | %-*s | weight  | %-*s|\n", nwdth, "i", 62-nwdth, "rule instance");
	output("-------------------------------------------------------------------------------\n");
	for (i = 0; i < nrinsts; i++){
		if (assigned_true(rule_inst_table->assignment[i]))
			output("| * ");
		else 
			output("|   ");
		double wt = rule_inst_table->rule_insts[i]->weight;
		if (wt == DBL_MAX) {
			output("| %*"PRIu32" |   MAX   | ", nwdth, i);
		} else {
			output("| %*"PRIu32" | % 7.3f | ", nwdth, i, wt);
		}
		print_rule_instance(rule_inst_table->rule_insts[i], table);
		output("\n");
	}
	output("-------------------------------------------------------------------------------\n");
}

void print_rule_inst_table(samp_table_t *table) {
	print_rule_instances(table);
}

void print_clause_list(samp_clause_list_t *list, samp_table_t *table){
	samp_clause_t *ptr;
	samp_clause_t *cls;

	for (ptr = list->head; ptr != list->tail; ptr = next_clause_ptr(ptr)) {
		cls = ptr->link;
		print_clause(cls, table);
		output("\n");
	}
}

void print_watched_clause(samp_literal_t lit, samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	samp_clause_t *ptr;
	samp_clause_t *cls;
	if (!is_empty_clause_list(&rule_inst_table->watched[lit])) {
		output("[");
		print_literal(lit, table);
		output("]\n");

		for (ptr = rule_inst_table->watched[lit].head;
				ptr != rule_inst_table->watched[lit].tail;
				ptr = next_clause_ptr(ptr)) {
			cls = ptr->link;
			output("    ");
			print_clause(cls, table);
			output("\n");
		}
	}
}

/*
 * Prints live clauses for the current round of MC-SAT
 */
void print_live_clauses(samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t i;

	output("Sat clauses:\n");
	print_clause_list(&rule_inst_table->sat_clauses, table);
	int32_t num_vars = atom_table->num_vars;
	output("Watched clauses:\n");
	for (i = 0; i < num_vars; i++){
		print_watched_clause(pos_lit(i), table);
		print_watched_clause(neg_lit(i), table);
	}
	output("Unsat clauses:\n");
	print_clause_list(&rule_inst_table->unsat_clauses, table);

	//output("Negative/Unit clauses:\n");
	//print_clause_list(&rule_inst_table->negative_or_unit_clauses, table);

	output("Other live clauses:\n");
	print_clause_list(&rule_inst_table->live_clauses, table);

	output("\n");
}

void print_state(samp_table_t *table, uint32_t round){
	if (verbosity_level > 2) {
		output("==============================================================\n");
		print_atoms(table);
		output("==============================================================\n");
		print_rule_inst_table(table);
	} else if (verbosity_level > 1) {
		output("Generated sample %"PRId32"...\n", round);
	} else if (verbosity_level > 0) {
		output(".");
	}
}

/*
 * Prints a rule with a substitution
 */
void print_rule_substit(samp_rule_t *rule, int32_t *substs, samp_table_t *table) {
	int32_t i;
	const_table_t *const_table = &table->const_table;
	for (i = 0; i < rule->num_vars; i++) {
		output(" [%s\\%s]", rule->vars[i]->name,
				substs[i] != INT32_MIN ?  const_name(substs[i], const_table) : "*");
	}
	print_rule(rule, table, 0);
}

void print_rule_atom_arg (rule_atom_arg_t *arg, var_entry_t **vars, const_table_t *const_table) {
	int32_t argidx = arg->value;

	switch (arg->kind) {
	case variable:
		output("%s", vars[argidx]->name);
		break;
	case constant:
		output("%s", const_table->entries[argidx].name);
		break;
	case integer:
		output("%"PRId32"", argidx);
		break;
	}
}

void print_rule_atom (rule_atom_t *ratom, bool neg, var_entry_t **vars,
		samp_table_t *table, int indent) {
	pred_table_t *pred_table = &table->pred_table;
	const_table_t *const_table = &table->const_table;
	int32_t arity, k;

	arity = rule_atom_arity(ratom, pred_table);
	if (ratom->builtinop != 0 && arity == 2) {
		// Infix
		if (neg) output("(");
		print_rule_atom_arg(&ratom->args[0], vars, const_table);
		output(" %s ", builtinop_string(ratom->builtinop));
		print_rule_atom_arg(&ratom->args[1], vars, const_table);
		if (neg) output(")");
	} else {
		// Prefix
		if (ratom->builtinop == 0) {
			output("%s", pred_name(ratom->pred, pred_table));
		} else {
			output("%s", builtinop_string(ratom->builtinop));
		}
		for (k = 0; k < arity; k++) {
			k==0 ? output("(") : output(", ");
			print_rule_atom_arg(&ratom->args[k], vars, const_table);
		}
		output(")");
	}
}

static void print_rule_clause(samp_rule_t *rule, rule_clause_t *rule_clause, 
		samp_table_t *table, int32_t indent) {
	rule_literal_t *rlit;
	int32_t i;
	for (i = 0; i < rule_clause->num_lits; i++) {
		//output("%*s  ", indent, "");
		i==0 ? output("") : output(" | ");
		rlit = rule_clause->literals[i];
		if (rlit->neg) output("~");
		print_rule_atom(rlit->atom, rlit->neg, rule->vars, table, 0);
	}

}

/* Prints a quantified clause */
void print_rule(samp_rule_t *rule, samp_table_t *table, int indent) {
	int32_t i;
	if (rule->num_vars > 0) {
		for (i = 0; i < rule->num_vars; i++) {
			i==0 ? output(" (") : output(", ");
			output("%s", rule->vars[i]->name);
		}
		output(")");
	}
	for (i = 0; i < rule->num_clauses; i++) {
		output("\n%*s", indent, "");
		i==0 ? output("   ") : output(" & ");
		rule_clause_t *rule_clause = rule->clauses[i];	
		if (i > 1 && rule_clause->num_lits > 1)
			output("(");
		print_rule_clause(rule, rule_clause, table, indent);
		if (i > 1 && rule_clause->num_lits > 1)
			output(")");
	}
}

//void print_rule_clause (rule_literal_t **lit, var_entry_t **vars,
//		samp_table_t *table) {
//	int32_t j;
//
//	if (vars != NULL) {
//		for(j = 0; vars[j] != NULL; j++) {
//			j==0 ? output(" (") : output(", ");
//			output("%s", vars[j]->name);
//		}
//		output(")");
//	}
//	for(j=0; lit[j] != NULL; j++) {
//		j==0 ? output("   ") : output(" | ");
//		if (lit[j]->neg) output("~");
//		print_rule_atom(lit[j]->atom, lit[j]->neg, vars, table, 0);
//	}
//}

void print_query_instance(samp_query_instance_t *qinst, samp_table_t *table,
		int32_t indent, bool newlinesp) {
	atom_table_t *atom_table;
	samp_literal_t **lit;
	int32_t i, j, samp_atom;

	atom_table = &table->atom_table;
	lit = qinst->lit;
	for (i = 0; lit[i] != NULL; i++) {
		if (i != 0) {
			if (newlinesp) {
				output(")\n& (");
			} else {
				output(") & (");
			}
		} else {
			if (newlinesp) {
				output("  (");
			} else {
				output(" (");
			}
		}
		for (j = 0; lit[i][j] != -1; j++) {
			if (j != 0) output(" | ");
			samp_atom = var_of(lit[i][j]);
			if (is_neg(lit[i][j])) output("~");
			print_atom(atom_table->atom[samp_atom], table);
		}
	}
	output(")");
}

/* 
 * Returns the string for a literal 
 * remember to free the string after use, the same for atom_string
 * and var_string
 */
char *literal_string(samp_literal_t lit, samp_table_t *table) {
	bool oldout = output_to_string;
	char *result;
	output_to_string = true;
	print_literal(lit, table);
	result = get_string_from_buffer(&output_buffer);
	output_to_string = oldout;
	return result;
}

/* Returns the string for an atom */
char *atom_string(samp_atom_t *atom, samp_table_t *table) {
	bool oldout = output_to_string;
	char *result;
	output_to_string = true;
	print_atom(atom, table);
	result = get_string_from_buffer(&output_buffer);
	output_to_string = oldout;
	return result;
}

/* Returns the string for an atom index */
char *var_string(int32_t var, samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	samp_atom_t *atom = atom_table->atom[var];
	return atom_string(atom, table);
}

///* Returns a newly allocated string */
//char *atom_string(samp_atom_t *atom, samp_table_t *table) {
//	pred_table_t *pred_table = &table->pred_table;
//	const_table_t *const_table = &table->const_table;
//	char *pred, *result;
//	uint32_t i, strsize;
//
//	pred = pred_name(atom->pred, pred_table);
//	strsize = strlen(pred) + 1;
//	for (i = 0; i < pred_arity(atom->pred, pred_table); i++) {
//		/* Assume no more than 99 vars in list */
//		if (atom->args[i] < 0) {
//			strsize += -(atom->args[i]+1) > 9 ? 3 : 2;
//		} else {
//			strsize += strlen(const_name(atom->args[i], const_table));
//		}
//		strsize += 2; // include ", " or ")\0"
//	}
//	result = (char *) safe_malloc(strsize * sizeof(char));
//	strcpy(result, pred);
//	for (i = 0; i < pred_arity(atom->pred, pred_table); i++){
//		i == 0 ? strcat(result, "(") : strcat(result, ", ");
//		if (atom->args[i] < 0) {
//			/* A variable - print X followed by index */
//			sprintf(result, "X%d", -(atom->args[i]+1));
//		} else {
//			sprintf(result, "%s", const_name(atom->args[i], const_table));
//		}
//	}
//	strcat(result, ")");
//	return result;
//}

void dump_sort_table (samp_table_t *table) {
	sort_table_t *sort_table = &(table->sort_table);
	const_table_t *const_table = &(table->const_table);
	// First get the sort length
	int i, j;
	int slen = 4;
	for (i=0; i<sort_table->num_sorts; i++) {
		slen = imax(slen, strlen(sort_table->entries[i].name));
	}
	output("| %-*s | Constants\n", slen, "Sort");
	output("--------------------------\n");
	for (i=0; i<sort_table->num_sorts; i++) {
		sort_entry_t entry = sort_table->entries[i];
		output("| %-*s ", slen, entry.name);
		int32_t clen = slen;
		if (entry.constants == NULL) {
			if (entry.ints == NULL) {
				// Have a subrange
				output("| [%"PRId32" .. %"PRId32"]", entry.lower_bound, entry.upper_bound);
			} else {
				output("| INTEGER: {");
				for (j=0; j<entry.cardinality; j++) {
					if (j != 0) output(", ");
					output("%"PRId32"", entry.ints[j]);
				}
				output("}");
			}
		} else {
			for (j=0; j<entry.cardinality; j++){
				char *nstr = const_table->entries[entry.constants[j]].name;
				j == 0 ? output("| ") : output(", ");
				clen += strlen(nstr) + 2;
				if (clen > 72) {
					clen = slen;
					output("\n| %-*s | %s", slen, "", nstr);
				} else {
					output("%s", nstr);
				}
			}
		}
		output("\n");
	}
	output("--------------------------\n");
}

static void dump_pred_tbl (pred_tbl_t * pred_tbl, sort_table_t * sort_table) {
	pred_entry_t * pred_entry = pred_tbl->entries;
	int i, j;
	// Start at 1 to skip the true() predicate
	for (i = 1; i < pred_tbl->num_preds; i++) {
		output("|  %s(", pred_entry[i].name);
		for (j = 0; j < pred_entry[i].arity; j++) {
			if (j != 0) output(", ");
			int32_t index = pred_entry[i].signature[j];
			output("%s", sort_table->entries[index].name);
		}
		output(")\n");
	}
}

void dump_pred_table (samp_table_t *table) {
	sort_table_t *sort_table = &(table->sort_table);
	pred_table_t *pred_table = &(table->pred_table);
	output("\n-------------------------------\n");
	output("| Evidence predicates:\n");
	output("-------------------------------\n");
	dump_pred_tbl(&(pred_table->evpred_tbl), sort_table);
	output("-------------------------------\n");
	output("| Non-evidence predicates:\n");
	output("-------------------------------\n");
	dump_pred_tbl(&(pred_table->pred_tbl), sort_table);
	output("-------------------------------\n");
}

void dump_const_table (const_table_t * const_table, sort_table_t * sort_table) {
	output("\nPrinting constants:\n");
	int i;
	for (i = 0; i < const_table->num_consts; i++) {
		output(" %s: ", const_table->entries[i].name);
		uint32_t index = const_table->entries[i].sort_index;
		output("%s\n", sort_table->entries[index].name);
	}
}

void dump_var_table (var_table_t * var_table, sort_table_t * sort_table) {
	output("\nPrinting variables:\n");
	int i;
	for (i = 0; i < var_table->num_vars; i++) {
		output(" %s: ", var_table->entries[i].name);
		uint32_t index = var_table->entries[i].sort_index;
		output("%s\n", sort_table->entries[index].name);
	}
}

void dump_atom_table (samp_table_t *table) {
	output("\nAtom Table:\n");
	print_atoms(table);
	print_assignment(table);
}

void dump_rule_inst_table (samp_table_t *table) {
	output("\nRule Instance Table:\n");
	print_rule_instances(table);
}

void dump_rule_table (samp_table_t *samp_table) {
	rule_table_t *rule_table = &(samp_table->rule_table);
	uint32_t nrules = rule_table->num_rules;
	char d[10];
	int nwdth = sprintf(d, "%"PRIu32, nrules);
	int32_t i;
	output("\nRule Table:\n");
	output("--------------------------------------------------------------------------------\n");
	output("| %-*s | weight    | %-*s |\n", nwdth, "i", 61-nwdth, "Rule");
	output("--------------------------------------------------------------------------------\n");
	for(i=0; i<nrules; i++) {
		samp_rule_t *rule = rule_table->samp_rules[i];
		double wt = rule_table->samp_rules[i]->weight;
		if (wt == DBL_MAX) {
			output("| %*"PRIu32" |    MAX    | ", nwdth, i);
		} else {
			output("| %*"PRIu32" | % 9.3f | ", nwdth, i, wt);
		}
		print_rule(rule, samp_table, nwdth+17);
		output("\n");
	}
	output("-------------------------------------------------------------------------------\n");
}

void dump_query_instance_table (samp_table_t *samp_table) {
	query_instance_table_t *query_instance_table = &samp_table->query_instance_table;
	samp_query_instance_t *qinst;
	uint32_t nqueries = query_instance_table->num_queries;
	char d[10];
	int nwdth = sprintf(d, "%"PRIu32, nqueries);
	int32_t i;

	output("\nQuery Instance Table:\n");
	output("--------------------------------------------------------------------------------\n");
	output("| %-*s |  prob  | %-*s |\n", nwdth, "i", 64-nwdth, "Rule");
	output("--------------------------------------------------------------------------------\n");
	for(i=0; i<nqueries; i++) {
		qinst = query_instance_table->query_inst[i];
		if (get_print_exp_p()) {
			output("| %-*u | %.4e | ", nwdth, i, query_probability(qinst, samp_table));
		} else {
			output("| %-*u | %6.4f |", nwdth, i, query_probability(qinst, samp_table));
		}
		print_query_instance(qinst, samp_table, 0, false);
		printf("\n");

		//print_query_instance(qinst, samp_table, nwdth+17, false);
		//if (get_print_exp_p()) {
		//	output(" : % .4e\n", query_probability(qinst, samp_table));
		//} else {
		//	output(" : % 11.4f\n", query_probability(qinst, samp_table));
		//}
		//output("\n");
	}
	output("-------------------------------------------------------------------------------\n");
}

void dump_all_tables(samp_table_t *table) {
	cprintf(1, "Dumping tables...\n");
	dump_sort_table(table);
	dump_pred_table(table);
	//dump_const_table(const_table, sort_table);
	//dump_var_table(var_table, sort_table);
	dump_atom_table(table);
	dump_rule_inst_table(table);
	dump_rule_table(table);
}

void summarize_sort_table (samp_table_t *table) {
	sort_table_t *sort_table = &(table->sort_table);
	sort_entry_t *entry;
	int32_t i, total;

	output("Sort table has %"PRId32" entries\n",
			sort_table->num_sorts);
	total = 0;
	for (i = 0; i < sort_table->num_sorts; i++) {
		entry = &sort_table->entries[i];
		total += entry->cardinality;
		output("    %s: %"PRId32"\n", entry->name, entry->cardinality);
	}
	output("  Total: %"PRId32"\n", total);
}

void summarize_pred_table (samp_table_t *table) {
	uint32_t obsnum, hidnum;

	obsnum = table->pred_table.evpred_tbl.num_preds;
	hidnum = table->pred_table.pred_tbl.num_preds;
	output("Predicate table has %"PRId32" total entries;\n", obsnum+hidnum);
	output("  %"PRId32" observable and %"PRId32" hidden\n", obsnum, hidnum);
}

void summarize_atom_table (samp_table_t *table) {
	output("Atom table has %"PRId32" entries\n",
			table->atom_table.num_vars);
}

void summarize_rule_inst_table (samp_table_t *table) {
	output("Clause table has %"PRId32" entries\n",
			table->rule_inst_table.num_rule_insts);
}

void summarize_rule_table (samp_table_t *table) {
	output("Rule table has %"PRId32" entries\n",
			table->rule_table.num_rules);
}

void summarize_tables(samp_table_t *table) {
	output("\n");
	summarize_sort_table(table);
	summarize_pred_table(table);
	//summarize_const_table(const_table, sort_table);
	//summarize_var_table(var_table, sort_table);
	summarize_atom_table(table);
	summarize_rule_inst_table(table);
	summarize_rule_table(table);
}

