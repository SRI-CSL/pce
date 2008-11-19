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
#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YYINCLUDED_STDLIB_H 1

extern void free_parse_data();

static bool yyerrflg = false;

// These are buffers for holding things until they are malloc'ed
static char yystrbuf[1000];
static pvector_t yyargs;
static pvector_t yylits;
FILE *parse_input;

void yyerror (char *str);

char **copy_yyargs() {
  char **arg;
  int32_t i;

  arg = (char **) safe_malloc((yyargs.size + 1) * sizeof(char *));
  for (i = 0; i<yyargs.size; i++) {
    arg[i] = yyargs.data[i];
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

  void yy_sortdecl (char *name) {
    input_command.kind = SORT;
    input_command.decl.sortdecl.name = name;
  };
  void yy_subsortdecl (char *subsort, char *supersort) {
    input_command.kind = SUBSORT;
    input_command.decl.subsortdecl.subsort = subsort;
    input_command.decl.subsortdecl.supersort = supersort;
  };
  void yy_preddecl (input_atom_t *atom, bool witness) {
    input_command.kind = PREDICATE;
    input_command.decl.preddecl.atom = atom;
    input_command.decl.preddecl.witness = witness;
  };
  void yy_constdecl (char **name, char *sort) {
    int32_t i = 0;
    
    input_command.kind = CONST;
    while (name[i] != NULL) {i++;}
    input_command.decl.constdecl.num_names = i;
    input_command.decl.constdecl.name = name;
    input_command.decl.constdecl.sort = sort;
  };
  void yy_vardecl (char **name, char *sort) {
    int32_t i = 0;
    
    input_command.kind = VAR;
    while (name[i] != NULL) {i++;}
    input_command.decl.constdecl.num_names = i;
    input_command.decl.vardecl.name = name;
    input_command.decl.vardecl.sort = sort;
  };
  void yy_atomdecl (input_atom_t *atom) {
    input_command.kind = ATOM;
    input_command.decl.atomdecl.atom = atom;
  };
  void yy_assertdecl (input_atom_t *atom) {
    input_command.kind = ASSERT;
    input_command.decl.assertdecl.atom = atom;
  };
  void yy_adddecl (input_clause_t *clause, char *wt) {
    input_command.kind = ADD;
    input_command.decl.adddecl.clause = clause;
    if (strcmp(wt, "DBL_MAX") == 0) {
      input_command.decl.adddecl.weight = DBL_MAX;
    } else {
      input_command.decl.adddecl.weight = atof(wt);
      safe_free(wt);
    }
  };
  void yy_askdecl (input_clause_t *clause, char *thresholdstr, char *allstr) {
    double threshold;
    input_command.kind = ASK;
    input_command.decl.askdecl.clause = clause;
    if (clause->litlen > 1) {
      yyerror("ask: currently only allows a single literal with variables");
      free_clause(clause);
    }
    threshold = atof(thresholdstr);
    if (0.0 <= threshold && threshold <= 1.0) {
      input_command.decl.askdecl.threshold = threshold;
    } else {
      free_clause(clause);
      safe_free(thresholdstr);
      safe_free(allstr);
      yyerror("ask: threshold must be between 0.0 and 1.0");
      return;
    }
    safe_free(thresholdstr);
    if (allstr == NULL || strcasecmp(allstr, "BEST") == 0) {
      input_command.decl.askdecl.all = false;
    } else if (strcasecmp(allstr, "ALL") == 0) {
      input_command.decl.askdecl.all = true;
    } else {
      free_clause(clause);
      safe_free(thresholdstr);
      safe_free(allstr);
      yyerror("ask: which must be 'all' or 'best'");
      return;
    }
    safe_free(allstr);
  };
  
/* The params map to sa_probability, samp_temperature, rvar_probability,
 * max_flips, max_extra_flips, and max_samples, in that order.  Missing
 * args get default values.
 */
void yy_mcsatdecl (char **params) {
  int32_t arglen = 0;
  if (params != NULL) {
    while (arglen <= 6 && (params[arglen] != NULL)) {
      arglen++;
    }
  }
  if (arglen > 6) {
    yyerror("mcsat: too many args");
  }
  input_command.kind = MCSAT;
  // sa_probability
  if (arglen > 0) {
    if (yy_check_float(params[0])) {
      double prob = atof(params[0]);
      if (0.0 <= prob && prob <= 1.0) {
	input_command.decl.mcsatdecl.sa_probability = prob;
      } else {
	yyerror("sa_probability should be between 0.0 and 1.0");
      }
    }
  } else {
    input_command.decl.mcsatdecl.sa_probability = DEFAULT_SA_PROBABILITY;
  }
  // samp_temperature
  if (arglen > 1) {
    if (yy_check_float(params[1])) {
      double temp = atof(params[1]);
      if (temp > 0.0) {
	input_command.decl.mcsatdecl.samp_temperature = temp;
      } else {
	yyerror("samp_temperature should be greater than 0.0");
      }
    }
  } else {
    input_command.decl.mcsatdecl.samp_temperature = DEFAULT_SAMP_TEMPERATURE;
  }
  // rvar_probability
  if (arglen > 2) {
    if (yy_check_float(params[2])) {
      double prob = atof(params[2]);
      if (0.0 <= prob && prob <= 1.0) {
	input_command.decl.mcsatdecl.rvar_probability = prob;
      } else {
	yyerror("rvar_probability should be between 0.0 and 1.0");
      }
    }
  } else {
    input_command.decl.mcsatdecl.rvar_probability = DEFAULT_RVAR_PROBABILITY;
  }
  // max_flips
  if (arglen > 3) {
    if (yy_check_int(params[3])) {
      input_command.decl.mcsatdecl.max_flips = atoi(params[3]);
    }
  } else {
    input_command.decl.mcsatdecl.max_flips = DEFAULT_MAX_FLIPS;
  }
  // max_extra_flips
  if (arglen > 4) {
    if (yy_check_int(params[4])) {
      input_command.decl.mcsatdecl.max_extra_flips = atoi(params[4]);
    }
  } else {
    input_command.decl.mcsatdecl.max_flips = DEFAULT_MAX_EXTRA_FLIPS;
  }
  // max_samples
  if (arglen > 5) {
    if (yy_check_int(params[5])) {
      input_command.decl.mcsatdecl.max_samples = atoi(params[5]);
    }
  } else {
    input_command.decl.mcsatdecl.max_samples = DEFAULT_MAX_SAMPLES;
  }
  free_strings(params);
};
  void yy_dumptables () {
    input_command.kind = DUMPTABLES;
  };
  void yy_reset (char *name) {
    input_command.kind = RESET;
    if (name == NULL || strcasecmp(name, "ALL") ==0) {
      input_command.decl.resetdecl.kind = ALL;
    } else if (strcasecmp(name, "PROBABILITIES") == 0) {
      input_command.decl.resetdecl.kind = PROBABILITIES;      
    } else {
      yyerror("Reset must be omitted, 'all', or 'probabilities'");
    }
    safe_free(name);
  };
  void yy_load (char *name) {
    input_command.kind = LOAD;
    input_command.decl.loaddecl.file = name;
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
    input_command.decl.verbositydecl.level = atoi(level);
    safe_free(level);
  };
  void yy_help (char* name) {
    input_command.kind = HELP;
    input_command.decl.helpdecl.command = name;
  };
  void yy_quit () {
    input_command.kind = QUIT;
  };

  %}

%token PREDICATE
%token DIRECT
%token INDIRECT
%token SORT
%token SUBSORT
%token CONST
%token VAR
%token ATOM
%token ASSERT
%token ADD
%token ASK
%token MCSAT
%token RESET
%token DUMPTABLES
%token LOAD
%token VERBOSITY
%token TEST
%token HELP
%token QUIT

%token NAME
%token NUM
%token STRING

%union
{
  bool bval;
  char *str;
  char **strs;
  input_clause_t *clause;
  input_literal_t *lit;
  input_literal_t **lits;
  input_atom_t *atom;
}

%type <str> arg NAME NUM STRING addwt oarg oname
%type <strs> arguments variables oarguments
%type <clause> clause
%type <lit> literal 
%type <lits> literals
%type <atom> atom
%type <bval> witness

%start command

%initial-action {
  yyargs.size = 0;
  yylits.size = 0;
  yyerrflg = false;
  yylloc.first_line = yylloc.last_line = 1;
  yylloc.first_column = yylloc.last_column = 0;
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

command: decl ';' {if (yyerrflg) {yyerrflg=false; YYABORT;} else {YYACCEPT;}}
       | QUIT {yy_quit(); YYACCEPT;}; 

decl: SORT NAME {yy_sortdecl($2);}
    | SUBSORT NAME NAME {yy_subsortdecl($2, $3);}
    | PREDICATE atom witness {yy_preddecl($2, $3);}
    | CONST arguments ':' NAME {yy_constdecl($2, $4);}
    | VAR arguments ':' NAME {yy_vardecl($2, $4);}
    | ATOM atom {yy_atomdecl($2);}
    | ASSERT atom {yy_assertdecl($2);}
    | ADD clause addwt {yy_adddecl($2, $3);}
    | ASK clause NUM oarg {yy_askdecl($2, $3, $4);}
    | MCSAT oarguments {yy_mcsatdecl($2);}
    | RESET oarg {yy_reset($2);}
    | DUMPTABLES {yy_dumptables();}
    | LOAD STRING {yy_load($2);}
    | VERBOSITY NUM {yy_verbosity($2);}
    | TEST {yy_test(@1);}
    | HELP oname {yy_help($2);}
    ;

witness: DIRECT {$$ = true;} | INDIRECT {$$ = false;}
       | /* empty */ {$$ = true;};

clause: literals {$$ = yy_clause(NULL, $1);};
      | '(' variables ')'
	literals {$$ = yy_clause($2, $4);};

literals: lits {$$ = copy_yylits();};

lits: literal {pvector_push(&yylits, $1);}
    | lits '|' literal {pvector_push(&yylits, $3);}
    ;

literal: atom {$$ = yy_literal(0,$1);}
       | '~' atom {$$ = yy_literal(1,$2);}
       ;

atom: NAME '(' arguments ')' {$$ = yy_atom($1, $3);}
    ;

oarguments: /*empty*/ {$$ = NULL;} | arguments;

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

%%

int isidentchar(int c) {
  return !(isspace(c) || iscntrl(c) || c =='(' || c == ')' || c == '|'
	   || c == ',' || c == ';' || c == ':');
}

int skip_white_space_and_comments (void) {
  int c;

  do {
    do {
      c = getc(parse_input);
      if (c == '\n') {
	++yylloc.last_line;
	yylloc.last_column = 0;
      } else {
	++yylloc.last_column;
      }
    } while (isspace(c));
    if (c == '#') {
      do {
	c = getc(parse_input);
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
      c = getc(parse_input);
      ++yylloc.last_column;
    } while (c != EOF && (isdigit(c) || c == '.'));
    yystrbuf[i] = '\0';
    ungetc(c, parse_input);
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
      c = getc(parse_input);
      ++yylloc.last_column;      
    } while (c != EOF && isidentchar(c));
    yystrbuf[i] = '\0';
    ungetc(c, parse_input);
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
    else if (strcasecmp(yylval.str, "MCSAT") == 0)
      return MCSAT;
    else if (strcasecmp(yylval.str, "RESET") == 0)
      return RESET;
    else if (strcasecmp(yylval.str, "DUMPTABLES") == 0)
      return DUMPTABLES;
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
    else
      nstr = (char *) safe_malloc((strlen(yylval.str)+1) * sizeof(char));
    strcpy(nstr, yylval.str);
    yylval.str = nstr;
    return NAME;
  }
  if (c == '\"') {
    // At the moment, escapes not recognized
    do {
      c = getc(parse_input);
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
      c = getc(parse_input);
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
  return c;
}


/* Free the memory allocated for the declaration */
  

void free_preddecl_data() {
  free_atom(input_command.decl.preddecl.atom);
}

void free_sortdecl_data() {
  safe_free(input_command.decl.sortdecl.name);
}

void free_subsortdecl_data() {
  safe_free(input_command.decl.subsortdecl.subsort);
  safe_free(input_command.decl.subsortdecl.supersort);
}

void free_constdecl_data() {
  free_strings(input_command.decl.constdecl.name);
  safe_free(input_command.decl.constdecl.name);
  safe_free(input_command.decl.constdecl.sort);
}

void free_vardecl_data() {
  free_strings(input_command.decl.vardecl.name);
  safe_free(input_command.decl.vardecl.sort);
}

void free_atomdecl_data() {
  free_atom(input_command.decl.atomdecl.atom);
}

void free_assertdecl_data() {
  free_atom(input_command.decl.assertdecl.atom);
}

void free_adddecl_data() {
  free_clause(input_command.decl.adddecl.clause);
}

void free_askdecl_data() {
  free_clause(input_command.decl.askdecl.clause);
}

void free_mcsatdecl_data() {
  // Nothing to do here
}

void free_resetdecl_data() {
  // Nothing to do here
}

void free_loaddecl_date() {
  safe_free(input_command.decl.loaddecl.file);
}

void free_dumptablesdecl_data() {
  // Nothing to do here
}

void free_verbositydecl_data() {
  // Nothing to do here
}

void free_testdecl_data() {
  // Nothing to do here
}

void free_helpdecl_data() {
  // Nothing to do here
}

void free_quitdecl_data() {
  // Nothing to do here
}

void free_parse_data () {
  switch (input_command.kind) {
  case PREDICATE: {
    free_preddecl_data();
    break;
  }
  case SORT: {
    free_sortdecl_data();
    break;
  }
  case SUBSORT: {
    free_subsortdecl_data();
    break;
  }
  case CONST: {
    free_constdecl_data();
    break;
  }
  case VAR: {
    free_vardecl_data();
    break;
  }
  case ATOM: {
    free_atomdecl_data();
    break;
  }
  case ASSERT: {
    free_assertdecl_data();
    break;
  }
  case ADD: {
    free_adddecl_data();
    break;
  }
  case ASK: {
    free_askdecl_data();
    break;
  }
  case MCSAT: {
    free_mcsatdecl_data();
    break;
  }
  case RESET: {
    free_resetdecl_data();
    break;
  }
  case DUMPTABLES: {
    free_dumptablesdecl_data();
    break;
  }
  case VERBOSITY: {
    free_verbositydecl_data();
    break;
  }
  case TEST: {
    free_testdecl_data();
    break;
  }
  case HELP: {
    free_helpdecl_data();
    break;
  }
  case QUIT: {
    free_quitdecl_data();
    break;
  }
  }
}

void yyerror (char *str) {
  printf("pce:%d: %s\n", yylloc.first_line, str);
  input_command.kind = 0;
  yyerrflg = true;
};

// Local variables:
// c-electric-flag: nil
// End:
