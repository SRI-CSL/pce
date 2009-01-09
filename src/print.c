#include <inttypes.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "utils.h"
#include "tables.h"
#include "print.h"

static int32_t verbosity_level = 1;

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
  pred_table_t *pred_table = &(table->pred_table);
  const_table_t *const_table = &(table->const_table);
  uint32_t i;
  printf("%s", pred_name(atom->pred, pred_table));
  for (i = 0; i < pred_arity(atom->pred, pred_table); i++){
    i == 0 ? printf("(") : printf(", ");
    if (atom->args[i] < 0) {
      // A variable - print X followed by index
      printf("X%d", -(atom->args[i]+1));
    } else {
      printf("%s", const_name(atom->args[i], const_table));
    }
  }
  printf(")");
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
  atom_table_t *atom_table = &(table->atom_table);
  uint32_t nvars = atom_table->num_vars;
  char d[10];
  int nwdth = sprintf(d, "%"PRId32"", nvars);
  int i;
  
  printf("--------------------------------------------------------------------------------\n");
  printf("| %*s | tval | prob   | %-*s |\n", nwdth, "i", 57-nwdth, "atom");
  printf("--------------------------------------------------------------------------------\n");
  for (i = 0; i < nvars; i++){
    samp_truth_value_t tv = atom_table->assignment[atom_table->current_assignment][i];
    printf("| %-*u | %-4s | % 5.3f | ",
	   nwdth, i, samp_truth_value_string(tv),
	   atom_probability(i, table));
    print_atom(atom_table->atom[i], table);
    printf("\n");
  }
  printf("--------------------------------------------------------------------------------\n");
}

void print_clause(samp_clause_t *clause, samp_table_t *table){
  atom_table_t *atom_table = &table->atom_table;
  int32_t i;
  for (i = 0; i<clause->numlits; i++){
    if (i != 0) printf(" | ");
    int32_t samp_atom = var_of(clause->disjunct[i]);
    if (is_neg(clause->disjunct[i])) printf("~");
    print_atom(atom_table->atom[samp_atom], table);
  }
}

void print_clauses(samp_table_t *table){
  clause_table_t *clause_table = &(table->clause_table);
  int32_t nclauses = clause_table->num_clauses;
  char d[20];
  int nwdth = sprintf(d, "%"PRId32"", nclauses);
  uint32_t i;
  printf("\n-------------------------------------------------------------------------------\n");
  printf("| %-*s | weight    | %-*s|\n", nwdth, "i", 62-nwdth, "clause");
  printf("-------------------------------------------------------------------------------\n");
  for (i = 0; i < nclauses; i++){
    double wt = clause_table->samp_clauses[i]->weight;
    if (wt == DBL_MAX) {
      printf("| %*"PRIu32" |    MAX    | ", nwdth, i);
    } else {
      printf("| %*"PRIu32" | % 9.3f | ", nwdth, i, wt);
    }
    print_clause(clause_table->samp_clauses[i], table);
    printf("\n");
  }
  printf("-------------------------------------------------------------------------------\n");
}

void print_clause_list(samp_clause_t *link, samp_table_t *table){
  while (link != NULL){
    print_clause(link, table);
    link = link->link;
  }
  printf("\n");
 }

void print_clause_table(samp_table_t *table, int32_t num_vars){
  clause_table_t *clause_table = &(table->clause_table);
  print_clauses(table);
  printf("\n");
  printf("Sat clauses: \n");
  samp_clause_t *link = clause_table->sat_clauses;
  print_clause_list(link, table);
  printf("Unsat clauses: \n");
  link = clause_table->unsat_clauses;
  print_clause_list(link, table);  
  printf("Watched clauses: \n");
  int32_t i, lit;
  for (i = 0; i < num_vars; i++){
    lit = pos_lit(i);
    printf("lit[%"PRId32"]: ", lit);
    link = clause_table->watched[lit];
    print_clause_list(link, table);
    lit = neg_lit(i);
    printf("lit[%"PRId32"]: ", lit);
    link = clause_table->watched[lit];
    print_clause_list(link, table);
  }
  printf("Dead clauses:\n");
  link = clause_table->dead_clauses;
  print_clause_list(link, table);
  printf("Negative/Unit clauses:\n");
  link = clause_table->negative_or_unit_clauses;
  print_clause_list(link, table);
  printf("Dead negative/unit clauses:\n");
  link = clause_table->dead_negative_or_unit_clauses;
  print_clause_list(link, table);
}


void print_state(samp_table_t *table, uint32_t round){
  atom_table_t *atom_table = &(table->atom_table);
  if (verbosity_level > 2) {
    printf("==============================================================\n");
    print_atoms(table);
    printf("==============================================================\n");
    print_clause_table(table, atom_table->num_vars);
  } else if (verbosity_level > 1) {
    printf("Generated sample %"PRId32"...\n", round);
  } else if (verbosity_level > 0) {
    printf(".");
  }
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
  printf("| %-*s | Constants\n", slen, "Sort");
  printf("--------------------------\n");
  for (i=0; i<sort_table->num_sorts; i++) {
    sort_entry_t entry = sort_table->entries[i];
    printf("| %-*s ", slen, entry.name);
    int32_t clen = slen;
    for (j=0; j<entry.cardinality; j++){
      char *nstr = const_table->entries[entry.constants[j]].name;
      j == 0 ? printf("| ") : printf(", ");
      clen += strlen(nstr) + 2;
      if (clen > 72) {
	clen = slen;
	printf("\n| %-*s | %s", slen, "", nstr);
      } else {
	printf("%s", nstr);
      }
    }
    printf("\n");
  }
  printf("--------------------------\n");
}

static void dump_pred_tbl (pred_tbl_t * pred_tbl, sort_table_t * sort_table) {
  pred_entry_t * pred_entry = pred_tbl->entries;
  int i, j;
  // Start at 1 to skip the true() predicate
  for (i = 1; i < pred_tbl->num_preds; i++) {
    printf("|  %s(", pred_entry[i].name);
    for (j = 0; j < pred_entry[i].arity; j++) {
      if (j != 0) printf(", ");
      int32_t index = pred_entry[i].signature[j];
      printf("%s", sort_table->entries[index].name);
    }
    printf(")\n");
  }
}

extern void dump_pred_table (samp_table_t *table) {
  sort_table_t *sort_table = &(table->sort_table);
  pred_table_t *pred_table = &(table->pred_table);
  printf("-------------------------------\n");
  printf("| Evidence predicates:\n");
  printf("-------------------------------\n");
  dump_pred_tbl(&(pred_table->evpred_tbl), sort_table);
  printf("-------------------------------\n");
  printf("| Non-evidence predicates:\n");
  printf("-------------------------------\n");
  dump_pred_tbl(&(pred_table->pred_tbl), sort_table);
  printf("-------------------------------\n");
}

void dump_const_table (const_table_t * const_table, sort_table_t * sort_table) {
  printf("Printing constants:\n");
  int i;
  for (i = 0; i < const_table->num_consts; i++) {
    printf(" %s: ", const_table->entries[i].name);
    uint32_t index = const_table->entries[i].sort_index;
    printf("%s\n", sort_table->entries[index].name);
  }
}

void dump_var_table (var_table_t * var_table, sort_table_t * sort_table) {
  printf("Printing variables:\n");
  int i;
  for (i = 0; i < var_table->num_vars; i++) {
    printf(" %s: ", var_table->entries[i].name);
    uint32_t index = var_table->entries[i].sort_index;
    printf("%s\n", sort_table->entries[index].name);
  }
}

extern void dump_atom_table (samp_table_t *table) {
  print_atoms(table);
}

extern void dump_clause_table (samp_table_t *table) {
  printf("Clause Table:\n");
  print_clauses(table);
}

void print_rule (samp_rule_t *rule, samp_table_t *table, int indent) {
  pred_table_t *pred_table = &(table->pred_table);
  const_table_t *const_table = &(table->const_table);
  rule_literal_t *lit;
  samp_atom_t *atom;
  int32_t j, k, argidx, pred_idx;

  if (rule->num_vars > 0) {
    for(j = 0; j < rule->num_vars; j++) {
      j==0 ? printf(" (") : printf(", ");
      printf("%s", rule->vars[j]->name);
    }
    printf(")");
  }
  for(j=0; j<rule->num_lits; j++) {
    printf("\n%*s  ", indent, "");
    j==0 ? printf("   ") : printf(" | ");
    lit = rule->literals[j];
    if (lit->neg) printf("~");
    atom = lit->atom;
    pred_idx = atom->pred;
    printf("%s", pred_name(pred_idx, pred_table));
    for(k = 0; k < pred_arity(pred_idx, pred_table); k++) {
      k==0 ? printf("(") : printf(", ");
      argidx = atom->args[k];
      if(argidx < 0) {
	// Must be a variable - look it up in the vars
	printf("%s", rule->vars[-(argidx + 1)]->name);
      } else {
	// Look it up in the const_table
	printf("%s", const_table->entries[argidx].name);
      }
    }
    printf(")");
  }
}

extern void dump_rule_table (samp_table_t *samp_table) {
  rule_table_t *rule_table = &(samp_table->rule_table);
  uint32_t nrules = rule_table->num_rules;
  char d[10];
  int nwdth = sprintf(d, "%"PRIu32, nrules);
  int32_t i;
  printf("Rule Table:\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("| %-*s | weight    | %-*s |\n", nwdth, "i", 61-nwdth, "Rule");
  printf("--------------------------------------------------------------------------------\n");
  for(i=0; i<nrules; i++) {
    samp_rule_t *rule = rule_table->samp_rules[i];
    double wt = rule_table->samp_rules[i]->weight;
    if (wt == DBL_MAX) {
      printf("| %*"PRIu32" |    MAX    | ", nwdth, i);
    } else {
      printf("| %*"PRIu32" | % 9.3f | ", nwdth, i, wt);
    }
    print_rule(rule, samp_table, nwdth+17);
    printf("\n");
  }
  printf("-------------------------------------------------------------------------------\n");
}

extern void print_query_instance(samp_query_instance_t *qinst, samp_table_t *table, int32_t indent) {
  atom_table_t *atom_table;
  samp_literal_t **lit;
  int32_t i, j, samp_atom;

  atom_table = &table->atom_table;
  lit = qinst->lit;
  for (i = 0; lit[i] != NULL; i++) {
    if (i != 0) printf(")\n& ("); else printf("  (");
    for (j = 0; lit[i][j] != -1; j++) {
      if (j != 0) printf(" | ");
      samp_atom = var_of(lit[i][j]);
      if (is_neg(lit[i][j])) printf("~");
      print_atom(atom_table->atom[samp_atom], table);
    }
  }
  printf(")");
}

extern void dump_query_instance_table (samp_table_t *samp_table) {
  query_instance_table_t *query_instance_table = &samp_table->query_instance_table;
  samp_query_instance_t *qinst;
  uint32_t nqueries = query_instance_table->num_queries;
  char d[10];
  int nwdth = sprintf(d, "%"PRIu32, nqueries);
  int32_t i;
  
  printf("Query Instance Table:\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("| %-*s | weight    | %-*s |\n", nwdth, "i", 61-nwdth, "Rule");
  printf("--------------------------------------------------------------------------------\n");
  for(i=0; i<nqueries; i++) {
    qinst = query_instance_table->query_inst[i];
    print_query_instance(qinst, samp_table, nwdth+17);
    printf("\n");
  }
  printf("-------------------------------------------------------------------------------\n");
}

double atom_probability(int32_t atom_index, samp_table_t *table) {
  atom_table_t *atom_table = &table->atom_table;
  double diff;

  diff = (double)
    (atom_table->num_samples - atom_table->sampling_nums[atom_index]);
  if (diff > 0) {
    return (double) (atom_table->pmodel[atom_index]/diff);
  } else {
    // No samples - we know nothing
    return .5;
  }
}

double query_probability(samp_query_instance_t *qinst, samp_table_t *table) {
  atom_table_t *atom_table = &table->atom_table;
  double diff;

  // diff subtracts off the sampling number when the query was introduced
  diff = (double) (atom_table->num_samples - qinst->sampling_num);
  if (diff > 0) {
    return (double) (qinst->pmodel/diff);
  } else {
    // No samples - we know nothing
    return .5;
  }
}
