#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <libicl.h>
#include <liboaa.h>
#include "utils.h"
#include "mcsat.h"
#include "y.tab.h"
#include "samplesat.h"
#include "memalloc.h"

#define AGENT_NAME "PCE"

int add_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions);
int setup_oaa_connection(int argc, char *argv[]);

bool nonstrict = 0;

extern input_command_t * input_command;
extern int yyparse ();
//extern int yydebug;

int32_t imax(int32_t i, int32_t j) {return i<j ? j : i;}
int32_t imin(int32_t i, int32_t j) {return i>j ? j : i;}

void dump_sort_table (samp_table_t *table) {
  sort_table_t *sort_table = &(table->sort_table);
  const_table_t *const_table = &(table->const_table);
  // First get the sort length
  int i, j;
  int32_t slen = 4;
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

void dump_pred_tbl(pred_tbl_t * pred_tbl, sort_table_t * sort_table) {
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

void dump_pred_table (samp_table_t *table) {
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

void dump_atom_table (samp_table_t *table) {
  print_atoms(table);
}

void dump_clause_table (samp_table_t *table) {
  print_clauses(table);
}

void print_rule (samp_rule_t *rule, samp_table_t *table, int32_t indent) {
  pred_table_t *pred_table = &(table->pred_table);
  const_table_t *const_table = &(table->const_table);  
  int32_t j;
  for(j=0; j<rule->num_vars; j++) {
    j==0 ? printf(" (") : printf(", ");
    printf("%s", rule->vars[j]->name);
  }
  printf(")");
  for(j=0; j<rule->num_lits; j++) {
    printf("\n%*s  ", indent, "");
    j==0 ? printf("   ") : printf(" | ");
    rule_literal_t *lit = rule->literals[j];
    if (lit->neg) printf("-");
    samp_atom_t *atom = lit->atom;
    int32_t pred_idx = atom->pred;
    printf("%s", pred_name(pred_idx, pred_table));
    int32_t k;
    for(k=0; k<pred_arity(pred_idx, pred_table); k++) {
      k==0 ? printf("(") : printf(", ");
      int32_t argidx = atom->args[k];
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

void dump_rule_table (samp_table_t *samp_table) {
  rule_table_t *rule_table = &(samp_table->rule_table);
  uint32_t nrules = rule_table->num_rules;
  char d[10];
  uint32_t nwdth = sprintf(d, "%d", nrules);
  int32_t i;
  printf("Rule Table:\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("| %-*s | weight    | %-*s |\n", nwdth, "i", 61-nwdth, "Rule");
  printf("--------------------------------------------------------------------------------\n");
  for(i=0; i<nrules; i++) {
    samp_rule_t *rule = rule_table->samp_rules[i];
    printf("| %*"PRIu32" | % 9.3f | ", nwdth, i, rule_table->samp_rules[i]->weight);
    print_rule(rule, samp_table, nwdth+17);
    printf("\n");
  }
  printf("-------------------------------------------------------------------------------\n");
}

void test (samp_table_t *table) {
  rule_table_t *rule_table = &(table->rule_table);
  int32_t i;
  for (i = 0; i<rule_table->num_rules; i++) {
    all_ground_instances(rule_table->samp_rules[i], table);
  }
  dump_rule_table(table);
  dump_clause_table(table);
}

int main(int argc, char *argv[]){
  // Initialize OAA
  oaa_Init(argc, argv);

  samp_table_t table;
  init_samp_table(&table);
  sort_table_t *sort_table = &(table.sort_table);
  const_table_t *const_table = &(table.const_table);
  var_table_t *var_table = &(table.var_table);
  pred_table_t *pred_table = &(table.pred_table);
  //atom_table_t *atom_table = &(table.atom_table);
  //clause_table_t *clause_table = &(table.clause_table);
  /* Create the symbol table */
  stbl_t symbol_table_r;
  stbl_t *symbol_table = &symbol_table_r;
  init_stbl(symbol_table, 0);

  //yydebug = 1;
  do {
    printf("pce> ");
    // yyparse returns 0 and sets input_command if no syntax errors
    if (yyparse() == 0)
      switch (input_command->kind) {
      case PREDICATE: {
	int err = 0;
	input_preddecl_t * decl = (input_preddecl_t *) input_command->decl;
	char * pred = decl->atom->pred;
	// First the sorts - those we don't find, we add to the sort list
	// if nonstrict is set.
	int32_t i = 0;
	do {
	  char * sort = decl->atom->args[i];
	  if (sort_name_index(sort, sort_table) == -1) {
	    if (nonstrict)
	      add_sort(sort_table, sort);
	    else {
	      err = 1;
	      fprintf(stderr, "Sort %s has not been declared\n", sort);
	    }
	  }
	  i += 1;
	} while (decl->atom->args[i] != NULL);
	int32_t arity = i;

	if (err)
	  fprintf(stderr, "Predicate %s not added\n", pred);
	else {
	  printf("Adding predicate %s\n", pred);
	  add_pred(pred_table, pred, decl->witness, arity,
		   sort_table, decl->atom->args);
	}
	break;
      }
      case SORT: {
	input_sortdecl_t * decl
	  = (input_sortdecl_t *) input_command->decl; 
	printf("Adding sort %s\n", decl->name);
	add_sort(sort_table, decl->name);
	break;
      }
      case CONST: {
	input_constdecl_t * decl
	  = (input_constdecl_t *) input_command->decl;
	if (sort_name_index(decl->sort, sort_table) == -1) {
	    if (nonstrict)
	      add_sort(sort_table, decl->sort);
	    else {
	      fprintf(stderr, "Sort %s has not been declared\n", decl->sort);
	      break;
	    }
	}
	int32_t i;
	for (i = 0; i<decl->num_names; i++) {
	  // Need to see if name in var_table
	  if (var_index(decl->name[i], var_table) == -1) {
	    printf("Adding const %s\n", decl->name[i]);
	    add_const(const_table, decl->name[i], sort_table, decl->sort);
	  }
	  else
	    fprintf(stderr, "%s is a variable, cannot be redeclared a constant\n",
		    decl->name[i]);
	}
	break;
      }
      case VAR: {
	input_vardecl_t * decl
	  = (input_vardecl_t *) input_command->decl;
	if (sort_name_index(decl->sort, sort_table) == -1) {
	    if (nonstrict)
	      add_sort(sort_table, decl->sort);
	    else {
	      fprintf(stderr, "Sort %s has not been declared\n", decl->sort);
	      break;
	    }
	}
	// Need to see if name in const_table
	printf("Adding var %s\n", decl->name);
	add_var(var_table, decl->name, sort_table, decl->sort);
	break;
      }
      case ASSERT: {
	// Need to check that the predicate is a witness predicate,
	// then invoke add_atom or assert_atom.
	input_assertdecl_t * decl = (input_assertdecl_t *) input_command->decl;
	assert_atom(&table, decl->atom);
	break;
      }
      case ADD: {
	input_adddecl_t * decl = (input_adddecl_t *) input_command->decl;
	if (decl->clause->varlen == 0) {
	  // No variables - adding a clause
	  printf("Adding clause\n");
	  add_clause(&table,
		     decl->clause->literals,
		     decl->weight);
	}
	else {
	  // Have variables - adding a rule
	  printf("Adding rule with weight %lf\n", decl->weight);
	  add_rule(decl->clause, decl->weight, &table);
	}
	break;
      }
      case ASK:
	printf("Got ask\n");
	break;
      case DUMPTABLES: {
	printf("Dumping tables...\n");
	dump_sort_table(&table);
	dump_pred_table(&table);
	//dump_const_table(const_table, sort_table);
	//dump_var_table(var_table, sort_table);
	dump_atom_table(&table);
	dump_clause_table(&table);
	dump_rule_table(&table);
	break;
      }
      case TEST: {
	test(&table);
	break;
      }
      case HELP: {
	printf("\nInput grammar:\n");
	printf(" sort NAME ';'\n const NAME ':' NAME ';'\n");
	printf(" predicate ATOM (direct|indirect) ';'\n assert ATOM ';'\n");
	printf(" add CLAUSE WEIGHT ';'\n");
	printf(" ask CLAUSE ';'\n dumptables ';'\n help ';'\n quit ';'\n");
	printf("where:\n CLAUSE := ['(' NAME++',' ')'] LITERAL++'|'\n");
	printf(" LITERAL := ['~'] ATOM\n ATOM := NAME '(' ARG++',' ')'\n");
	printf(" ARG := NAME | NUM\n");
	printf(" NAME := chars except whitespace parens ':' ',' ';'\n");
	printf(" NUM := ['+'|'-'] simple floating point number\n");
	break;
      }
      case QUIT:
	exit(0);
      };
  } while (true);
  return 0;
}
