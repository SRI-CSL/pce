/* Functions for creating input structures */

#include <inttypes.h>
#include <float.h>
#include <string.h>
#include "memalloc.h"
#include "utils.h"
#include "parser.h"
#include "yacc.tab.h"
#include "print.h"
#include "input.h"
#include "cnf.h"
#include "samplesat.h"
#include "lazysamplesat.h"

extern int yyparse ();
extern void free_parse_data();
extern int yydebug;

static bool strict_consts = true;
static bool lazy = false;

input_clause_buffer_t input_clause_buffer = {0, 0, NULL};
input_literal_buffer_t input_literal_buffer = {0, 0, NULL};
input_atom_buffer_t input_atom_buffer = {0, 0, NULL};

bool strict_constants() {
  return strict_consts;
}

void set_strict_constants(bool val) {
  strict_consts = val;
}

bool lazy_mcsat() {
  return lazy;
}

void set_lazy_mcsat(bool val) {
  lazy = val;
}


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
      cprintf(2,"Increasing clause buffer to %"PRIu32"\n", capacity);
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
      cprintf(2, "Increasing literal buffer to %"PRIu32"\n", capacity);
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
      cprintf(2, "Increasing atom buffer to %"PRIu32"\n", capacity);
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

void free_atom(input_atom_t *atom) {
  int32_t i;

  i = 0;
  safe_free(atom->pred);
  while (atom->args[i] != NULL) {
    safe_free(atom->args[i]);
    i++;
  }
  safe_free(atom->args);
  safe_free(atom);
}

void free_literal(input_literal_t *lit) {
  free_atom(lit->atom);
  safe_free(lit);
}

void free_literals (input_literal_t **lit) {
  int32_t i;

  i = 0;
  while (lit[i] != NULL) {
    free_literal(lit[i]);
    i++;
  }
  safe_free(lit);
}

void free_fmla (input_fmla_t *fmla) {
  if (fmla->kind == ATOM) {
    free_atom(fmla->ufmla->atom);
  } else {
    input_comp_fmla_t *cfmla = fmla->ufmla->cfmla;
    input_fmla_t *arg1 = (input_fmla_t *) cfmla->arg1;
    free_fmla(arg1);
    if (cfmla->arg2 != NULL) {
      free_fmla((input_fmla_t *) cfmla->arg2);
    }
  }
  safe_free(fmla);
}

void free_var_entry(var_entry_t *var) {
  safe_free(var->name);
  safe_free(var);
}

void free_var_entries(var_entry_t **vars) {
  int32_t i;

  if (vars != NULL) {
    for (i = 0; vars[i] != NULL; i++) {
      free_var_entry(vars[i]);
    }
    safe_free(vars);
  }
}

void free_formula(input_formula_t *formula) {
  free_var_entries(formula->vars);
  free_fmla(formula->fmla);
  safe_free(formula);
}

void free_clause(input_clause_t *clause) {
  free_strings(clause->variables);
  free_literals(clause->literals);
  safe_free(clause);
}

void show_help(int32_t topic) {
  switch (topic) {
  case ALL: {
    printf("\n\
Input grammar:\n\
 sort NAME ';'\n\
 subsort NAME NAME ';'\n\
 const NAME++',' ':' NAME ';'\n\
 predicate ATOM [direct|indirect] ';'\n\
 atom ATOM ';'\n assert ATOM ';'\n\
 add CLAUSE [NUM] ';'\n\
 ask CLAUSE NUM ';'\n\
 mcsat NUM**','\n\
 reset probabilities\n\
 dumptables ';'\n verbosity NUM ';'\n help ';'\n quit ';'\n\
where:\n CLAUSE := ['(' NAME++',' ')'] LITERAL++'|'\n\
 LITERAL := ['~'] ATOM\n ATOM := NAME '(' ARG++',' ')'\n\
 ARG := NAME | NUM\n\
 NAME := chars except whitespace parens ':' ',' ';'\n\
 NUM := ['+'|'-'] simple floating point number\n\
predicates default to 'direct' (i.e., witness/observable)\n\
mcsat NUMs are optional, and represent, in order:\n\
  sa_probability - double\n\
  samp_temperature - double\n\
  rvar_probability - double\n\
  max_flips - int\n\
  max_extra_flips - int\n\
  max_samples - int\n\
");
    break;
  }
  case SORT: {
    printf("\n\
sort NAME;\n\
  declares NAME as a new sort\n\
");
    break;
  };
  case SUBSORT: {
    printf("\n\
subsort NAME1 NAME2;\n\
  declares NAME1 as a subsort of NAME2\n\
NAME1 and NAME2 may or may not be existing sorts.\n\
The effect of this is to allow more refined predicates,\n\
leading to a smaller sample space.\n\
");
    break;
  }
  case PREDICATE: {
    printf("\n\
predicate PRED(SORT1, ..., SORTn) [WITNESS];\n\
  Declares a predicate PRED, its signature, and whether it is a witness.\n\
WITNESS may be 'DIRECT' or 'INDIRECT' - if omitted, defaults to 'DIRECT'.\n\
A direct predicate is one that is observable.  The primary impact of this\n\
is that direct predicates satisfy the closed world assumption, while\n\
indirect predicates do not.\n\
");
    break;
  }
  case CONST: {
    printf("\n\
const NAME1, NAME2, ... : SORT;\n\
  Declares constants to be of the given sort.\n\
");
    break;
  }
  case ADD: {
    printf("\n\
add FORMULA WEIGHT [SOURCE];\n\
  Clausifies the FORMULA, and adds each clause to the clause or rules table,\n\
with the given WEIGHT (a floating point number) and associated with the SOURCE\n\
(an arbitrary NAME).\n\
Ground clauses are added to the clause table, and clauses with variables are\n\
added to the rule table.  For a ground clause, the formula is an atom\n\
(i.e., predicate applied to constants), or a boolean expression\n\
built using NOT, AND, OR, IMPLIES (or =>), IFF, and parentheses.\n\
A rule is similar, but involves variables, which are in square brackets\n\
before the boolean formula.\n\
For example:\n\
  add p(c1) and (p(c2) or p(c3)) 3.3;\n\
  add [x, y, z] r(x, y) and r(y, z) implies r(x, z) 15 user;\n\
");
    break;
  }
  case ADD_CLAUSE: {
    printf("\n\
add_clause [VARS] CLAUSE WEIGHT [SOURCE];\n\
  Adds the clause to the clause or rules table with the given WEIGHT\n\
(a floating point number) and associated with the SOURCE (a NAME).\n\
Ground clauses are added to the clause table, and clauses with variables are\n\
added to the rule table.  Ground clauses are added to the clause table\n\
For example:\n\
  add_clause ~p(c1) | ~p(c2) | p(c3) 3.3;\n\
  add [x, y, z] r(x, y) and r(y, z) implies r(x, z) 15 user;\n\
");
    break;
  }
  case ATOM: {
    printf("\n\
atom ATOM;\n\
  Simply adds an atom to the atom table, without any associated weight or truth value.\n\
Probably not needed.\n\
");
    break;
  }
    //case VAR:
  case ASK: {
    printf("\n\
ask FORMULA [NUMBER_OF_SAMPLES [THRESHOLD]];\n\
  Queries for the probability of instances of the given formula.\n\
Does sampling over the given NUMBER_OF_SAMPLES (default %d), and prints\n\
the instance with the highest probability if THRESHOLD is not provided.\n\
If THRESHOLD is provided, all instances whose probabilities are >= THRESHOLD\n\
are printed, in order.\n\
For example:\n\
  ask [x] father(Bob, x) and mother(Alice, x)\n\
    may produce 'father(Bob, Carl) and mother(Alice, Carl): .253\n\
  ask [x] father(Bob, x) and mother(Alice, x) 200 .2\n\
    may produce 'father(Bob, Carl) and mother(Alice, Carl): .253\n\
                 father(Bob, Cathy) and mother(Alice, Cathy): .215'\n\
", DEFAULT_MAX_SAMPLES);
    break;
  }

//  case ASK_CLAUSE:
  case MCSAT:
  case RESET:
  case DUMPTABLE:
  case LOAD:
  case VERBOSITY:
  case HELP:
    printf("\n\
No help available yet\n\
");
    break;
  }
}
	
extern void read_eval_print_loop(char *file, samp_table_t *table) {
  sort_table_t *sort_table = &(table->sort_table);
  const_table_t *const_table = &(table->const_table);
  var_table_t *var_table = &(table->var_table);
  pred_table_t *pred_table = &(table->pred_table);
  atom_table_t *atom_table = &(table->atom_table);
  input_stack_push(file); // sets parse_file and parse_input
  //yydebug = 1;
  yylloc.first_line = 1;
  yylloc.first_column = 0;
  yylloc.last_line = 1;
  yylloc.last_column = 0;
  do {
    printf("mcsat> ");
    fflush(stdout);
    // yyparse returns 0 and sets input_command if no syntax errors
    if (yyparse() == 0)
      switch (input_command.kind) {
      case PREDICATE: {
	int err = 0;
	input_pred_decl_t decl = input_command.decl.pred_decl;
	char * pred = decl.atom->pred;
	// First the sorts - those we don't find, we add to the sort list
	// if nonstrict is set.
	int32_t i = 0;
	do {
	  char * sort = decl.atom->args[i];
	  if (sort_name_index(sort, sort_table) == -1) {
	    if (!strict_constants())
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
	  cprintf(1, "Adding predicate %s\n", pred);
	  add_pred(pred_table, pred, decl.witness, arity,
		   sort_table, decl.atom->args);
	}
	break;
      }
      case SORT: {
	input_sort_decl_t decl = input_command.decl.sort_decl;
	cprintf(1, "Adding sort %s\n", decl.name);
	add_sort(sort_table, decl.name);
	break;
      }
      case SUBSORT: {
	input_subsort_decl_t decl = input_command.decl.subsort_decl;
	cprintf(1, "Adding subsort %s of %s information\n",
	       decl.subsort, decl.supersort);
	add_subsort(sort_table, decl.subsort, decl.supersort);
	break;
      }
      case CONST: {
	input_const_decl_t decl = input_command.decl.const_decl;
	if (sort_name_index(decl.sort, sort_table) == -1) {
	  if (!strict_constants())
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
	    cprintf(1, "Adding const %s\n", decl.name[i]);
	    if (add_const(decl.name[i], decl.sort, table) != -1) {
	      int32_t cidx = const_index(decl.name[i], const_table);
	      // We don't invoke this in add_const, as this is eager.
	      // Last arg says this is not lazy.
	      create_new_const_rule_instances(cidx, table, 0);
	      create_new_const_query_instances(cidx, table, 0);
	    }
	  }
	  else
	    fprintf(stderr, "%s is a variable, cannot be redeclared a constant\n",
		    decl.name[i]);
	}
	break;
      }
      case VAR: {
	input_var_decl_t decl = input_command.decl.var_decl;
	if (sort_name_index(decl.sort, sort_table) == -1) {
	  if (!strict_constants())
	    add_sort(sort_table, decl.sort);
	  else {
	    fprintf(stderr, "Sort %s has not been declared\n", decl.sort);
	    break;
	  }
	}
	int32_t i;
	for (i=0; i<decl.num_names; i++) {
	  // Need to see if name in const_table
	  cprintf(1, "Adding var %s\n", decl.name[i]);
	  add_var(var_table, decl.name[i], sort_table, decl.sort);
	}
	break;
      }
      case ATOM: {
	// invoke add_atom
	input_atom_decl_t decl = input_command.decl.atom_decl;
	add_atom(table, decl.atom);
	break;
      }
      case ASSERT: {
	// Need to check that the predicate is a witness predicate,
	// then invoke assert_atom.
	input_assert_decl_t decl = input_command.decl.assert_decl;
	assert_atom(table, decl.atom);
	break;
      }
      case ADD: {
	input_add_fdecl_t decl = input_command.decl.add_fdecl;
	cprintf(1, "Clausifying and adding formula\n");
	add_cnf(decl.formula, decl.weight);
	break;
      }
      case ADD_CLAUSE: {
	input_add_decl_t decl = input_command.decl.add_decl;
	if (decl.clause->varlen == 0) {
	  // No variables - adding a clause
	  cprintf(1, "Adding clause\n");
	  add_clause(table,
		     decl.clause->literals,
		     decl.weight);
	}
	else {
	  // Have variables - adding a rule
	  double wt = decl.weight;
	  if (wt == DBL_MAX) {
	    cprintf(1, "Adding rule with MAX weight\n");
	  } else {
	    cprintf(1, "Adding rule with weight %f\n", wt);
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
	input_ask_fdecl_t decl = input_command.decl.ask_fdecl;
	cprintf(1, "Clausifying formula\n");
	ask_cnf(decl.formula, decl.num_samples, decl.threshold);
	break;
      }
//       case ASK_CLAUSE: {
// 	input_ask_decl_t decl = input_command.decl.ask_decl;
// 	assert(decl.clause->litlen == 1);
// 	ask_clause(decl.clause, decl.threshold, decl.all, decl.num_samples);
// 	break;
//       }
      case MCSAT: {
	input_mcsat_decl_t decl = input_command.decl.mcsat_decl;
	printf("Calling %sMC_SAT with parameters:\n",
	       lazy_mcsat() ? "LAZY_" : "");
	printf(" sa_probability = %f\n", decl.sa_probability);
	printf(" samp_temperature = %f\n", decl.samp_temperature);
	printf(" rvar_probability = %f\n", decl.rvar_probability);
	printf(" max_flips = %"PRId32"\n", decl.max_flips);
	printf(" max_extra_flips = %"PRId32"\n", decl.max_extra_flips);
	printf(" max_samples = %"PRId32"\n", decl.max_samples);
	if (lazy_mcsat()) {
	  lazy_mc_sat(table, decl.sa_probability, decl.samp_temperature,
		      decl.rvar_probability, decl.max_flips,
		      decl.max_extra_flips, decl.max_samples);
	} else {
	  mc_sat(table, decl.sa_probability, decl.samp_temperature,
		 decl.rvar_probability, decl.max_flips,
		 decl.max_extra_flips, decl.max_samples);
	}
	printf("\n");
	break;
      }
      case RESET: {
	input_reset_decl_t decl = input_command.decl.reset_decl;
	switch (decl.kind) {
	case RESETALL: {
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
	input_load_decl_t decl = input_command.decl.load_decl;
	load_mcsat_file(decl.file, table);
	break;
      }
      case DUMPTABLE: {
	input_dumptable_decl_t decl = input_command.decl.dumptable_decl;
	switch (decl.table) {
	case ALL: {
	  cprintf(1, "Dumping tables...\n");
	  dump_sort_table(table);
	  dump_pred_table(table);
	  //dump_const_table(const_table, sort_table);
	  //dump_var_table(var_table, sort_table);
	  dump_atom_table(table);
	  dump_clause_table(table);
	  dump_rule_table(table);
	  break;
	}
	case SORT: {
	  dump_sort_table(table);
	  break;
	}
	case PREDICATE: {
	  dump_pred_table(table);
	  break;
	}
	case ATOM: {
	  dump_atom_table(table);
	  break;
	}
	case CLAUSE: {
	  dump_clause_table(table);
	  break;
	}
	case RULE: {
	  dump_rule_table(table);
	  break;
	}
	}
	break;
      }
      case VERBOSITY: {
	input_verbosity_decl_t decl = input_command.decl.verbosity_decl;
	set_verbosity_level(decl.level);
	cprintf(1, "Setting verbosity to %"PRId32"\n", decl.level);
	break;
      }
      case TEST: {
	test(table);
	break;
      }
      case HELP: {
	input_help_decl_t decl = input_command.decl.help_decl;
	show_help(decl.command);
	break;
      }
      case QUIT:
	cprintf(1, "QUIT reached, exiting read_eval_print_loop\n");
	input_command.kind = 0;
	goto end;
	break;
      };
    free_parse_data();
    //yylloc.first_line += yylloc.last_line;
    //yylloc.first_column += yyloc.last_column;
  } while (true);
 end:
  input_stack_pop();
  return;
}

extern void load_mcsat_file(char *file, samp_table_t *table) {
  read_eval_print_loop(file, table);
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
