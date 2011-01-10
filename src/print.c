#include <inttypes.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include "memalloc.h"
#include "utils.h"
#include "tables.h"
#include "print.h"

#define INIT_STRING_BUFFER_SIZE 32

pthread_mutex_t pmutex = PTHREAD_MUTEX_INITIALIZER;

bool output_to_string = false;

string_buffer_t output_buffer = {0, 0, NULL};
string_buffer_t error_buffer = {0, 0, NULL};
string_buffer_t warning_buffer = {0, 0, NULL};

static int32_t verbosity_level = 0;

extern void set_verbosity_level(int32_t v) {
  verbosity_level = v;
}

extern int32_t get_verbosity_level() {
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

extern char *get_string_from_buffer (string_buffer_t *strbuf) {
  char *new_str;
  if (strbuf->size == 0) {
    return "";
  } else {
    new_str = (char *) safe_malloc((strbuf->size+1) * sizeof(char));
    strcpy(new_str, strbuf->string);
    strbuf->size = 0;
    return new_str;
  }
}

void string_buffer_resize (string_buffer_t *strbuf, int32_t delta) {
  if (strbuf->capacity == 0) {
    strbuf->string = (char *)
      safe_malloc(INIT_STRING_BUFFER_SIZE * sizeof(char));
    strbuf->capacity = INIT_STRING_BUFFER_SIZE;
  }
  uint32_t size = strbuf->size;
  uint32_t capacity = strbuf->capacity;
  if (size+delta >= capacity){
    if (MAX_SIZE(sizeof(char), 0) - capacity <= delta){
      out_of_memory();
    }
    capacity += delta;
    strbuf->string = (char *)
      safe_realloc(strbuf->string, capacity * sizeof(char));
    strbuf->capacity = capacity;
  }
}

// Output to stdout or output_buffer
extern void output(const char *fmt, ...) {
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
  if (!output_to_string || verbosity_level > 1) {
    va_start(argp, fmt);
    vprintf(fmt, argp);
    va_end(argp);
  }
}

// Errors to stderr or error_buffer
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

// Errors to stdout or warning_buffer
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

void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table){
  char *buffer;
  int32_t i, j; 
   printf("num of evpreds: %"PRId32"\n", pred_table->evpred_tbl.num_preds);
   for (i = 0; i < pred_table->evpred_tbl.num_preds; i++){
     printf("evpred[%"PRId32"] = ", i);
      buffer = pred_table->evpred_tbl.entries[i].name;
      printf("index(%"PRId32"); ", pred_index(buffer, pred_table));
      printf( "%s(", buffer);
      for (j = 0; j < pred_table->evpred_tbl.entries[i].arity; j++){
	if (j != 0) printf(", ");
	printf("%s", sort_table->entries[pred_table->evpred_tbl.entries[i].signature[j]].name);
      }
      printf(")\n");
  }
   printf("num of preds: %"PRId32"\n", pred_table->pred_tbl.num_preds);
   for (i = 0; i < pred_table->pred_tbl.num_preds; i++){
     printf("\n pred[%"PRId32"] = ", i);
     buffer = pred_table->pred_tbl.entries[i].name;
     printf("index(%"PRId32"); ", pred_index(buffer, pred_table));
     printf("%s(", buffer);
    for (j = 0; j < pred_table->pred_tbl.entries[i].arity; j++){
      if (j != 0) printf(", ");
      printf("%s", sort_table->entries[pred_table->pred_tbl.entries[i].signature[j]].name);
    }
    printf(")\n");
  }
}

char *literal_string(samp_literal_t lit, samp_table_t *table) {
  bool oldout = output_to_string;
  char *result;
  output_to_string = true;
  print_literal(lit, table);
  result = get_string_from_buffer(&output_buffer);
  output_to_string = oldout;
  return result;
}

// Returns a newly allocated string
// char *atom_string(samp_atom_t *atom, samp_table_t *table) {
//   pred_table_t *pred_table = &table->pred_table;
//   const_table_t *const_table = &table->const_table;
//   char *pred, *result;
//   uint32_t i, strsize;

//   pred = pred_name(atom->pred, pred_table);
//   strsize = strlen(pred) + 1;
//   for (i = 0; i < pred_arity(atom->pred, pred_table); i++) {
//     // Assume no more than 99 vars in list
//     if (atom->args[i] < 0) {
//       strsize += -(atom->args[i]+1) > 9 ? 3 : 2;
//     } else {
//       strsize += strlen(const_name(atom->args[i], const_table));
//     }
//     strsize += 2; // include ", " or ")\0"
//   }
//   result = (char *) safe_malloc(strsize * sizeof(char));
//   strcpy(result, pred);
//   for (i = 0; i < pred_arity(atom->pred, pred_table); i++){
//     i == 0 ? strcat(result, "(") : strcat(result, ", ");
//     if (atom->args[i] < 0) {
//       // A variable - print X followed by index
//       sprintf(result, "X%d", -(atom->args[i]+1));
//     } else {
//       sprintf(result, "%s", const_name(atom->args[i], const_table));
//     }
//   }
//   strcat(result, ")");
//   return result;
// }
  

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
  printf("\n");
  fflush(stdout);
}

char *samp_truth_value_string(samp_truth_value_t val){
  return
    val == v_undef ? "U" :
    val == v_false ? "F" :
    val == v_true ?  "T" :
    val == v_fixed_false ? "fx F" :
    val == v_fixed_true ? "fx T" :
    val == v_db_false ? "db F" :
    val == v_db_true ? "db T" : "ERROR";
}

void print_atoms(samp_table_t *table){
  atom_table_t *atom_table = &table->atom_table;
  uint32_t nvars = atom_table->num_vars;
  char d[10];
  int nwdth = sprintf(d, "%"PRId32"", nvars);
  int i;
  
  output("--------------------------------------------------------------------------------\n");
  output("| %*s | probability | %-*s |\n", nwdth, "i", 65-nwdth, "atom");
  output("--------------------------------------------------------------------------------\n");
  for (i = 0; i < nvars; i++){
    if (get_print_exp_p()) {
      output("| %-*u | % .4e | ", nwdth, i, atom_probability(i, table));
    } else {
      output("| %-*u | % 11.4f | ", nwdth, i, atom_probability(i, table));
    }
    print_atom(atom_table->atom[i], table);
    output("\n");
  }
  output("--------------------------------------------------------------------------------\n");
}

void print_literal(samp_literal_t lit, samp_table_t *table) {
  atom_table_t *atom_table = &table->atom_table;
  if (is_neg(lit)) output("~");
  print_atom(atom_table->atom[var_of(lit)], table);
}

void print_clause(samp_clause_t *clause, samp_table_t *table) {
  int32_t i;
  for (i = 0; i<clause->numlits; i++) {
    if (i != 0) output(" | ");
    print_literal(clause->disjunct[i], table);
  }
}

void print_clauses(samp_table_t *table){
  clause_table_t *clause_table = &(table->clause_table);
  int32_t nclauses = clause_table->num_clauses;
  char d[20];
  int nwdth = sprintf(d, "%"PRId32"", nclauses);
  uint32_t i;
  output("\n-------------------------------------------------------------------------------\n");
  output("| %-*s | weight    | %-*s|\n", nwdth, "i", 62-nwdth, "clause");
  output("-------------------------------------------------------------------------------\n");
  for (i = 0; i < nclauses; i++){
    double wt = clause_table->samp_clauses[i]->weight;
    if (wt == DBL_MAX) {
      output("| %*"PRIu32" |    MAX    | ", nwdth, i);
    } else {
      output("| %*"PRIu32" | % 9.3f | ", nwdth, i, wt);
    }
    print_clause(clause_table->samp_clauses[i], table);
    output("\n");
  }
  output("-------------------------------------------------------------------------------\n");
}

void print_clause_list(samp_clause_t *link, samp_table_t *table){
  while (link != NULL){
    print_clause(link, table);
    link = link->link;
  }
  output("\n");
 }

void print_clause_table(samp_table_t *table, int32_t num_vars){
  clause_table_t *clause_table = &(table->clause_table);
  print_clauses(table);
  output("\n");
  output("Sat clauses: \n");
  samp_clause_t *link = clause_table->sat_clauses;
  print_clause_list(link, table);
  output("Unsat clauses: \n");
  link = clause_table->unsat_clauses;
  print_clause_list(link, table);  
  output("Watched clauses: \n");
  int32_t i, lit;
  for (i = 0; i < num_vars; i++){
    lit = pos_lit(i);
    output("lit[%"PRId32"]: ", lit);
    link = clause_table->watched[lit];
    print_clause_list(link, table);
    lit = neg_lit(i);
    output("lit[%"PRId32"]: ", lit);
    link = clause_table->watched[lit];
    print_clause_list(link, table);
  }
  output("Dead clauses:\n");
  link = clause_table->dead_clauses;
  print_clause_list(link, table);
  output("Negative/Unit clauses:\n");
  link = clause_table->negative_or_unit_clauses;
  print_clause_list(link, table);
  output("Dead negative/unit clauses:\n");
  link = clause_table->dead_negative_or_unit_clauses;
  print_clause_list(link, table);
}


void print_state(samp_table_t *table, uint32_t round){
  atom_table_t *atom_table = &(table->atom_table);
  if (verbosity_level > 2) {
    output("==============================================================\n");
    print_atoms(table);
    output("==============================================================\n");
    print_clause_table(table, atom_table->num_vars);
  } else if (verbosity_level > 1) {
    output("Generated sample %"PRId32"...\n", round);
  } else if (verbosity_level > 0) {
    output(".");
  }
}

void print_assignment(samp_table_t *table){
  atom_table_t *atom_table = &(table->atom_table);
  samp_truth_value_t *assignment =
    atom_table->assignment[atom_table->current_assignment];
  int32_t i;
  
  for (i = 0; i < atom_table->num_vars; i++) {
    print_atom(atom_table->atom[i], table);
    assigned_true(assignment[i]) ? output(": T ") : output(": F ");
  }
  printf("\n");
  fflush(stdout);
}

  
extern void dump_sort_table (samp_table_t *table) {
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

extern void dump_pred_table (samp_table_t *table) {
  sort_table_t *sort_table = &(table->sort_table);
  pred_table_t *pred_table = &(table->pred_table);
  output("-------------------------------\n");
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
  output("Printing constants:\n");
  int i;
  for (i = 0; i < const_table->num_consts; i++) {
    output(" %s: ", const_table->entries[i].name);
    uint32_t index = const_table->entries[i].sort_index;
    output("%s\n", sort_table->entries[index].name);
  }
}

void dump_var_table (var_table_t * var_table, sort_table_t * sort_table) {
  output("Printing variables:\n");
  int i;
  for (i = 0; i < var_table->num_vars; i++) {
    output(" %s: ", var_table->entries[i].name);
    uint32_t index = var_table->entries[i].sort_index;
    output("%s\n", sort_table->entries[index].name);
  }
}

extern void dump_atom_table (samp_table_t *table) {
  print_atoms(table);
}

extern void dump_clause_table (samp_table_t *table) {
  output("Clause Table:\n");
  print_clauses(table);
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

  arity = atom_arity(ratom, pred_table);
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

void print_rule (samp_rule_t *rule, samp_table_t *table, int indent) {
  rule_literal_t *lit;
  int32_t j;

  if (rule->num_vars > 0) {
    for(j = 0; j < rule->num_vars; j++) {
      j==0 ? output(" (") : output(", ");
      output("%s", rule->vars[j]->name);
    }
    output(")");
  }
  for(j=0; j<rule->num_lits; j++) {
    output("\n%*s  ", indent, "");
    j==0 ? output("   ") : output(" | ");
    lit = rule->literals[j];
    if (lit->neg) output("~");
    print_rule_atom(lit->atom, lit->neg, rule->vars, table, indent);
  }
}

void print_rule_clause (rule_literal_t **lit, var_entry_t **vars,
			samp_table_t *table) {
  int32_t j;

  if (vars != NULL) {
    for(j = 0; vars[j] != NULL; j++) {
      j==0 ? output(" (") : output(", ");
      output("%s", vars[j]->name);
    }
    output(")");
  }
  for(j=0; lit[j] != NULL; j++) {
    j==0 ? output("   ") : output(" | ");
    if (lit[j]->neg) output("~");
    print_rule_atom(lit[j]->atom, lit[j]->neg, vars, table, 0);
  }
}

extern void dump_rule_table (samp_table_t *samp_table) {
  rule_table_t *rule_table = &(samp_table->rule_table);
  uint32_t nrules = rule_table->num_rules;
  char d[10];
  int nwdth = sprintf(d, "%"PRIu32, nrules);
  int32_t i;
  output("Rule Table:\n");
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

extern void print_query_instance(samp_query_instance_t *qinst, samp_table_t *table,
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

extern void dump_query_instance_table (samp_table_t *samp_table) {
  query_instance_table_t *query_instance_table = &samp_table->query_instance_table;
  samp_query_instance_t *qinst;
  uint32_t nqueries = query_instance_table->num_queries;
  char d[10];
  int nwdth = sprintf(d, "%"PRIu32, nqueries);
  int32_t i;
  
  output("Query Instance Table:\n");
  output("--------------------------------------------------------------------------------\n");
  output("| %-*s | weight    | %-*s |\n", nwdth, "i", 61-nwdth, "Rule");
  output("--------------------------------------------------------------------------------\n");
  for(i=0; i<nqueries; i++) {
    qinst = query_instance_table->query_inst[i];
    print_query_instance(qinst, samp_table, nwdth+17, true);
    if (get_print_exp_p()) {
      output(" : % .4e\n", query_probability(qinst, samp_table));
    } else {
      output(" : % 11.4f\n", query_probability(qinst, samp_table));
    }
    output("\n");
  }
  output("-------------------------------------------------------------------------------\n");
}

extern void summarize_sort_table (samp_table_t *table) {
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

extern void summarize_pred_table (samp_table_t *table) {
  uint32_t obsnum, hidnum;

  obsnum = table->pred_table.evpred_tbl.num_preds;
  hidnum = table->pred_table.pred_tbl.num_preds;
  output("Predicate table has %"PRId32" total entries;\n", obsnum+hidnum);
  output("  %"PRId32" observable and %"PRId32" hidden\n", obsnum, hidnum);
}

extern void summarize_atom_table (samp_table_t *table) {
  output("Atom table has %"PRId32" entries\n",
	 table->atom_table.num_vars);
}

extern void summarize_clause_table (samp_table_t *table) {
  output("Clause table has %"PRId32" entries\n",
	 table->clause_table.num_clauses);
}

extern void summarize_rule_table (samp_table_t *table) {
  output("Rule table has %"PRId32" entries\n",
	 table->rule_table.num_rules);
}


double atom_probability(int32_t atom_index, samp_table_t *table) {
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
    return atom_table->assignment[0][atom_index] == v_fixed_true ? 1.0 : 0.0;
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
