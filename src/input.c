/* Functions for creating input structures */

#include <inttypes.h>
#include <float.h>
#include "memalloc.h"
#include "utils.h"
#include "parser.h"
#include "yacc.tab.h"
#include "print.h"
#include "input.h"
#include "samplesat.h"

bool nonstrict = 0;

extern int yyparse ();
extern void free_parse_data();
extern int yydebug;

static file_stack_t parse_input_stack = {0, 0, NULL};

input_clause_buffer_t input_clause_buffer = {0, 0, NULL};
input_literal_buffer_t input_literal_buffer = {0, 0, NULL};
input_atom_buffer_t input_atom_buffer = {0, 0, NULL};

void input_clause_buffer_resize (){
  if (input_clause_buffer.capacity == 0){
    input_clause_buffer.clauses = (input_clause_t **)
      safe_malloc(INIT_INPUT_CLAUSE_BUFFER_SIZE * sizeof(input_clause_t *));
    input_clause_buffer.capacity = INIT_INPUT_CLAUSE_BUFFER_SIZE;
  } else {
    uint32_t size = input_clause_buffer.size;
    uint32_t capacity = input_clause_buffer.capacity;
    if (size+1 >= capacity){
      if (MAX_SIZE(sizeof(input_clause_t), 0) - capacity <= capacity/2){
	out_of_memory();
      }
      capacity += capacity/2;
      printf("Increasing clause buffer to %"PRIu32"\n", capacity);
      input_clause_buffer.clauses = (input_clause_t **)
	safe_realloc(input_clause_buffer.clauses,
		     capacity * sizeof(input_clause_t *));
      input_clause_buffer.capacity = capacity;
    }
  }
}

void input_literal_buffer_resize (){
  if (input_literal_buffer.capacity == 0){
    input_literal_buffer.literals = (input_literal_t *)
      safe_malloc(INIT_INPUT_LITERAL_BUFFER_SIZE * sizeof(input_literal_t));
    input_literal_buffer.capacity = INIT_INPUT_LITERAL_BUFFER_SIZE;
  } else {
    uint32_t size = input_literal_buffer.size;
    uint32_t capacity = input_literal_buffer.capacity;
    if (size+1 >= capacity){
      if (MAX_SIZE(sizeof(input_literal_t), 0) - capacity <= capacity/2){
	out_of_memory();
      }
      capacity += capacity/2;
      printf("Increasing literal buffer to %"PRIu32"\n", capacity);
      input_literal_buffer.literals = (input_literal_t *)
	safe_realloc(input_literal_buffer.literals,
		     capacity * sizeof(input_literal_t));
      input_literal_buffer.capacity = capacity;
    }
  }
}


void input_atom_buffer_resize (){
  if (input_atom_buffer.capacity == 0){
    input_atom_buffer.atoms = (input_atom_t *)
      safe_malloc(INIT_INPUT_ATOM_BUFFER_SIZE * sizeof(input_atom_t));
    input_atom_buffer.capacity = INIT_INPUT_ATOM_BUFFER_SIZE;
  } else {
    uint32_t size = input_atom_buffer.size;
    uint32_t capacity = input_atom_buffer.capacity;
    if (size+1 >= capacity){
      if (MAX_SIZE(sizeof(input_atom_t), 0) - capacity <= capacity/2){
	out_of_memory();
      }
      capacity += capacity/2;
      printf("Increasing atom buffer to %"PRIu32"\n", capacity);
      input_atom_buffer.atoms = (input_atom_t *)
	safe_realloc(input_atom_buffer.atoms, capacity * sizeof(input_atom_t));
      input_atom_buffer.capacity = capacity;
    }
  }
}

input_clause_t *new_input_clause () {
  input_clause_buffer_resize();
  return input_clause_buffer.clauses[input_clause_buffer.size++];
}

input_literal_t *new_input_literal () {
  input_literal_buffer_resize();
  return &input_literal_buffer.literals[input_literal_buffer.size++];
}

input_atom_t *new_input_atom () {
  input_atom_buffer_resize();
  return &input_atom_buffer.atoms[input_atom_buffer.size++];
}

extern void read_eval_print_loop(FILE *input, samp_table_t *table) {
  sort_table_t *sort_table = &(table->sort_table);
  const_table_t *const_table = &(table->const_table);
  var_table_t *var_table = &(table->var_table);
  pred_table_t *pred_table = &(table->pred_table);
  atom_table_t *atom_table = &(table->atom_table);

  file_stack_push(input, &parse_input_stack); // sets parse_input
  //yydebug = 1;
  do {
    printf("mcsat> ");
    fflush(stdout);
    // yyparse returns 0 and sets input_command if no syntax errors
    if (yyparse() == 0)
      switch (input_command.kind) {
      case PREDICATE: {
	int err = 0;
	input_preddecl_t decl = input_command.decl.preddecl;
	char * pred = decl.atom->pred;
	// First the sorts - those we don't find, we add to the sort list
	// if nonstrict is set.
	int32_t i = 0;
	do {
	  char * sort = decl.atom->args[i];
	  if (sort_name_index(sort, sort_table) == -1) {
	    if (nonstrict)
	      add_sort(sort_table, sort);
	    else {
	      err = 1;
	      fprintf(stderr, "Sort %s has not been declared\n", sort);
	    }
	  }
	  i += 1;
	} while (decl.atom->args[i] != NULL);
	int32_t arity = i;

	if (err)
	  fprintf(stderr, "Predicate %s not added\n", pred);
	else {
	  printf("Adding predicate %s\n", pred);
	  add_pred(pred_table, pred, decl.witness, arity,
		   sort_table, decl.atom->args);
	}
	break;
      }
      case SORT: {
	input_sortdecl_t decl = input_command.decl.sortdecl;
	printf("Adding sort %s\n", decl.name);
	add_sort(sort_table, decl.name);
	break;
      }
      case SUBSORT: {
	input_subsortdecl_t decl = input_command.decl.subsortdecl;
	printf("Adding subsort %s of %s information\n",
	       decl.subsort, decl.supersort);
	add_subsort(sort_table, decl.subsort, decl.supersort);
	break;
      }
      case CONST: {
	input_constdecl_t decl = input_command.decl.constdecl;
	if (sort_name_index(decl.sort, sort_table) == -1) {
	  if (nonstrict)
	    add_sort(sort_table, decl.sort);
	  else {
	    fprintf(stderr, "Sort %s has not been declared\n", decl.sort);
	    break;
	  }
	}
	int32_t i;
	for (i = 0; i<decl.num_names; i++) {
	  // Need to see if name in var_table
	  if (var_index(decl.name[i], var_table) == -1) {
	    cprintf(1,"Adding const %s\n", decl.name[i]);
	    if(add_const(decl.name[i], decl.sort, table) != -1){
	      int32_t cidx = const_index(decl.name[i], const_table);
	      // We don't invoke this in add_const, as this is eager.
	      // Last arg says this is not lazy.
	      create_new_const_rule_instances(cidx, table, false, 0);
	      create_new_const_query_instances(cidx, table, false, 0);
	    }
	  }
	  else
	    fprintf(stderr, "%s is a variable, cannot be redeclared a constant\n",
		    decl.name[i]);
	}
	break;
      }
      case VAR: {
	input_vardecl_t decl = input_command.decl.vardecl;
	if (sort_name_index(decl.sort, sort_table) == -1) {
	  if (nonstrict)
	    add_sort(sort_table, decl.sort);
	  else {
	    fprintf(stderr, "Sort %s has not been declared\n", decl.sort);
	    break;
	  }
	}
	int32_t i;
	for (i=0; i<decl.num_names; i++) {
	  // Need to see if name in const_table
	  printf("Adding var %s\n", decl.name[i]);
	  add_var(var_table, decl.name[i], sort_table, decl.sort);
	}
	break;
      }
      case ATOM: {
	// invoke add_atom
	input_atomdecl_t decl = input_command.decl.atomdecl;
	add_atom(table, decl.atom);
	break;
      }
      case ASSERT: {
	// Need to check that the predicate is a witness predicate,
	// then invoke assert_atom.
	input_assertdecl_t decl = input_command.decl.assertdecl;
	assert_atom(table, decl.atom);
	break;
      }
      case ADD: {
	input_adddecl_t decl = input_command.decl.adddecl;
	if (decl.clause->varlen == 0) {
	  // No variables - adding a clause
	  printf("Adding clause\n");
	  add_clause(table,
		     decl.clause->literals,
		     decl.weight);
	}
	else {
	  // Have variables - adding a rule
	  double wt = decl.weight;
	  if (wt == DBL_MAX) {
	    printf("Adding rule with MAX weight\n");
	  } else {
	    printf("Adding rule with weight %f\n", wt);
	  }
	  int32_t ruleidx = add_rule(decl.clause, decl.weight, table);
	  // Create instances here rather than add_rule, as this is eager
	  if (ruleidx != -1) {
	    all_rule_instances(ruleidx, table);
	  }
	}
	break;
      }
      case ASK: {
	input_askdecl_t decl = input_command.decl.askdecl;
	assert(decl.clause->litlen == 1);
	query_clause(decl.clause, decl.threshold, decl.all, table);
	break;
      }
      case MCSAT: {
	input_mcsatdecl_t decl = input_command.decl.mcsatdecl;
	printf("Calling MCSAT with parameters:\n");
	printf(" sa_probability = %f\n", decl.sa_probability);
	printf(" samp_temperature = %f\n", decl.samp_temperature);
	printf(" rvar_probability = %f\n", decl.rvar_probability);
	printf(" max_flips = %"PRId32"\n", decl.max_flips);
	printf(" max_extra_flips = %"PRId32"\n", decl.max_extra_flips);
	printf(" max_samples = %"PRId32"\n", decl.max_samples);
	mc_sat(table, decl.sa_probability, decl.samp_temperature,
	       decl.rvar_probability, decl.max_flips,
	       decl.max_extra_flips, decl.max_samples);
	printf("\n");
	break;
      }
      case RESET: {
	input_resetdecl_t decl = input_command.decl.resetdecl;
	switch (decl.kind) {
	case ALL: {
	  // Resets the sample tables
	  reset_sort_table(sort_table);
	  // Need to do more here - like free up space.
	  init_samp_table(table);
	  break;
	}
	case PROBABILITIES: {
	  // Simply resets the probabilities of the atom table to -1.0
	  printf("Resetting probabilities of atoms to -1.0\n");
	  int32_t i;
	  atom_table->num_samples = 0;
	  for (i=0; i<atom_table->num_vars; i++) {
	    atom_table->pmodel[i] = -1;
	  }
	  break;
	}
	}
	break;
      }
      case LOAD: {
	input_loaddecl_t decl = input_command.decl.loaddecl;
	load_mcsat_file(decl.file, table);
	break;
      }
      case DUMPTABLES: {
	printf("Dumping tables...\n");
	dump_sort_table(table);
	dump_pred_table(table);
	//dump_const_table(const_table, sort_table);
	//dump_var_table(var_table, sort_table);
	dump_atom_table(table);
	dump_clause_table(table);
	dump_rule_table(table);
	break;
      }
      case VERBOSITY: {
	input_verbositydecl_t decl = input_command.decl.verbositydecl;
	set_verbosity_level(decl.level);
	printf("Setting verbosity to %"PRId32"\n", decl.level);
	break;
      }
      case TEST: {
	test(table);
	break;
      }
      case HELP: {
	printf("\nInput grammar:\n");
	printf(" sort NAME ';'\n");
	printf(" subsort NAME NAME ';'\n");
	printf(" const NAME++',' ':' NAME ';'\n");
	printf(" predicate ATOM [direct|indirect] ';'\n");
	printf(" atom ATOM ';'\n assert ATOM ';'\n");
	printf(" add CLAUSE [NUM] ';'\n");
	printf(" ask CLAUSE ';'\n");
	printf(" mcsat NUM**','\n");
	printf(" reset probabilities\n");
	printf(" dumptables ';'\n verbosity NUM ';'\n help ';'\n quit ';'\n");
	printf("where:\n CLAUSE := ['(' NAME++',' ')'] LITERAL++'|'\n");
	printf(" LITERAL := ['~'] ATOM\n ATOM := NAME '(' ARG++',' ')'\n");
	printf(" ARG := NAME | NUM\n");
	printf(" NAME := chars except whitespace parens ':' ',' ';'\n");
	printf(" NUM := ['+'|'-'] simple floating point number\n");
	printf("predicates default to 'direct' (i.e., witness/observable)\n");
	printf("mcsat NUMs are optional, and represent, in order:\n");
	printf("  sa_probability - double\n");
	printf("  samp_temperature - double\n");
	printf("  rvar_probability - double\n");
	printf("  max_flips - int\n");
	printf("  max_extra_flips - int\n");
	printf("  max_samples - int\n");
	break;
      }
      case QUIT:
	printf("QUIT reached, exiting read_eval_print_loop\n");
	goto end;
	break;
      };
    free_parse_data();
  } while (true);
 end:
  file_stack_pop(&parse_input_stack);
  return;
}

extern bool load_mcsat_file(char *file, samp_table_t *table) {
  FILE *fp = fopen(file, "r");
  if (fp != NULL) {
    printf("Loading file %s\n", file);
    read_eval_print_loop(fp, table);
    fclose(fp);
    return true;
  } else {
    printf("File %s could not be opened\n", file);
    return false;
  }
}

// This is a placeholder - useful for debugging purposes.
// Will be invoked when 'TEST;' is given to the read_eval_print_loop
void test (samp_table_t *table) {
  rule_table_t *rule_table = &(table->rule_table);
  int32_t i;
  for (i = 0; i<rule_table->num_rules; i++) {
    all_rule_instances(i, table);
  }
  dump_rule_table(table);
  dump_clause_table(table);
}
