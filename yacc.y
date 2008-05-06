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
  void yy_dumptables () {
    yy_command(DUMPTABLES, (input_decl_t *) NULL);
  };
  void yy_test () {
    yy_command(TEST, (input_decl_t *) NULL);
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
%token ASSERT
%token ADD
%token ASK
%token DUMPTABLES
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
| ADD clause addwt { yy_adddecl($2, $3); }
| ASK clause { yy_askdecl($2); }
| DUMPTABLES { yy_dumptables(); }
| TEST { yy_test(); }
| HELP { yy_help(); }
;

witness: DIRECT {$$ = true;} | INDIRECT {$$ = false;};

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
  while ((c = getchar()) == ' ' || c == '\t' || c == '\n')
    ;
  int i = 0;
  /* process numbers - note that we process for as long as it could be a
     legitimate number, but return the string, so that numbers may be used
     as arguments, but treated as names.  Number is of the form
     ['+'|'-']d*'.'d* */
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
    else if (strcasecmp(str, "ASSERT") == 0)
      return ASSERT;
    else if (strcasecmp(str, "ADD") == 0)
      return ADD;
    else if (strcasecmp(str, "ASK") == 0)
      return ASK;
    else if (strcasecmp(str, "DUMPTABLES") == 0)
      return DUMPTABLES;
    else if (strcasecmp(str, "TEST") == 0)
      return TEST;
    else if (strcasecmp(str, "HELP") == 0)
      return HELP;
    else
      return NAME;
  }
  /* return end-of-file  */
  if (c == EOF) return QUIT;
  /* return single chars */
  return c;
}
