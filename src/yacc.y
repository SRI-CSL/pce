/* Parser for standalone PCE */

%{
#include <ctype.h>
#include <string.h>
#include <float.h>
#include "memalloc.h"
#include "samplesat.h"
#include "mcsat.h"
#include "input.h"
#include "parser.h"
#include "vectors.h"
#include "tables.h"
#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YYINCLUDED_STDLIB_H 1

extern void free_parse_data();

static bool yyerrflg = false;

// These are buffers for holding things until they are malloc'ed
static char yystrbuf[1000];
static pvector_t yyargs;
static pvector_t yylits;
parse_input_t *parse_input;

void yyerror (char *str);

char **copy_yyargs() {
  char **arg;
  int32_t i;

  arg = (char **) safe_malloc((yyargs.size + 1) * sizeof(char *));
  for (i = 0; i<yyargs.size; i++) {
    if (yyargs.data[i] == NULL) {
      arg[i] = "";
    } else {
      arg[i] = yyargs.data[i];
    }
  }
  arg[i] = NULL;
  // Now that it's copied, reset the vector
  yyargs.size=0;
  return arg;
}  

input_atom_t *yy_atom (char *pred, char **args) {
  input_atom_t *atom;
  
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  atom->pred = pred;
  atom->args = args;
  return atom;
};

input_formula_t *yy_formula (char **vars, input_fmla_t *fmla) {
  input_formula_t *formula;
  int32_t i, vlen;

  formula = (input_formula_t *) safe_malloc(sizeof(input_formula_t));
  if (vars == NULL) {
    formula->vars = NULL;
  } else {
    for (vlen = 0; vars[vlen] != NULL; vlen++) {}
    formula->vars = (var_entry_t **)
      safe_malloc((vlen + 1) * sizeof(var_entry_t *));
    for (i = 0; i < vlen; i++) {
      formula->vars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
      formula->vars[i]->name = vars[i];
      formula->vars[i]->sort_index = -1;
    }
    formula->vars[vlen] = NULL;
  }
  formula->fmla = fmla;
  return formula;
};

input_fmla_t *yy_fmla (int32_t op, input_fmla_t *arg1, input_fmla_t *arg2) {
  input_fmla_t *fmla;
  input_ufmla_t *ufmla;
  input_comp_fmla_t *cfmla;

  fmla = (input_fmla_t *) safe_malloc(sizeof(input_fmla_t));
  ufmla = (input_ufmla_t *) safe_malloc(sizeof(input_ufmla_t));
  cfmla = (input_comp_fmla_t *) safe_malloc(sizeof(input_comp_fmla_t));
  fmla->kind = op;
  fmla->ufmla = ufmla;
  ufmla->cfmla = cfmla;
  cfmla->op = op;
  cfmla->arg1 = (input_fmla_t *) arg1;
  cfmla->arg2 = arg2;
  return fmla;
};

input_fmla_t *yy_atom_to_fmla (input_atom_t *atom) {
  input_fmla_t *fmla;
  input_ufmla_t *ufmla;

  fmla = (input_fmla_t *) safe_malloc(sizeof(input_fmla_t));
  ufmla = (input_ufmla_t *) safe_malloc(sizeof(input_ufmla_t));
  fmla->kind = ATOM;
  fmla->ufmla = ufmla;
  ufmla->atom = atom;
  return fmla;
};

input_literal_t *yy_literal(bool neg, input_atom_t *atom) {
  input_literal_t *lit;
  
  lit = (input_literal_t *) safe_malloc(sizeof(input_literal_t));
  lit->neg = neg;
  lit->atom = atom;
  return lit;
};

input_literal_t **copy_yylits () {
  input_literal_t **lits;
  int32_t i;

  lits = (input_literal_t **)
    safe_malloc((yylits.size + 1) * sizeof(input_literal_t *));
  for (i = 0; i<yylits.size; i++) {
    lits[i] = yylits.data[i];
  }
  lits[i] = NULL;
  yylits.size = 0;
  return lits;
}
  
input_clause_t *yy_clause(char **vars, input_literal_t **lits) {
  input_clause_t *clause;
  int32_t varlen = 0;
  int32_t litlen = 0;

  clause = (input_clause_t *) safe_malloc(sizeof(input_clause_t));
  if (vars != NULL) {
    while (vars[varlen] != NULL) {varlen++;}
  }
  clause->varlen = varlen;
  clause->variables = vars;
  while (lits[litlen] != NULL) {litlen++;}
  clause->litlen = litlen;
  clause->literals = lits;
  return clause;
}
  
int yylex (void);
input_command_t input_command;

bool yy_check_int(char *str){
  int32_t i;
  for(i=0; str[i] != '\0'; i++){
    if (! isdigit(str[i])) {
      yyerror("Integer expected");
      return false;
    }
  }
  return true;
}

bool yy_check_float(char *str) {
  bool have_digit = false;
  bool have_dot = false;
  int32_t i;
  if (str[0] == '.' || str[0] == '+' || str[0] == '-' || isdigit(str[0])) {
    if (str[0] == '.') {
      have_dot = true;
    } else if (isdigit(str[0])) {
      have_digit = true;
    }
    for(i=1; str[i] != '\0'; i++){
      if (isdigit(str[i])){
	have_digit = true;
      } else if (str[i] == '.') {
	if (have_dot) {
	  yyerror("Number has two decimal points");
	  return false;
	}
	else
	  have_dot = true;
      } else {
	yyerror("Invalid floating point number");
	return false;
      }
    }
  } else {
    yyerror("Invalid floating point number");
    return false;
  }
  return true;
}

  void yy_command(int kind, input_decl_t *decl) {
    input_command.kind = kind;
    //input_command.decl = decl;
  };

  void yy_sort_decl (char *name) {
    input_command.kind = SORT;
    input_command.decl.sort_decl.name = name;
  };
  void yy_subsort_decl (char *subsort, char *supersort) {
    input_command.kind = SUBSORT;
    input_command.decl.subsort_decl.subsort = subsort;
    input_command.decl.subsort_decl.supersort = supersort;
  };
  void yy_pred_decl (input_atom_t *atom, bool witness) {
    input_command.kind = PREDICATE;
    input_command.decl.pred_decl.atom = atom;
    input_command.decl.pred_decl.witness = witness;
  };
  void yy_const_decl (char **name, char *sort) {
    int32_t i = 0;
    
    input_command.kind = CONST;
    while (name[i] != NULL) {i++;}
    input_command.decl.const_decl.num_names = i;
    input_command.decl.const_decl.name = name;
    input_command.decl.const_decl.sort = sort;
  };
  void yy_var_decl (char **name, char *sort) {
    int32_t i = 0;
    
    input_command.kind = VAR;
    while (name[i] != NULL) {i++;}
    input_command.decl.const_decl.num_names = i;
    input_command.decl.var_decl.name = name;
    input_command.decl.var_decl.sort = sort;
  };
  void yy_atom_decl (input_atom_t *atom) {
    input_command.kind = ATOM;
    input_command.decl.atom_decl.atom = atom;
  };
  void yy_assert_decl (input_atom_t *atom, char *source) {
    input_command.kind = ASSERT;
    input_command.decl.assert_decl.atom = atom;
    input_command.decl.assert_decl.source = source;
  };
  void yy_add_fdecl (input_formula_t *formula, char *wt, char *source) {
    input_command.kind = ADD;
    input_command.decl.add_fdecl.formula = formula;
    input_command.decl.add_fdecl.source = source;
    if (strcmp(wt, "DBL_MAX") == 0) {
      input_command.decl.add_fdecl.weight = DBL_MAX;
    } else {
      input_command.decl.add_fdecl.weight = atof(wt);
      safe_free(wt);
    }
  };
  void yy_add_decl (input_clause_t *clause, char *wt, char *source) {
    input_command.kind = ADD_CLAUSE;
    input_command.decl.add_decl.clause = clause;
    input_command.decl.add_decl.source = source;
    if (strcmp(wt, "DBL_MAX") == 0) {
      input_command.decl.add_decl.weight = DBL_MAX;
    } else {
      input_command.decl.add_decl.weight = atof(wt);
      safe_free(wt);
    }
  };
  void yy_ask_fdecl (input_formula_t *formula, char *numsamp, char *thresholdstr) {
    double threshold;
    input_command.kind = ASK;
    input_command.decl.ask_fdecl.formula = formula;
    if (numsamp == NULL) {
      input_command.decl.ask_fdecl.num_samples = get_max_samples();
    } else if (yy_check_int(numsamp)) {
      input_command.decl.ask_fdecl.num_samples = atoi(numsamp);
    } else {
      free_formula(formula);
      safe_free(numsamp);
      safe_free(thresholdstr);
      yyerror("ask: NUMBER_OF_SAMPLES must be an integer");
    }
    safe_free(numsamp);
    if (thresholdstr == NULL) {
      input_command.decl.ask_fdecl.threshold = 0;
    } else if (strcmp(thresholdstr,"BEST") == 0) {
      input_command.decl.ask_fdecl.threshold = -1;
    } else {
      threshold = atof(thresholdstr);
      if (0.0 <= threshold && threshold <= 1.0) {
	input_command.decl.ask_fdecl.threshold = threshold;
      } else {
	free_formula(formula);
	safe_free(numsamp);
	safe_free(thresholdstr);
	yyerror("ask: THRESHOLD must be between 0.0 and 1.0");
	return;
      }
      safe_free(thresholdstr);
    }
  };
  
//   void yy_ask_decl (input_clause_t *clause, char *thresholdstr, char *allstr, char *numsamp) {
//     double threshold;
//     input_command.kind = ASK_CLAUSE;
//     input_command.decl.ask_decl.clause = clause;
//     threshold = atof(thresholdstr);
//     if (0.0 <= threshold && threshold <= 1.0) {
//       input_command.decl.ask_decl.threshold = threshold;
//     } else {
//       free_clause(clause);
//       safe_free(thresholdstr);
//       safe_free(allstr);
//       safe_free(numsamp);
//       yyerror("ask_clause: threshold must be between 0.0 and 1.0");
//       return;
//     }
//     safe_free(thresholdstr);
//     if (allstr == NULL || strcasecmp(allstr, "BEST") == 0) {
//       input_command.decl.ask_decl.all = false;
//     } else if (strcasecmp(allstr, "ALL") == 0) {
//       input_command.decl.ask_decl.all = true;
//     } else {
//       free_clause(clause);
//       safe_free(allstr);
//       safe_free(numsamp);
//       yyerror("ask_clause: which must be 'all' or 'best'");
//       return;
//     }
//     safe_free(allstr);
//     if (numsamp == NULL) {
//       input_command.decl.ask_decl.num_samples = DEFAULT_MAX_SAMPLES;
//     } else if (yy_check_int(numsamp)) {
//       input_command.decl.ask_decl.num_samples = atoi(numsamp);
//     } else {
//       free_clause(clause);
//       safe_free(numsamp);
//       yyerror("ask_clause: numsamps must be an integer");
//     }
//     safe_free(numsamp);
// };
  
/* The params map to sa_probability, samp_temperature, rvar_probability,
 * max_flips, max_extra_flips, and max_samples, in that order.  Missing
 * args get default values.
 */
void yy_mcsat_decl (char **params) {
  //yy_get_mcsat_params(params);
  input_command.kind = MCSAT;
}

void yy_mcsat_params_decl (char **params) {
  int32_t arglen = 0;
  
  input_command.kind = MCSAT_PARAMS;
  // Determine num_params
  if (params != NULL) {
    while (arglen <= 6 && (params[arglen] != NULL)) {
      arglen++;
    }
  }
  if (arglen > 6) {
    yyerror("mcsat_params: too many args");
  }
  input_command.decl.mcsat_params_decl.num_params = arglen;
  // sa_probability
  if (arglen > 0 && strcmp(params[0], "") != 0) {
    if (yy_check_float(params[0])) {
      double prob = atof(params[0]);
      if (0.0 <= prob && prob <= 1.0) {
	input_command.decl.mcsat_params_decl.sa_probability = prob;
      } else {
	yyerror("sa_probability should be between 0.0 and 1.0");
      }
    }
  } else {
    input_command.decl.mcsat_params_decl.sa_probability = -1;
  }
  // samp_temperature
  if (arglen > 1 && strcmp(params[1], "") != 0) {
    if (yy_check_float(params[1])) {
      double temp = atof(params[1]);
      if (temp > 0.0) {
	input_command.decl.mcsat_params_decl.samp_temperature = temp;
      } else {
	yyerror("samp_temperature should be greater than 0.0");
      }
    }
  } else {
    input_command.decl.mcsat_params_decl.samp_temperature = -1;
  }
  // rvar_probability
  if (arglen > 2 && strcmp(params[2], "") != 0) {
    if (yy_check_float(params[2])) {
      double prob = atof(params[2]);
      if (0.0 <= prob && prob <= 1.0) {
	input_command.decl.mcsat_params_decl.rvar_probability = prob;
      } else {
	yyerror("rvar_probability should be between 0.0 and 1.0");
      }
    }
  } else {
    input_command.decl.mcsat_params_decl.rvar_probability = -1;
  }
  // max_flips
  if (arglen > 3 && strcmp(params[3], "") != 0) {
    if (yy_check_int(params[3])) {
      input_command.decl.mcsat_params_decl.max_flips = atoi(params[3]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_flips = -1;
  }
  // max_extra_flips
  if (arglen > 4 && strcmp(params[4], "") != 0) {
    if (yy_check_int(params[4])) {
      input_command.decl.mcsat_params_decl.max_extra_flips = atoi(params[4]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_extra_flips = -1;
  }
  // max_samples
  if (arglen > 5 && strcmp(params[5], "") != 0) {
    if (yy_check_int(params[5])) {
      input_command.decl.mcsat_params_decl.max_samples = atoi(params[5]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_samples = -1;
  }
  free_strings(params);
};
  void yy_dumptables (int32_t table) {
    input_command.kind = DUMPTABLE;
    input_command.decl.dumptable_decl.table = table;
  };
  void yy_reset (int32_t what) {
    input_command.kind = RESET;
    input_command.decl.reset_decl.kind = what;
  };
  void yy_retract (char *source) {
    input_command.kind = RETRACT;
    input_command.decl.retract_decl.source = source;
  };
  void yy_load (char *name) {
    input_command.kind = LOAD;
    input_command.decl.load_decl.file = name;
  };
  void yy_test () {
    input_command.kind = TEST;
  };
  void yy_verbosity (char *level) {
    int32_t i;
    for (i=0; level[i] != '\0'; i++) {
      if (! isdigit(level[i])) {
	yyerror("Invalid verbosity");
      }
    }
    input_command.kind = VERBOSITY;
    input_command.decl.verbosity_decl.level = atoi(level);
    safe_free(level);
  };
  void yy_help (int32_t command) {
    input_command.kind = HELP;
    input_command.decl.help_decl.command = command;
  };
  void yy_quit () {
    input_command.kind = QUIT;
  };

  %}

%token ALL
%token BEST
%token PROBABILITIES
%token PREDICATE
%token DIRECT
%token INDIRECT
%token CLAUSE
%token RULE
%token SORT
%token SUBSORT
%token CONST
%token VAR
%token ATOM
%token ASSERT
%token ADD
%token ASK
%token ADD_CLAUSE
//%token ASK_CLAUSE
%token MCSAT
%token MCSAT_PARAMS
%token RESET
%token RETRACT
%token DUMPTABLE
%token LOAD
%token VERBOSITY
%token TEST
%token HELP
%token QUIT
%left IFF
%right IMPLIES
%left OR
%left AND
%nonassoc NOT
%token NAME
%token NUM
%token STRING

%union
{
  bool bval;
  char *str;
  char **strs;
  input_fmla_t *fmla;
  input_formula_t *formula;
  input_clause_t *clause;
  input_literal_t *lit;
  input_literal_t **lits;
  input_atom_t *atom;
  int32_t ival;
}

%type <str> arg NAME NUM STRING addwt oarg oname onum onumorbest retractarg
%type <strs> arguments variables oarguments
%type <formula> formula
%type <fmla> fmla
%type <clause> clause
%type <lit> literal 
%type <lits> literals
%type <atom> atom
%type <bval> witness
%type <ival> cmd table resetarg

%start command

%initial-action {
  yyargs.size = 0;
  yylits.size = 0;
  yyerrflg = false;
  //yylloc.first_line = yylloc.last_line = 1;
  //yylloc.first_column = yylloc.last_column = 0;
 }

/* Add sorts, declarations (var, const have sorts)
   signature for predicate

   PREDICATE p(s1, ..., sn): DIRECT/INDIRECT;
   SORT s;
   CONST c: s;
   VAR v: s;
   ASSERT p(c1, ..., cn);
   ADD clause [weight];
   ASK clause;
*/

/* Grammar */
%%

command: decl enddecl {if (yyerrflg) {yyerrflg=false; YYABORT;} else {YYACCEPT;}}
       | QUIT {yy_quit(); YYACCEPT;}
       | error enddecl {yyerrflg=false; YYABORT;}
       ;

enddecl: ';' | QUIT;

decl: SORT NAME {yy_sort_decl($2);}
    | SUBSORT NAME NAME {yy_subsort_decl($2, $3);}
    | PREDICATE atom witness {yy_pred_decl($2, $3);}
    | CONST arguments ':' NAME {yy_const_decl($2, $4);}
    | VAR arguments ':' NAME {yy_var_decl($2, $4);}
    | ATOM atom {yy_atom_decl($2);}
    | ASSERT atom oname {yy_assert_decl($2, $3);}
    | ADD_CLAUSE clause addwt oname {yy_add_decl($2, $3, $4);}
//    | ASK_CLAUSE clause NUM oarg oarg {yy_ask_decl($2, $3, $4, $5);}
    | ADD formula addwt oname {yy_add_fdecl($2, $3, $4);}
    | ASK formula onum onumorbest {yy_ask_fdecl($2, $3, $4);}
    | MCSAT oarguments {yy_mcsat_decl($2);}
    | MCSAT_PARAMS oarguments {yy_mcsat_params_decl($2);}
    | RESET resetarg {yy_reset($2);}
    | RETRACT retractarg {yy_retract($2);}
    | DUMPTABLE table {yy_dumptables($2);}
    | LOAD STRING {yy_load($2);}
    | VERBOSITY NUM {yy_verbosity($2);}
    | TEST {yy_test(@1);}
    | HELP cmd {yy_help($2);}
    ;

cmd: /* empty */ {$$ = ALL;} | ALL {$$ = ALL;}
     | SORT {$$ = SORT;} | SUBSORT {$$ = SUBSORT;} | PREDICATE {$$ = PREDICATE;}
     | CONST {$$ = CONST;} | VAR {$$ = VAR;} | ATOM {$$ = ATOM;}
     | ASSERT {$$ = ASSERT;} | ADD {$$ = ADD;} | ADD_CLAUSE {$$ = ADD_CLAUSE;}
     | ASK {$$ = ASK;}
     //| ASK_CLAUSE {$$ = ASK_CLAUSE;}
     | MCSAT {$$ = MCSAT;} | MCSAT_PARAMS {$$ = MCSAT_PARAMS}
     | RESET {$$ = RESET;} | RETRACT {$$ = RETRACT;} | DUMPTABLE {$$ = DUMPTABLE;}
     | LOAD {$$ = LOAD;} | VERBOSITY {$$ = VERBOSITY;} | HELP {$$ = HELP;}
     ;

table: /* empty */ {$$ = ALL;} | ALL {$$ = ALL;}
       | SORT {$$ = SORT;} | PREDICATE {$$ = PREDICATE;} | ATOM {$$ = ATOM;}
       | CLAUSE {$$ = CLAUSE;} | RULE {$$ = RULE;}
       ;

witness: DIRECT {$$ = true;} | INDIRECT {$$ = false;}
       | /* empty */ {$$ = true;};

resetarg: /* empty */ {$$ = ALL;} | ALL {$$ = ALL;} | PROBABILITIES {$$ = PROBABILITIES;};

retractarg: ALL {$$ = "ALL";} | NAME;

formula: fmla {$$ = yy_formula(NULL, $1);}
       | '[' variables ']' fmla {$$ = yy_formula($2, $4);}
       ;

fmla: atom {$$ = yy_atom_to_fmla($1);}
    | fmla IFF fmla {$$ = yy_fmla(IFF, $1, $3);}
    | fmla IMPLIES fmla {$$ = yy_fmla(IMPLIES, $1, $3);}
    | fmla OR fmla {$$ = yy_fmla(OR, $1, $3);}
    | fmla AND fmla {$$ = yy_fmla(AND, $1, $3);}
    | NOT fmla {$$ = yy_fmla(NOT, $2, NULL);}
    | '(' fmla ')' {$$ = $2;}
    ;

// formula: iff_formula {$$ = yy_formula(NULL, $1);}
//        | '(' variables ')' iff_formula {$$ = yy_formula($2, $4);};

// iff_formula: implies_formula
//            | iff_formula IFF implies_formula {yy_fmla(IFF, $1, $3);};

// implies_formula: or_formula
//                | implies_formula IMPLIES or_formula {yy_fmla(IMPLIES, $1, $3);};

// or_formula: and_formula
//           | or_formula OR and_formula {yy_fmla(OR, $1, $3);};

// and_formula: not_formula
//            | and_formula AND not_formula {yy_fmla(AND, $1, $3);};

// not_formula: atom {$$ = yy_atom_to_fmla($1);}
//            | NOT not_formula {yy_fmla(NOT, $2, NULL);};

clause: literals {$$ = yy_clause(NULL, $1);};
      | '(' variables ')'
	literals {$$ = yy_clause($2, $4);};

literals: lits {$$ = copy_yylits();};

lits: literal {pvector_push(&yylits, $1);}
    | lits '|' literal {pvector_push(&yylits, $3);}
    ;

literal: atom {$$ = yy_literal(0,$1);}
       | NOT atom {$$ = yy_literal(1,$2);}
       ;

atom: NAME '(' arguments ')' {$$ = yy_atom($1, $3);}
    ;

oarguments: /*empty*/ {$$ = NULL;} | oargs {$$ = copy_yyargs();};

oargs: oarg {pvector_push(&yyargs,$1);}
      | oargs ',' oarg {pvector_push(&yyargs,$3);}
      ;

arguments: args {$$ = copy_yyargs();};

args: arg {pvector_push(&yyargs,$1);}
      | args ',' arg {pvector_push(&yyargs,$3);}
      ;

variables: vars {$$ = copy_yyargs();};
   
vars: NAME {pvector_push(&yyargs, $1);}
      | vars ',' NAME {pvector_push(&yyargs, $3);}
      ;

oarg: /*empty*/ {$$=NULL;} | arg;

arg: NAME | NUM; // returns string from yystrings

addwt: NUM | /* empty */ {$$ = "DBL_MAX";};

oname: /* empty */ {$$=NULL;} | NAME;
onum: /* empty */ {$$=NULL;} | NUM;
onumorbest: /* empty */ {$$=NULL;} | NUM | BEST {$$="BEST"};

%%

int isidentchar(int c) {
  return !(isspace(c) || iscntrl(c) || c =='(' || c == ')' || c == '|'
	   || c == '[' || c == ']' || c == ',' || c == ';' || c == ':');
}

int yygetc(parse_input_t *input) {
  if (input->kind == INFILE) {
    return getc(input->input.in_file.fps);
  } else {
    return input->input.in_string.string[input->input.in_string.index++];
  }
}

int yyungetc(int c, parse_input_t *input) {
  if (input->kind == INFILE) {
    return ungetc(c, input->input.in_file.fps);
  } else {
    input->input.in_string.index--;
    return c;
  }
}

// if YYRECOVERING need to read till no more input, then output ';'
int skip_white_space_and_comments (void) {
  int c;

  do {
    do {
      c = yygetc(parse_input);
      if (c == '\n') {
	if (yynerrs && (input_is_stdin(parse_input))) {
	  return ';';
	} else {
	  ++yylloc.last_line;
	  yylloc.last_column = 0;
	}
      } else {
	++yylloc.last_column;
      }
    } while (isspace(c));
    if (c == '#') {
      do {
	c = yygetc(parse_input);
	++yylloc.last_column;
      } while (c != '\n' && c != EOF);
    } else {
      return c;
    }
  } while (true);
  return c;
}

int yylex (void) {
  int c;
  int32_t i;
  char *nstr;
  
  c = skip_white_space_and_comments();
  yylloc.first_line = yylloc.last_line;
  yylloc.first_column = yylloc.last_column;

  /* process numbers - note that we process for as long as it could be a
     legitimate number, but return the string, so that numbers may be used
     as arguments, but treated as names.  Number is of the form
     ['+'|'-']d*'.'d* 
  */
  i = 0;
  if (c == '.' || c == '-' || c == '+' || isdigit(c)) {
    int have_digit = 0;
    int have_dot = 0;
    do {
      if (! have_digit && isdigit(c))
	have_digit = 1;
      if (c == '.') {
	if (have_dot)
	  yyerror("Number has two decimal points");
	else
	  have_dot = 1;
      };
      yystrbuf[i++] = c;
      c = yygetc(parse_input);
      ++yylloc.last_column;
    } while (c != EOF && (isdigit(c) || c == '.'));
    yystrbuf[i] = '\0';
    yyungetc(c, parse_input);
    --yylloc.last_column;
    yylval.str = yystrbuf;
    if (! have_digit)
      yyerror("Malformed number - '.', '+', '-' must have following digits");
    nstr = (char *) safe_malloc((strlen(yylval.str)+1) * sizeof(char));
    strcpy(nstr, yylval.str);
    yylval.str = nstr;
    return NUM;
  }
  if (isalpha(c)) {
    do {
      yystrbuf[i++] = c;
      c = yygetc(parse_input);
      ++yylloc.last_column;      
    } while (c != EOF && isidentchar(c));
    yystrbuf[i] = '\0';
    yyungetc(c, parse_input);
    --yylloc.last_column;
    yylval.str = yystrbuf;
    if (strcasecmp(yylval.str, "PREDICATE") == 0)
      return PREDICATE;
    else if (strcasecmp(yylval.str, "DIRECT") == 0)
      return DIRECT;
    else if (strcasecmp(yylval.str, "INDIRECT") == 0)
      return INDIRECT;
    else if (strcasecmp(yylval.str, "SORT") == 0)
      return SORT;
    else if (strcasecmp(yylval.str, "SUBSORT") == 0)
      return SUBSORT;
    else if (strcasecmp(yylval.str, "CONST") == 0)
      return CONST;
    else if (strcasecmp(yylval.str, "VAR") == 0)
      return VAR;
    else if (strcasecmp(yylval.str, "ATOM") == 0)
      return ATOM;
    else if (strcasecmp(yylval.str, "ASSERT") == 0)
      return ASSERT;
    else if (strcasecmp(yylval.str, "ADD") == 0)
      return ADD;
    else if (strcasecmp(yylval.str, "ASK") == 0)
      return ASK;
    else if (strcasecmp(yylval.str, "ADD_CLAUSE") == 0)
      return ADD_CLAUSE;
//     else if (strcasecmp(yylval.str, "ASK_CLAUSE") == 0)
//       return ASK_CLAUSE;
    else if (strcasecmp(yylval.str, "MCSAT") == 0)
      return MCSAT;
    else if (strcasecmp(yylval.str, "MCSAT_PARAMS") == 0)
      return MCSAT_PARAMS;
    else if (strcasecmp(yylval.str, "RESET") == 0)
      return RESET;
    else if (strcasecmp(yylval.str, "RETRACT") == 0)
      return RETRACT;
    else if (strcasecmp(yylval.str, "DUMPTABLE") == 0
	     || strcasecmp(yylval.str, "DUMPTABLES") == 0)
      return DUMPTABLE;
    else if (strcasecmp(yylval.str, "CLAUSE") == 0)
      return CLAUSE;
    else if (strcasecmp(yylval.str, "RULE") == 0)
      return RULE;
    else if (strcasecmp(yylval.str, "LOAD") == 0)
      return LOAD;
    else if (strcasecmp(yylval.str, "VERBOSITY") == 0)
      return VERBOSITY;
    else if (strcasecmp(yylval.str, "TEST") == 0)
      return TEST;
    else if (strcasecmp(yylval.str, "HELP") == 0)
      return HELP;
    else if (strcasecmp(yylval.str, "QUIT") == 0)
      return QUIT;
    else if (strcasecmp(yylval.str, "ALL") == 0)
      return ALL;
    else if (strcasecmp(yylval.str, "BEST") == 0)
      return BEST;
    else if (strcasecmp(yylval.str, "PROBABILITIES") == 0)
      return PROBABILITIES;
    else if (strcasecmp(yylval.str, "IFF") == 0)
      return IFF;
    else if (strcasecmp(yylval.str, "IMPLIES") == 0)
      return IMPLIES;
    else if (strcasecmp(yylval.str, "OR") == 0)
      return OR;
    else if (strcasecmp(yylval.str, "AND") == 0)
      return AND;
    else if (strcasecmp(yylval.str, "NOT") == 0)
      return NOT;
    else
      nstr = (char *) safe_malloc((strlen(yylval.str)+1) * sizeof(char));
    strcpy(nstr, yylval.str);
    yylval.str = nstr;
    return NAME;
  }
  if (c == '\"') {
    // At the moment, escapes not recognized
    do {
      c = yygetc(parse_input);
      ++yylloc.last_column;      
      if (c == '\"' || c == EOF) {
        break;
      } else {
        yystrbuf[i++] = c;
      }
    } while (true);
    yystrbuf[i] = '\0';
    nstr = (char *) safe_malloc((strlen(yystrbuf)+1) * sizeof(char));
    strcpy(nstr, yystrbuf);
    yylval.str = nstr;
    return STRING;
  }
  if (c == '\'') {
    // At the moment, escapes not recognized
    do {
      c = yygetc(parse_input);
      ++yylloc.last_column;      
      if (c == '\'' || c == EOF) {
        break;
      } else {
        yystrbuf[i++] = c;
      }
    } while (true);
    if (c == EOF) {
      yyerror ("EOF reached while expecting '");
      return 0;
    }
    yystrbuf[i] = '\0';
    nstr = (char *) safe_malloc((strlen(yystrbuf)+1) * sizeof(char));
    strcpy(nstr, yystrbuf);
    yylval.str = nstr;
    return NAME;
  }
  /* return end-of-file  */
  if (c == EOF) return QUIT;
  /* return single chars */
  if (c == '~') {
    return NOT;
  } else {
    return c;
  }
}


/* Free the memory allocated for the declaration */
  

void free_pred_decl_data() {
  free_atom(input_command.decl.pred_decl.atom);
}

void free_sort_decl_data() {
  safe_free(input_command.decl.sort_decl.name);
}

void free_subsort_decl_data() {
  safe_free(input_command.decl.subsort_decl.subsort);
  safe_free(input_command.decl.subsort_decl.supersort);
}

void free_const_decl_data() {
  free_strings(input_command.decl.const_decl.name);
  safe_free(input_command.decl.const_decl.name);
  safe_free(input_command.decl.const_decl.sort);
}

void free_var_decl_data() {
  free_strings(input_command.decl.var_decl.name);
  safe_free(input_command.decl.var_decl.sort);
}

void free_atom_decl_data() {
  free_atom(input_command.decl.atom_decl.atom);
}

void free_assert_decl_data() {
  free_atom(input_command.decl.assert_decl.atom);
}

void free_add_fdecl_data() {
  free_formula(input_command.decl.add_fdecl.formula);
}

void free_ask_fdecl_data() {
  free_formula(input_command.decl.ask_fdecl.formula);
}

void free_add_decl_data() {
  free_clause(input_command.decl.add_decl.clause);
}

void free_ask_decl_data() {
  free_clause(input_command.decl.ask_decl.clause);
}

void free_mcsat_decl_data() {
  // Nothing to do here
}

void free_mcsat_params_decl_data() {
  // Nothing to do here
}

void free_reset_decl_data() {
  // Nothing to do here
}

void free_retract_decl_data() {
  // Nothing to do here
}

void free_load_decl_date() {
  safe_free(input_command.decl.load_decl.file);
}

void free_dumptables_decl_data() {
  // Nothing to do here
}

void free_verbosity_decl_data() {
  // Nothing to do here
}

void free_testdecl_data() {
  // Nothing to do here
}

void free_help_decl_data() {
  // Nothing to do here
}

void free_quit_decl_data() {
  // Nothing to do here
}

void free_parse_data () {
  switch (input_command.kind) {
  case PREDICATE: {
    free_pred_decl_data();
    break;
  }
  case SORT: {
    free_sort_decl_data();
    break;
  }
  case SUBSORT: {
    free_subsort_decl_data();
    break;
  }
  case CONST: {
    free_const_decl_data();
    break;
  }
  case VAR: {
    free_var_decl_data();
    break;
  }
  case ATOM: {
    free_atom_decl_data();
    break;
  }
  case ASSERT: {
    free_assert_decl_data();
    break;
  }
  case ADD: {
    free_add_fdecl_data();
    break;
  }
  case ASK: {
    free_ask_fdecl_data();
    break;
  }
  case ADD_CLAUSE: {
    free_add_decl_data();
    break;
  }
//   case ASK_CLAUSE: {
//     free_ask_decl_data();
//     break;
//   }
  case MCSAT: {
    free_mcsat_decl_data();
    break;
  }
  case MCSAT_PARAMS: {
    free_mcsat_params_decl_data();
    break;
  }
  case RESET: {
    free_reset_decl_data();
    break;
  }
  case RETRACT: {
    free_retract_decl_data();
    break;
  }
  case DUMPTABLE: {
    free_dumptables_decl_data();
    break;
  }
  case VERBOSITY: {
    free_verbosity_decl_data();
    break;
  }
  case TEST: {
    free_testdecl_data();
    break;
  }
  case HELP: {
    free_help_decl_data();
    break;
  }
  case QUIT: {
    free_quit_decl_data();
    break;
  }
  }
}

void yyerror (char *str) {
  if (parse_input->kind == INFILE) {
    printf("%s:%d:%d: %s\n", parse_input->input.in_file.file,
	   yylloc.last_line, yylloc.last_column, str);
  } else {
    printf("%s:%d:%d: %s\n", parse_input->input.in_string.string,
	   yylloc.last_line, yylloc.last_column, str);
  }
  input_command.kind = 0;
  yyerrflg = true;
};

// Local variables:
// c-electric-flag: nil
// End:
