/* Functions for creating input structures */

#include <ctype.h>
#include <inttypes.h>
#include <float.h>
#include <string.h>
#include "memalloc.h"
#include "utils.h"
#include "parser.h"
#include "print.h"
#include "SFMT.h"
#include "input.h"
#include "cnf.h"
#include "samplesat.h"
#include "lazysamplesat.h"

extern int yyparse ();
extern void free_parse_data();
extern int yydebug;

// MCSAT parameters
#define DEFAULT_MAX_SAMPLES 100
#define DEFAULT_SA_PROBABILITY .5
#define DEFAULT_SAMP_TEMPERATURE 5.0
#define DEFAULT_RVAR_PROBABILITY .05
#define DEFAULT_MAX_FLIPS 100
#define DEFAULT_MAX_EXTRA_FLIPS 5
#define DEFAULT_MCSAT_TIMEOUT 0

static int32_t max_samples = DEFAULT_MAX_SAMPLES;
static double sa_probability = DEFAULT_SA_PROBABILITY;
static double samp_temperature = DEFAULT_SAMP_TEMPERATURE;
static double rvar_probability = DEFAULT_RVAR_PROBABILITY;
static int32_t max_flips = DEFAULT_MAX_FLIPS;
static int32_t max_extra_flips = DEFAULT_MAX_EXTRA_FLIPS;
static int32_t mcsat_timeout = DEFAULT_MCSAT_TIMEOUT;

static bool strict_consts = true;
static bool lazy = false;

static bool print_exp_p = false;

void set_print_exp_p(bool flag) {
  print_exp_p = flag;
}

bool get_print_exp_p() {
  return print_exp_p;
}

uint32_t pce_rand_seed = 12345;

void rand_reset() {
  init_gen_rand(pce_rand_seed);
}

void set_pce_rand_seed(uint32_t seed) {
  pce_rand_seed = seed;
  rand_reset();
}

input_clause_buffer_t input_clause_buffer = {0, 0, NULL};
input_literal_buffer_t input_literal_buffer = {0, 0, NULL};
input_atom_buffer_t input_atom_buffer = {0, 0, NULL};

int32_t get_max_samples() {
  return max_samples;
}
double get_sa_probability() {
  return sa_probability;
}
double get_samp_temperature() {
  return samp_temperature;
}
double get_rvar_probability() {
  return rvar_probability;
}
int32_t get_max_flips() {
  return max_flips;
}
int32_t get_max_extra_flips() {
  return max_extra_flips;
}
int32_t get_mcsat_timeout() {
  return mcsat_timeout;
}

void set_max_samples(int32_t m) {
  max_samples = m;
}
void set_sa_probability(double d) {
  sa_probability = d;
}
void set_samp_temperature(double d) {
  samp_temperature = d;
}
void set_rvar_probability(double d) {
  rvar_probability = d;
}
void set_max_flips(int32_t m) {
  max_flips = m;
}
void set_max_extra_flips(int32_t m) {
  max_extra_flips = m;
}
void set_mcsat_timeout(int32_t m) {
  mcsat_timeout = m;
}

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
  if (atom->builtinop == 0) {
    safe_free(atom->pred);
  }
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
  input_comp_fmla_t *cfmla;
  
  if (fmla->atomic) {
    free_atom(fmla->ufmla->atom);
  } else {
    cfmla = fmla->ufmla->cfmla;
    free_fmla((input_fmla_t *) cfmla->arg1);
    if (cfmla->arg2 != NULL) {
      free_fmla((input_fmla_t *) cfmla->arg2);
    }
    safe_free(cfmla);
  }
  safe_free(fmla->ufmla);
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

void free_samp_atom(samp_atom_t *atom) {
  safe_free(atom);
}

void free_rule_atom(rule_atom_t *atom) {
  safe_free(atom->args);
  safe_free(atom);
}

void free_rule_literal(rule_literal_t *lit) {
  free_rule_atom(lit->atom);
  safe_free(lit);
}

void free_rule_literals(rule_literal_t **lit) {
  int32_t i;

  for (i = 0; lit[i] != NULL; i++) {
    free_rule_literal(lit[i]);
  }
  safe_free(lit);
}

void free_samp_query(samp_query_t *query) {
  int32_t i;
  
  for (i = 0; i < query->num_vars; i++) {
    free_var_entry(query->vars[i]);
  }
  for (i = 0; i < query->num_clauses; i++) {
    free_rule_literals(query->literals[i]);
  }
  safe_free(query->literals);
  safe_free(query);
}

void free_samp_query_instance(samp_query_instance_t *qinst) {
  int32_t i;
  
  safe_free(qinst->subst);
  for (i = 0; qinst->lit[i] != NULL; i++) {
    safe_free(qinst->lit[i]);
  }
  safe_free(qinst->lit);
  safe_free(qinst);
}

void show_help(int32_t topic) {
  switch (topic) {
  case ALL: {
    output("\n\
Type help followed by a command for details, e.g., 'help mcsat_params;'\n\n\
Input grammar:\n\
 sort NAME [sortspec] ';'\n\
 subsort NAME NAME ';'\n\
 predicate ATOM ['observable' | 'hidden'] ';'\n\
 const NAME++',' ':' NAME ';'\n\
 assert ATOM ';'\n\
 add FORMULA [WEIGHT [SOURCE]] ';'\n\
 add_clause CLAUSE [NUM [NAME]] ';'\n\
 ask FORMULA [THRESHOLD [NUMRESULTS]] ';'\n\
 mcsat ';'\n\
 mcsat_params [NUM]**',' ';'\n\
 reset ['all' | 'probabilities'] ';'\n\
 retract NAME ';'\n\
 dumptable ['all' | 'sort' | 'predicate' | 'atom' | 'clause' | 'rule' | 'summary'] ';'\n\
 load STRING ';'\n\
 verbosity NUM ';'\n help ';'\n quit ';'\n\
 help [all | sort | subsort | predicate | const | atom | assert |\n\
       add | add_clause | ask | mcsat | mcsat_params | reset | retract |\n\
       dumptable | load | verbosity | help] ';' \n\n\
where:\n\
 sortspec :=  '=' '[' INT '..' INT ']'\n\
           |  '=' INTEGER\n\
predicates default to 'observable' (i.e., direct/witness)\n\n\
 FORMULA := FMLA\n\
          | '[' VAR+',' ']' FMLA\n\
 FMLA := ATOM\n\
       | FMLA 'iff' FMLA\n\
       | FMLA ('implies'|'=>') FMLA\n\
       | FMLA 'or' FMLA\n\
       | FMLA 'xor' FMLA\n\
       | FMLA 'and' FMLA\n\
       | ('not'|'~') FMLA\n\
       | '(' FMLA ')'\n\
 CLAUSE := LITERAL+'|'\n\
         | '(' VAR+',' ')' LITERAL+'|'\n\
 LITERAL := ATOM\n\
          | ('not'|'~') ATOM\n\
 ATOM := PREDICATE '(' ARGUMENT+',' ')'\n\
       | ARG BOP ARG\n\
       | PREOP '(' ARGUMENT+',' ')'\n\
 ARGUMENT := NAME | NUM\n\
 VAR := NAME\n\
 PREDICATE := NAME\n\
 BOP := '=' | NEQ | '<' | '<=' | '>' | '>='\n\
 NEQ := '/=' | '~='\n\
 PREOP := '+' | '-' | '*' | '/' | '%'\n\
        +(x,y,z) means x + y = z, similarly for -, *, /, %\n\
        see standard C operators for definitions.\n\
 STRING := Anything between double quotes, e.g., \"a\"\n\
           or between single quotes, e.g., 'a'\n\
 NAME := ALPHA (^ DELIM | CNTRL | COMMENT )*\n\
 NUM := ( DIGIT | '.' | '+' | '-' ) ( DIGIT | '.' )*\n\
        must contain at least one digit, at most one '.'\n\
 INT := ['+'|'-'] DIGIT+\n\
 ALPHA := 'a' | ... | 'z' | 'A' | ... | 'Z'\n\
 DIGIT := '0' | ... | '9'\n\
 DELIM := ' ' | '(' | ')' | '[' | ']' | '|' | ',' | ';' | ':'\n\
 CNTRL := non-printing (control) characters\n\
 COMMENT := '#' to end of line\n\n\
");
    break;
  }
  case SORT: {
    output("\n\
sort NAME [ = '[' INT .. INT ']' ];\n\
  declares NAME as a new sort\n\
");
    break;
  };
  case SUBSORT: {
    output("\n\
subsort NAME1 NAME2;\n\
  declares NAME1 as a subsort of NAME2\n\
NAME1 and NAME2 may or may not be existing sorts.\n\
The effect of this is to allow more refined predicates,\n\
leading to a smaller sample space.\n\
");
    break;
  }
  case PREDICATE: {
    output("\n\
predicate PRED(SORT1, ..., SORTn) [WITNESS];\n\
  Declares a predicate PRED, its signature, and whether it is a witness.\n\
WITNESS may be 'DIRECT' or 'INDIRECT' - if omitted, defaults to 'DIRECT'.\n\
A direct predicate is one that is observable.  The primary impact of this\n\
is that direct predicates satisfy the closed world assumption, while\n\
indirect predicates do not.\n\
");
    break;
  }
  case CONSTD: {
    output("\n\
const NAME1, NAME2, ... : SORT;\n\
  Declares constants to be of the given sort.\n\
");
    break;
  }
  case ADD: {
    output("\n\
add FORMULA [WEIGHT [SOURCE]];\n\
  Clausifies the FORMULA, and adds each clause to the clause or rules table,\n\
with the given WEIGHT (a floating point number) and associated with the SOURCE\n\
(an arbitrary NAME).\n\
Ground clauses are added to the clause table, and clauses with variables are\n\
added to the rule table.  For a ground clause, the formula is an atom\n\
(i.e., predicate applied to constants), or a boolean expression\n\
built using NOT, AND, OR, IMPLIES (or =>), XOR, IFF, and parentheses.\n\
A rule is similar, but involves variables, which are in square brackets\n\
before the boolean formula.\n\
For example:\n\
  add p(c1) and (p(c2) or p(c3)) 3.3;\n\
  add [x, y, z] r(x, y) and r(y, z) implies r(x, z) 15 user;\n\
");
    break;
  }
  case ADD_CLAUSE: {
    output("\n\
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
  case ATOMD: {
    output("\n\
atom ATOM;\n\
  Simply adds an atom to the atom table, without any associated weight or truth value.\n\
Probably not needed.\n\
");
    break;
  }
    //case VAR:
  case ASK: {
    output("\n\
ask FORMULA [THRESHOLD [NUMRESULTS];\n\
  Queries for the probability of instances of the given formula.\n\
Does sampling over the number of samples (set by MCSAT_PARAMS), and prints\n\
only instances of probability >= THRESHOLD (default 0.0), up to NUMRESULTS\n\
 (default all), in probability order.\n\
For example:\n\
  ask [x] father(Bob, x) and mother(Alice, x)\n\
    may produce\n\
      [x <- Carl] 0.253: father(Bob, Carl) and mother(Alice, Carl)\n\
      [x <- Darla] 0.117: father(Bob, Darla) and mother(Alice, Darla)\n\
      [x <- Earl] 0.019: father(Bob, Earl) and mother(Alice, Earl)\n\
  ask [x] father(Bob, x) and mother(Alice, x) .2 2\n\
    may produce\n\
      [x <- Carl] 0.253: father(Bob, Carl) and mother(Alice, Carl)\n\
");
    break;
  }

//  case ASK_CLAUSE:
  case MCSAT_PARAMS: {
    output("\n\
mcsat_params [NUM++','];\n\
  Sets the MCSAT parameters.  With no arguments, displays the current values.\n\
mcsat NUMs are optional, and represent, in order:\n\
  max_samples (int): Number of samples to take\n\
  sa_probability (double): Prob of taking simulated annealing step\n\
  samp_temperature (double): Simulated annealing temperature\n\
  rvar_probability (double): Prob of flipping a random variable\n\
                             in non-simulated annealing step\n\
  max_flips (int): Max number of variable flips to find a model\n\
  max_extra_flips (int): Max number of extra flips to try\n\
  timeout (int): Number of seconds to timeout - 0 means no timeout\n\
 The sampling runs until either max_samples or the timeout, whichever comes first.\n\
Example:\n\
  mcsat_params ,.8,,1000;\n\
    Sets sa_probability to .8, max_flips to 1000, and keeps other parameter values.\n\
");
    break;
  }
  case MCSAT: {
    output("\n\
mcsat;\n\
  Runs the %s mcsat process\n\
Restart MCSAT with '--lazy=%s' for the %slazy version\n\
", lazy_mcsat() ? "lazy" : "",
	   lazy_mcsat() ? "false" : "true",
	   lazy_mcsat() ? "non" : "");
    break;
  }
  case RESET: {
    output("\n\
reset [all | probabilities];\n\
  All: resets the internal tables and the random number seed\n\
  Probabilities: just resets the count used to generate probabilities\n\
");
    break;
  }
  case RETRACT: {
    output("\n\
retract [all | source];\n\
  Removes assertions and clauses provided by source\n\
");
    break;
  }
  case DUMPTABLE: {
    output("\n\
dumptable [all | sort | predicate | atom | clause | rule | summary];\n\
  dumps the corresponding table(s), or a summary of them\n\
");
    break;
  }    
  case LOAD: {
    output("\n\
load \"file\";\n\
  loads the file (files may themselves load files)\n\
");
    break;
  }
  case VERBOSITY: {
    output("\n\
verbosity num;\n\
  Sets the verbosity level, mostly useful for debugging\n\
");
    break;
  } 
  case HELP:
    output("\n\
help [command];\n\
  Provides help for the specified command\n\
");
    break;
  }
}

extern int32_t add_predicate(char *pred, char **sort, bool directp, samp_table_t *table) {
  sort_table_t *sort_table = &table->sort_table;
  pred_table_t *pred_table = &table->pred_table;
  
  int err, i, arity;
  err = 0;
  // First the sorts - those we don't find, we add to the sort list
  // if nonstrict is set.
  i = 0;
  do {
    if (sort_name_index(sort[i], sort_table) == -1) {
      if (!strict_constants())
	add_sort(sort_table, sort[i]);
      else {
	err = 1;
	mcsat_err("Sort %s has not been declared\n", sort[i]);
      }
    }
    i += 1;
  } while (sort[i] != NULL);
  arity = i;
  
  if (err) {
    mcsat_err("Predicate %s not added\n", pred);
  } else {
    cprintf(2, "Adding predicate %s\n", pred);
    err = add_pred(pred_table, pred, directp, arity,
		   sort_table, sort);
    if (err == 0 && !lazy_mcsat()) {
      // Need to create all atom instances
      all_pred_instances(pred, table);
    }
  }
  return 0;
}

extern int32_t str2int(char *cnst) {
  int32_t i;
  long int lcnst;

  for (i = (cnst[0] == '+' || cnst[0] == '-') ? 1 : 0; cnst[i] != '\0'; i++) {
    if (! isdigit(cnst[i])) {
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
  

extern void add_int_const(int32_t icnst, sort_entry_t *entry, sort_table_t *sort_table) {
  sort_entry_t *supentry;
  int32_t i, j;
  bool foundit;

  assert(entry->constants == NULL && entry->ints != NULL);
  for (i = 0; i < entry->cardinality; i++) {
    if (entry->ints[i] == icnst) {
      mcsat_warn("Constant declaration ignored: %"PRId32" is already in %s\n", icnst, entry->name);
      return;
    }
  }
  if (entry->size == entry->cardinality) {
    if (MAXSIZE(sizeof(long int), 0) - entry->size < entry->size/2) {
      out_of_memory();
    }
    entry->size += entry->size/2;
    entry->ints = (int32_t *)
      safe_realloc(entry->ints, entry->size * sizeof(int32_t));
  }
  entry->ints[entry->cardinality++] = icnst;
  // Added it to this sort, now do its supersorts
  if (entry->supersorts != NULL) {
    for (i = 0; entry->supersorts[i] != -1; i++) {
      supentry = &sort_table->entries[entry->supersorts[i]];
      foundit = false;
      for (j = 0; j < supentry->cardinality; j++) {
	if (supentry->ints[j] == icnst) {
	  foundit = true;
	  break;
	}
      }
      if (!foundit) {
	add_int_const(icnst, supentry, sort_table);
      }
    }
  }
}


extern int32_t add_constant(char *cnst, char *sort, samp_table_t *table) {
  sort_table_t *sort_table = &table->sort_table;
  const_table_t *const_table = &table->const_table;
  var_table_t *var_table = &table->var_table;
  int32_t sort_index, icnst;
  sort_entry_t *sort_entry;

  sort_index = sort_name_index(sort, sort_table);
  if (sort_index == -1) {
    if (!strict_constants())
      add_sort(sort_table, sort);
    else {
      mcsat_err("Sort %s has not been declared\n", sort);
      return -1;
    }
  }
  sort_entry = &sort_table->entries[sort_index];
  if (sort_entry->constants == NULL) {
    // Check that the const is an integer in range
    icnst = str2int(cnst);
    if (sort_entry->ints == NULL) {
      if (icnst >= sort_entry->lower_bound && icnst <= sort_entry->upper_bound) {
	mcsat_warn("Constant declaration ignored: %s is already in [%"PRId32"..%"PRId32"]\n",
		   cnst, sort_entry->lower_bound, sort_entry->upper_bound);
      } else {
	mcsat_err("Illegal constant %s: sort %s only allows integers in [%"PRId32"..%"PRId32"]\n",
		  cnst, sort, sort_entry->lower_bound, sort_entry->upper_bound);
      }
    } else {
      // Sparse integers
      add_int_const(icnst, sort_entry, sort_table);
      create_new_const_atoms(icnst, sort_index, table);
      create_new_const_rule_instances(icnst, sort_index, table, 0);
      create_new_const_query_instances(icnst, sort_index, table, 0);
    }
  } else {
    // Need to see if name in var_table
    if (var_index(cnst, var_table) == -1) {
      cprintf(2, "Adding const %s\n", cnst);
      // add_const returns -1 for error, 1 for already exists, 0 for new
      if (add_const(cnst, sort, table) == 0) {
	int32_t cidx = const_index(cnst, const_table);
	// We don't invoke this in add_const, as this is eager.
	create_new_const_atoms(cidx, sort_index, table);
	create_new_const_rule_instances(cidx, sort_index, table, 0);
	create_new_const_query_instances(cidx, sort_index, table, 0);
      }
    } else {
      mcsat_err("%s is a variable, cannot be redeclared a constant\n", cnst);
    }
  }
  return 0;
}

void print_ask_results (input_formula_t *fmla, samp_table_t *table) {
  samp_query_instance_t *qinst;
  int32_t i, j, *qsubst;
  char *cname;
  
  printf("\n%d results:\n", ask_buffer.size);
  for (i = 0; i < ask_buffer.size; i++) {
    qinst = (samp_query_instance_t *) ask_buffer.data[i];
    qsubst = qinst->subst;
    printf("[");
    if (fmla->vars != NULL) {
      for (j = 0; fmla->vars[j] != NULL; j++) {
	if (j != 0) printf(", ");
	if (qinst->constp[j]) {
	  cname = const_name(qsubst[j], &table->const_table);
	  printf("%s <- %s", fmla->vars[j]->name, cname);
	} else {
	  printf("%s <- %"PRId32"", fmla->vars[j]->name, qsubst[j]);
	}
      }
    }
    printf("]");
    if (get_print_exp_p()) {
      output("% .4e:", query_probability((samp_query_instance_t *) ask_buffer.data[i], table));
    } else {
      output("% 11.4f:", query_probability((samp_query_instance_t *) ask_buffer.data[i], table));
    }
    print_query_instance(qinst, table, 0, false);
    printf("\n");
    fflush(stdout);
  }
}

extern void dumptable(int32_t tbl, samp_table_t *table) {
  switch (tbl) {
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
  case ATOMD: {
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
  case SUMMARY: {
    summarize_sort_table(table);
    summarize_pred_table(table);
    //summarize_const_table(const_table, sort_table);
    //summarize_var_table(var_table, sort_table);
    summarize_atom_table(table);
    summarize_clause_table(table);
    summarize_rule_table(table);
    break;
  }
  }
}

extern bool read_eval(samp_table_t *table) {
  sort_table_t *sort_table = &table->sort_table;
  var_table_t *var_table = &table->var_table;
  atom_table_t *atom_table = &table->atom_table;
  
  fflush(stdout);
  // yyparse returns 0 and sets input_command if no syntax errors
  if (yyparse() == 0)
    switch (input_command.kind) {
    case PREDICATE: {
      input_pred_decl_t decl;
      decl = input_command.decl.pred_decl;
      cprintf(2, "Adding predicate %s\n", decl.atom->pred);
      add_predicate(decl.atom->pred, decl.atom->args, decl.witness, table);
      break;
    }
    case SORT: {
      input_sort_decl_t decl = input_command.decl.sort_decl;
      cprintf(2, "Adding sort %s\n", decl.name);
      add_sort(sort_table, decl.name);
      if (decl.sortdef != NULL) {
	add_sortdef(sort_table, decl.name, decl.sortdef);
      }
      break;
    }
    case SUBSORT: {
      input_subsort_decl_t decl = input_command.decl.subsort_decl;
      cprintf(2, "Adding subsort %s of %s information\n",
	      decl.subsort, decl.supersort);
      add_subsort(sort_table, decl.subsort, decl.supersort);
      break;
    }
    case CONSTD: {
      int32_t i;
      input_const_decl_t decl = input_command.decl.const_decl;
      for (i = 0; i < decl.num_names; i++) {
	cprintf(2, "Adding const %s\n", decl.name[i]);
	add_constant(decl.name[i], decl.sort, table);
      }
      break;
    }
    case VAR: {
      input_var_decl_t decl = input_command.decl.var_decl;
      if (sort_name_index(decl.sort, sort_table) == -1) {
	if (!strict_constants())
	  add_sort(sort_table, decl.sort);
	else {
	  mcsat_err("Sort %s has not been declared\n", decl.sort);
	  break;
	}
      }
      int32_t i;
      for (i=0; i<decl.num_names; i++) {
	// Need to see if name in const_table
	cprintf(2, "Adding var %s\n", decl.name[i]);
	add_var(var_table, decl.name[i], sort_table, decl.sort);
      }
      break;
    }
    case ATOMD: {
      // invoke add_atom
      input_atom_decl_t decl = input_command.decl.atom_decl;
      add_atom(table, decl.atom);
      break;
    }
    case ASSERT: {
      // Need to check that the predicate is a witness predicate,
      // then invoke assert_atom.
      input_assert_decl_t decl = input_command.decl.assert_decl;
      assert_atom(table, decl.atom, decl.source);
      break;
    }
    case ADD: {
      input_add_fdecl_t decl = input_command.decl.add_fdecl;
      cprintf(2, "Clausifying and adding formula\n");
      add_cnf(decl.frozen, decl.formula, decl.weight, decl.source, true);
      break;
    }
    case ADD_CLAUSE: {
      input_add_decl_t decl = input_command.decl.add_decl;
      if (decl.clause->varlen == 0) {
	// No variables - adding a clause
	cprintf(2, "Adding clause\n");
	add_clause(table,
		   decl.clause->literals, decl.weight, decl.source, true);
      }
      else {
	// Have variables - adding a rule
	double wt = decl.weight;
	if (wt == DBL_MAX) {
	  cprintf(2, "Adding rule with MAX weight\n");
	} else {
	  cprintf(2, "Adding rule with weight %f\n", wt);
	}
	int32_t ruleidx = add_rule(decl.clause, decl.weight, decl.source, table);
	// Create instances here rather than add_rule, as this is eager
	if (ruleidx != -1) {
	  all_rule_instances(ruleidx, table);
	}
      }
      break;
    }
    case ASK: {
      input_ask_fdecl_t decl = input_command.decl.ask_fdecl;
      cprintf(2, "ask: clausifying formula\n");
      ask_cnf(decl.formula, decl.threshold, decl.numresults);
      // Results are in ask_buffer - print them out
      print_ask_results(decl.formula, table);
      // Now clear out the query_instance table for the next query
      reset_query_instance_table(&table->query_instance_table);
      break;
    }
      //       case ASK_CLAUSE: {
      // 	input_ask_decl_t decl = input_command.decl.ask_decl;
      // 	assert(decl.clause->litlen == 1);
      // 	ask_clause(decl.clause, decl.threshold, decl.all, decl.num_samples);
      // 	break;
      //       }
    case MCSAT: {
	
      output("Calling %sMCSAT with parameters (set using mcsat_params):\n",
	     lazy_mcsat() ? "LAZY_" : "");
      output(" max_samples = %"PRId32"\n", get_max_samples());
      output(" sa_probability = %f\n", get_sa_probability());
      output(" samp_temperature = %f\n", get_samp_temperature());
      output(" rvar_probability = %f\n", get_rvar_probability());
      output(" max_flips = %"PRId32"\n", get_max_flips());
      output(" max_extra_flips = %"PRId32"\n", get_max_extra_flips());
      output(" timeout = %"PRId32"\n", get_mcsat_timeout());
      if (lazy_mcsat()) {
	lazy_mc_sat(table, get_max_samples(), get_sa_probability(),
		    get_samp_temperature(), get_rvar_probability(),
		    get_max_flips(), get_max_extra_flips(), get_mcsat_timeout());
      } else {
	mc_sat(table, get_max_samples(), get_sa_probability(),
	       get_samp_temperature(), get_rvar_probability(),
	       get_max_flips(), get_max_extra_flips(), get_mcsat_timeout());
      }
      output("\n");
      break;
    }
    case MCSAT_PARAMS: {
      input_mcsat_params_decl_t decl = input_command.decl.mcsat_params_decl;
      if (decl.num_params == 0) {
	output("MCSAT param values:\n");
	output(" max_samples = %"PRId32"\n", get_max_samples());
	output(" sa_probability = %f\n", get_sa_probability());
	output(" samp_temperature = %f\n", get_samp_temperature());
	output(" rvar_probability = %f\n", get_rvar_probability());
	output(" max_flips = %"PRId32"\n", get_max_flips());
	output(" max_extra_flips = %"PRId32"\n", get_max_extra_flips());
	output(" timeout = %"PRId32"\n", get_mcsat_timeout());
      } else {
	output("Setting MCSAT parameters:\n");
	if (decl.max_samples >= 0) {
	  output(" max_samples was %"PRId32", now %"PRId32"\n",
		 get_max_samples(), decl.max_samples);
	  set_max_samples(decl.max_samples);
	}
	if (decl.sa_probability >= 0) {
	  output(" sa_probability was %f, now %f\n",
		 get_sa_probability(), decl.sa_probability);
	  set_sa_probability(decl.sa_probability);
	}
	if (decl.samp_temperature >= 0) {
	  output(" samp_temperature was %f, now %f\n",
		 get_samp_temperature(), decl.samp_temperature);
	  set_samp_temperature(decl.samp_temperature);
	}
	if (decl.rvar_probability >= 0) {
	  output(" rvar_probability was %f, now %f\n",
		 get_rvar_probability(), decl.rvar_probability);
	  set_rvar_probability(decl.rvar_probability);
	}
	if (decl.max_flips >= 0) {
	  output(" max_flips was %"PRId32", now  %"PRId32"\n",
		 get_max_flips(), decl.max_flips);
	  set_max_flips(decl.max_flips);
	}
	if (decl.max_extra_flips >= 0) {
	  output(" max_extra_flips was %"PRId32", now %"PRId32"\n",
		 get_max_extra_flips(), decl.max_extra_flips);
	  set_max_extra_flips(decl.max_extra_flips);
	}
	if (decl.timeout >= 0) {
	  output(" timeout was %"PRId32", now %"PRId32"\n",
		 get_mcsat_timeout(), decl.timeout);
	  set_mcsat_timeout(decl.timeout);
	}
      }
      output("\n");
      break;
    }
    case RESET: {
      input_reset_decl_t decl = input_command.decl.reset_decl;
      switch (decl.kind) {
      case ALL: {
	// Resets the sample tables
	reset_sort_table(sort_table);
	// Need to do more here - like free up space.
	init_samp_table(table);
	init_gen_rand(pce_rand_seed);
	break;
      }
      case PROBABILITIES: {
	// Simply resets the probabilities of the atom table to -1.0
	output("Resetting probabilities of atoms to -1.0\n");
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
    case RETRACT: {
      input_retract_decl_t decl = input_command.decl.retract_decl;
      retract_source(decl.source,table);
      break;
    }
    case LOAD: {
      input_load_decl_t decl = input_command.decl.load_decl;
      load_mcsat_file(decl.file, table);
      break;
    }
    case DUMPTABLE: {
      input_dumptable_decl_t decl = input_command.decl.dumptable_decl;
      dumptable(decl.table, table);
      break;
    }
    case VERBOSITY: {
      input_verbosity_decl_t decl = input_command.decl.verbosity_decl;
      set_verbosity_level(decl.level);
      cprintf(1, "Setting verbosity to %"PRId32"\n", decl.level);
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
      return true;
      break;
    };
  free_parse_data();
  //yylloc.first_line += yylloc.last_line;
  //yylloc.first_column += yyloc.last_column;
  return false;
}
	
extern void read_eval_print_loop(char *file, samp_table_t *table) {
  
  input_stack_push_file(file); // sets parse_file and parse_input
  //yydebug = 1;
  yylloc.first_line = 1;
  yylloc.first_column = 0;
  yylloc.last_line = 1;
  yylloc.last_column = 0;
  do {
    if (input_is_stdin(parse_input)) {
      printf("mcsat> ");
    }
  } while (! read_eval(table));
  input_stack_pop();
  return;
}

extern void load_mcsat_file(char *file, samp_table_t *table) {
  read_eval_print_loop(file, table);
}
