/* Parser for standalone PCE */

%{
#include <ctype.h>
#include <string.h>
#include <float.h>
#include "memalloc.h"
#include "samplesat.h"
#include "mcsat.h"
#define YYDEBUG 1

  void yyerror (char *str) {
    printf("pce: %s\n", str);
  }
  
  int yylex (void);
  char* yytext;
  static input_literal_t* yylits[2000];
  static int yylitlen = 0;
  static char* yyargs[300];
  static int yyvarlen = 0;
  static char* yyvars[300];
  static int yyarglen = 0;

  input_command_t * input_command;

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

  bool yy_check_float(char *str){
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
    input_command_t * command
      = (input_command_t *) safe_malloc(sizeof(input_command_t));
    command->kind = kind;
    command->decl = decl;
    input_command = command;
  };

  void yy_preddecl (input_atom_t *atom, bool witness) {
    input_preddecl_t * preddecl
      = (input_preddecl_t *) safe_malloc(sizeof(input_preddecl_t));
    preddecl->atom = atom;
    preddecl->witness = witness;
    yy_command(PREDICATE, (input_decl_t *) preddecl);
  };
  void yy_sortdecl (char *name) {
    input_sortdecl_t * sortdecl
      = (input_sortdecl_t *) safe_malloc(sizeof(input_sortdecl_t));
    sortdecl->name = name;
    yy_command(SORT, (input_decl_t *) sortdecl);
  };
  void yy_constdecl (char *sort) {
    // yyargs has yyarglen string arguments
    int32_t i;
    input_constdecl_t * constdecl =
      (input_constdecl_t *) safe_malloc(sizeof(input_constdecl_t));
    constdecl->name = safe_malloc(yyarglen*sizeof(char *));
    for (i = 0; i<yyarglen; i++) {
      constdecl->name[i] = yyargs[i];
    }
    constdecl->num_names = yyarglen;
    yyarglen = 0;
    constdecl->sort = sort;
    yy_command(CONST, (input_decl_t *) constdecl);
  };
  void yy_vardecl (char *name, char *sort) {
    input_vardecl_t * vardecl
      = (input_vardecl_t *) safe_malloc(sizeof(input_vardecl_t));
    vardecl->name = name;
    vardecl->sort = sort;
    yy_command(VAR, (input_decl_t *) vardecl);
  };
  void yy_atomdecl (input_atom_t *atom) {
    input_atomdecl_t *atomdecl
      = (input_atomdecl_t *) safe_malloc(sizeof(input_atomdecl_t));
    atomdecl->atom = atom;
    yy_command(ATOM, (input_decl_t *) atomdecl);
  };
  void yy_assertdecl (input_atom_t *atom) {
    input_assertdecl_t * assertdecl
      = (input_assertdecl_t *) safe_malloc(sizeof(input_assertdecl_t));
    assertdecl->atom = atom;
    yy_command(ASSERT, (input_decl_t *) assertdecl);
  };
  void yy_adddecl (input_clause_t *clause, char *wt) {
    input_adddecl_t * adddecl
      = (input_adddecl_t *) safe_malloc(sizeof(input_adddecl_t));
    adddecl->clause = clause;
    adddecl->weight = (strcmp(wt, "DBL_MAX") == 0) ? DBL_MAX : atof(wt);
    yy_command(ADD, (input_decl_t *) adddecl);
  };
  void yy_askdecl (input_clause_t * clause) {
    input_askdecl_t * askdecl
      = (input_askdecl_t *) safe_malloc(sizeof(input_askdecl_t));
    askdecl->clause = clause;
    yy_command(ASK, (input_decl_t *) askdecl);
  };
  bool yy_mcsatdecl () {
    // yyargs has yyarglen string arguments
    // We map them to sa_probability, samp_temperature, rvar_probability,
    // max_flips, and max_samples, in that order.  Missing args get default values.
    if (yyarglen > 5) {
      yyerror("mcsat: too many args");
      yyarglen = 0;
      return false;
    }
    input_mcsatdecl_t *mcsatdecl
      = (input_mcsatdecl_t *) safe_malloc(sizeof(input_mcsatdecl_t));
    // sa_probability
    if (yyarglen > 0) {
      if (yy_check_float(yyargs[0])) {
	mcsatdecl->sa_probability = atof(yyargs[0]);
      } else {
	yyarglen = 0;
	return false;
      }
    } else {
      mcsatdecl->sa_probability = DEFAULT_SA_PROBABILITY;
    }
    // samp_temperature
    if (yyarglen > 1) {
      if (yy_check_float(yyargs[1])) {
	mcsatdecl->samp_temperature = atof(yyargs[1]);
      } else {
	yyarglen = 0;
	return false;
      }
    } else {
      mcsatdecl->samp_temperature = DEFAULT_SAMP_TEMPERATURE;
    }
    // rvar_probability
    if (yyarglen > 2) {
      if (yy_check_float(yyargs[2])) {
	mcsatdecl->rvar_probability = atof(yyargs[2]);
      } else {
	yyarglen = 0;
	return false;
      }
    } else {
      mcsatdecl->rvar_probability = DEFAULT_RVAR_PROBABILITY;
    }
    // max_flips
    if (yyarglen > 3) {
      if (yy_check_int(yyargs[3])) {
	mcsatdecl->max_flips = atoi(yyargs[3]);
      } else {
	yyarglen = 0;
	return false;
      }
    } else {
      mcsatdecl->max_flips = DEFAULT_MAX_FLIPS;
    }
    // max_samples
    if (yyarglen > 4) {
      if (yy_check_int(yyargs[4])) {
	mcsatdecl->max_samples = atoi(yyargs[4]);
      } else {
	yyarglen = 0;
	return false;
      }
    } else {
      mcsatdecl->max_samples = DEFAULT_MAX_SAMPLES;
    }
    yy_command(MCSAT, (input_decl_t *) mcsatdecl);
    yyarglen = 0;
    return true;
  };
  void yy_dumptables () {
    yy_command(DUMPTABLES, (input_decl_t *) NULL);
  };
  void yy_reset (char *name) {
    if (strcasecmp(name, "PROBABILITIES") == 0) {
      yy_command(RESET, (input_decl_t *) NULL);
    } else {
      yyerror("Can only reset 'probabilities' at the moment");
    }
  };
  void yy_test () {
    yy_command(TEST, (input_decl_t *) NULL);
  };
  void yy_verbosity (char *level) {
    int32_t i;
    for (i=0; level[i] != '\0'; i++) {
      if (! isdigit(level[i])) {
	yyerror("Invalid verbosity");
      }
    }
    input_verbositydecl_t *verbositydecl
      = (input_verbositydecl_t *) safe_malloc(sizeof(input_verbositydecl_t));
    verbositydecl->level = atoi(level);
    yy_command(VERBOSITY, (input_decl_t *) verbositydecl);
  };
  void yy_help () {
    yy_command(HELP, (input_decl_t *) NULL);
  };
  void yy_quit () {
    yy_command(QUIT, (input_decl_t *) NULL);
  };

  input_literal_t *yy_literal(bool neg, input_atom_t *atom) {
    input_literal_t * lit
      = (input_literal_t *) safe_malloc(sizeof(input_literal_t));
    lit->neg = neg;
    lit->atom = atom;
    return lit;
  }

  void copy_yyargs_to_yyvars() {
    yyvarlen = yyarglen;
    int i;
    for (i=0; i<yyarglen; i++) {
      yyvars[i] = yyargs[i];
    }
    yyarglen = 0;
  }
  input_clause_t *yy_clause() {
    // yyargs has yyarglen string arguments
    // yylits has yylitlen literals
    int i;
    input_clause_t* clause
      = (input_clause_t *) safe_malloc(sizeof(input_clause_t));
    clause->varlen = yyvarlen;
    clause->variables
      = (char **) safe_malloc(yyvarlen * sizeof(char *));
    clause->litlen = yylitlen;
    clause->literals
      = (input_literal_t **) safe_malloc((yylitlen + 1) * sizeof(input_literal_t));
    for (i = 0; i < yyvarlen; i++) {
      clause->variables[i] = yyvars[i];
    }
    for (i = 0; i < yylitlen; i++) {
      clause->literals[i] = yylits[i];
    }
    clause->literals[yylitlen] = NULL;
    yyvarlen = 0;
    yylitlen = 0;
    return clause;
  }

  input_atom_t *yy_atom(char* pred) {
    // yyargs has yyarglen string aruments
    input_atom_t * atom =
      (input_atom_t *) safe_malloc((yyarglen + 1) * sizeof(char *));
    int i;
    atom->pred = str_copy(pred);
    for (i = 0; i < yyarglen; i++) {
      atom->args[i] = yyargs[i];
    }
    atom->args[i] = (char *) NULL;
    yyarglen = 0;
    return atom;
  }

  %}

%token PREDICATE
%token DIRECT
%token INDIRECT
%token SORT
%token CONST
%token VAR
%token ATOM
%token ASSERT
%token ADD
%token ASK
%token MCSAT
%token RESET
%token DUMPTABLES
%token VERBOSITY
%token TEST
%token HELP
%token QUIT

%token NAME
%token NUM

%union
{
  bool bval;
  char *str;
  char **strs;
  input_clause_t *clause;
  input_literal_t *lit;
  input_atom_t *atom;
}

%type <str> arg NAME NUM addwt
%type <strs> args
%type <clause> clause
%type <lit> literal
%type <atom> atom
%type <bval> witness

%start command

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

command: decl ';' { YYACCEPT; } | QUIT { yy_quit(); YYACCEPT; }; 

decl: PREDICATE atom witness { yy_preddecl($2, $3); }
| SORT NAME { yy_sortdecl($2); }
| CONST args ':' NAME { yy_constdecl($4); }
| VAR NAME ':' NAME { yy_vardecl($2, $4); }
| ASSERT atom { yy_assertdecl($2); }
| ATOM atom { yy_atomdecl($2); }
| ADD clause addwt { yy_adddecl($2, $3); }
| ASK clause { yy_askdecl($2); }
| MCSAT eargs {if (! yy_mcsatdecl()) YYABORT;}
| RESET NAME {yy_reset($2);}
| DUMPTABLES { yy_dumptables(); }
| VERBOSITY NUM {yy_verbosity($2);}
| TEST { yy_test(); }
| HELP { yy_help(); }
;

witness: DIRECT {$$ = true;} | INDIRECT {$$ = false;}
       | /* empty */ {$$ = true;};

clause: literals { $$ = yy_clause(); };
      | '(' args ')' { copy_yyargs_to_yyvars(); }
        literals { $$ = yy_clause(); };

literals: literal { yylits[yylitlen] = $1; yylitlen+=1; }
        | literals '|' literal { yylits[yylitlen] = $3; yylitlen+=1; }
        ;

literal: atom {$$ = yy_literal(0,$1);}
       | '~' atom {$$ = yy_literal(1,$2);}
       ;

atom: NAME '(' args ')' {$$ = yy_atom($1);}
    ;

eargs: /*empty*/ | args ;


args: arg { yyargs[yyarglen] = str_copy($1); yyarglen+=1;}
    | args ',' arg { yyargs[yyarglen] = str_copy($3); yyarglen+=1;}
    ;

arg: NAME | NUM ;

addwt: NUM | /* empty */ {$$ = "DBL_MAX";};

%%

int isidentchar(int c) {
  return !(isspace(c) || iscntrl(c) || c =='(' || c == ')' || c == '|'
	   || c == ',' || c == ';' || c == ':');
}

int yylex (void) {
  int c;
  char str[300];
  /* skip white space  */
  do {
    c = getchar();
  } while (isspace(c));

  int i = 0;
  /* process numbers - note that we process for as long as it could be a
     legitimate number, but return the string, so that numbers may be used
     as arguments, but treated as names.  Number is of the form
     ['+'|'-']d*'.'d* 
  */
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
      str[i++] = c;
      c = getchar();
    } while (c != EOF && (isdigit(c) || c == '.'));
    str[i] = '\0';
    ungetc(c, stdin);
    if (! have_digit)
      yyerror("Malformed number - '.', '+', '-' must have following digits");
    yylval.str = str_copy(str);
    return NUM;
  }
  if (isalpha(c)) {
    do {
      str[i++] = c;
      c = getchar();
    } while (c != EOF && isidentchar(c));
    str[i] = '\0';
    ungetc(c, stdin);
    yylval.str = str_copy(str);
    if (strcasecmp(str, "PREDICATE") == 0)
      return PREDICATE;
    else if (strcasecmp(str, "DIRECT") == 0)
      return DIRECT;
    else if (strcasecmp(str, "INDIRECT") == 0)
      return INDIRECT;
    else if (strcasecmp(str, "SORT") == 0)
      return SORT;
    else if (strcasecmp(str, "CONST") == 0)
      return CONST;
    else if (strcasecmp(str, "VAR") == 0)
      return VAR;
    else if (strcasecmp(str, "ATOM") == 0)
      return ATOM;
    else if (strcasecmp(str, "ASSERT") == 0)
      return ASSERT;
    else if (strcasecmp(str, "ADD") == 0)
      return ADD;
    else if (strcasecmp(str, "ASK") == 0)
      return ASK;
    else if (strcasecmp(str, "MCSAT") == 0)
      return MCSAT;
    else if (strcasecmp(str, "RESET") == 0)
      return RESET;
    else if (strcasecmp(str, "DUMPTABLES") == 0)
      return DUMPTABLES;
    else if (strcasecmp(str, "VERBOSITY") == 0)
      return VERBOSITY;
    else if (strcasecmp(str, "TEST") == 0)
      return TEST;
    else if (strcasecmp(str, "HELP") == 0)
      return HELP;
    else if (strcasecmp(str, "QUIT") == 0)
      return QUIT;
    else
      return NAME;
  }
  /* return end-of-file  */
  if (c == EOF) return QUIT;
  /* return single chars */
  return c;
}
