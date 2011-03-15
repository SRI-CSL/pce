/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
//#line 3 "yacc.y"

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
#define INIT_YYSTRBUF_SIZE 200
uint32_t yystrbuf_size = INIT_YYSTRBUF_SIZE;
char *yystrbuf = NULL;
static pvector_t yyargs;
static pvector_t yylits;
parse_input_t *parse_input;

void yyerror (char *str);

void yystrbuf_add(char c, uint32_t i) {
  uint32_t n;
  
  if (yystrbuf == NULL) {
    yystrbuf = (char *) safe_malloc(yystrbuf_size * sizeof(char));
  } else {
    if (i+1 >= yystrbuf_size) {
      n = yystrbuf_size + 1;
      n += n >> 1;
      if (n >= UINT32_MAX) {
	out_of_memory();
      }
      yystrbuf_size = n;
      yystrbuf = (char *) safe_realloc(yystrbuf, n * sizeof(char));
    }
  }
  yystrbuf[i] = c;
}

char **copy_yyargs() {
  char **arg;
  int32_t i;

  arg = (char **) safe_malloc((yyargs.size + 1) * sizeof(char *));
  for (i = 0; i<yyargs.size; i++) {
    if (yyargs.data[i] == NULL) {
      arg[i] = "";
    } else {
      arg[i] = (char*) yyargs.data[i];
    }
  }
  arg[i] = NULL;
  // Now that it's copied, reset the vector
  yyargs.size=0;
  return arg;
}  

bool yy_check_nat(char *str){
  int32_t i;
  for(i=0; str[i] != '\0'; i++){
    if (! isdigit(str[i])) {
      yyerror("Integer expected");
      return false;
    }
  }
  return true;
}

bool yy_check_int(char *str) {
  if (str[0] == '+' || str[0] == '-') {
    return yy_check_nat(&str[1]);
  } else {
    return yy_check_nat(str);
  }
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

input_sortdef_t *yy_sortdef(char *lbnd, char *ubnd) {
  input_sortdef_t *sdef;

  if (strcmp(lbnd, "MIN") == 0 || yy_check_int(lbnd)) {
    if (strcmp(ubnd, "MAX") == 0 || yy_check_int(ubnd)) {
      sdef = (input_sortdef_t *) safe_malloc(sizeof(input_sortdef_t));
      
      sdef->lower_bound = (strcmp(lbnd, "MIN") == 0) ? INT32_MIN : atoi(lbnd);
      sdef->upper_bound = (strcmp(ubnd, "MAX") == 0) ? INT32_MAX : atoi(ubnd);
      return sdef;
    } else {
      yyerror("upper bound must be an integer");
      return NULL;
    }
  } else {
    yyerror("lower bound must be an integer");
    return NULL;
  }
}

input_atom_t *yy_atom (char *pred, char **args, int32_t builtinop) {
  input_atom_t *atom;
  
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  atom->pred = pred;
  atom->args = args;
  atom->builtinop = builtinop;
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
  fmla->atomic = false;
  fmla->ufmla = ufmla;
  ufmla->cfmla = cfmla;
  cfmla->op = op;
  cfmla->arg1 = arg1;
  cfmla->arg2 = arg2;
  return fmla;
};

input_fmla_t *yy_atom_to_fmla (input_atom_t *atom) {
  input_fmla_t *fmla;
  input_ufmla_t *ufmla;

  fmla = (input_fmla_t *) safe_malloc(sizeof(input_fmla_t));
  ufmla = (input_ufmla_t *) safe_malloc(sizeof(input_ufmla_t));
  fmla->atomic = true;
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
    lits[i] = (input_literal_t*) yylits.data[i];
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

  void yy_command(int kind, input_decl_t *decl) {
    input_command.kind = kind;
    //input_command.decl = decl;
  };
  void yy_sort_decl (char *name, input_sortdef_t *sortdef) {
    input_command.kind = SORT;
    input_command.decl.sort_decl.name = name;
    input_command.decl.sort_decl.sortdef = sortdef;
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
    
    input_command.kind = CONSTD;
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
    input_command.kind = ATOMD;
    input_command.decl.atom_decl.atom = atom;
  };
  void yy_assert_decl (input_atom_t *atom, char *source) {
    input_command.kind = ASSERT;
    input_command.decl.assert_decl.atom = atom;
    input_command.decl.assert_decl.source = source;
  };
  void yy_add_fdecl (char **frozen, input_formula_t *formula, char *wt, char *source) {
    input_command.kind = ADD;
    input_command.decl.add_fdecl.frozen = frozen;
    input_command.decl.add_fdecl.formula = formula;
    input_command.decl.add_fdecl.source = source;
    if (strcmp(wt, "DBL_MAX") == 0) {
      input_command.decl.add_fdecl.weight = DBL_MAX;
    } else {
      input_command.decl.add_fdecl.weight = strtod(wt, NULL);
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
  
void yy_ask_fdecl (input_formula_t *formula, char **threshold_numresult) {
  double threshold;

  input_command.kind = ASK;
  input_command.decl.ask_fdecl.formula = formula;
  if (threshold_numresult == NULL) {
    input_command.decl.ask_fdecl.threshold = 0;
    input_command.decl.ask_fdecl.numresults = 0; // Zero means no limit
  } else {
    threshold = atof(threshold_numresult[0]);
    if (0.0 <= threshold && threshold <= 1.0) {
      input_command.decl.ask_fdecl.threshold = threshold;
    } else {
      free_formula(formula);
      safe_free(threshold_numresult);
      yyerror("ask: THRESHOLD must be between 0.0 and 1.0");
      return;
    }
    if (threshold_numresult[1] == NULL) {
      input_command.decl.ask_fdecl.numresults = 0; // Zero means no limit
    } else if (yy_check_nat(threshold_numresult[1])) {
      input_command.decl.ask_fdecl.numresults = atoi(threshold_numresult[1]);
    } else {
      free_formula(formula);
      safe_free(threshold_numresult);
      yyerror("ask: NUMRESULTS must be an integer");
    }
    safe_free(threshold_numresult);
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
//     } else if (yy_check_nat(numsamp)) {
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
void yy_mcsat_decl () {
  //yy_get_mcsat_params(params);
  input_command.kind = MCSAT;
}

void yy_mcsat_params_decl (char **params) {
  int32_t arglen = 0;
  
  input_command.kind = MCSAT_PARAMS;
  // Determine num_params
  if (params != NULL) {
    while (arglen <= 7 && (params[arglen] != NULL)) {
      arglen++;
    }
  }
  if (arglen > 7) {
    yyerror("mcsat_params: too many args");
  }
  input_command.decl.mcsat_params_decl.num_params = arglen;
  // max_samples
  if (arglen > 0 && strcmp(params[0], "") != 0) {
    if (yy_check_nat(params[0])) {
      input_command.decl.mcsat_params_decl.max_samples = atoi(params[0]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_samples = -1;
  }
  // sa_probability
  if (arglen > 1 && strcmp(params[1], "") != 0) {
    if (yy_check_float(params[1])) {
      double prob = atof(params[1]);
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
  if (arglen > 2 && strcmp(params[2], "") != 0) {
    if (yy_check_float(params[2])) {
      double temp = atof(params[2]);
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
  if (arglen > 3 && strcmp(params[3], "") != 0) {
    if (yy_check_float(params[3])) {
      double prob = atof(params[3]);
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
  if (arglen > 4 && strcmp(params[4], "") != 0) {
    if (yy_check_nat(params[4])) {
      input_command.decl.mcsat_params_decl.max_flips = atoi(params[4]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_flips = -1;
  }
  // max_extra_flips
  if (arglen > 5 && strcmp(params[5], "") != 0) {
    if (yy_check_nat(params[5])) {
      input_command.decl.mcsat_params_decl.max_extra_flips = atoi(params[5]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_extra_flips = -1;
  }
  // timeout
  if (arglen > 6 && strcmp(params[6], "") != 0) {
    if (yy_check_nat(params[6])) {
      input_command.decl.mcsat_params_decl.timeout = atoi(params[6]);
    }
  } else {
    input_command.decl.mcsat_params_decl.timeout = -1;
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

  

/* Line 189 of yacc.c  */
//#line 589 "yacc.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ALL = 258,
     BEST = 259,
     PROBABILITIES = 260,
     PREDICATE = 261,
     DIRECT = 262,
     INDIRECT = 263,
     CLAUSE = 264,
     RULE = 265,
     SORT = 266,
     SUBSORT = 267,
     CONSTD = 268,
     VAR = 269,
     ATOMD = 270,
     ASSERT = 271,
     ADD = 272,
     ASK = 273,
     ADD_CLAUSE = 274,
     MCSAT = 275,
     MCSAT_PARAMS = 276,
     RESET = 277,
     RETRACT = 278,
     DUMPTABLE = 279,
     SUMMARY = 280,
     LOAD = 281,
     VERBOSITY = 282,
     HELP = 283,
     QUIT = 284,
     IFF = 285,
     IMPLIES = 286,
     OR = 287,
     XOR = 288,
     AND = 289,
     NOT = 290,
     EQ = 291,
     NEQ = 292,
     LE = 293,
     LT = 294,
     GE = 295,
     GT = 296,
     DDOT = 297,
     INTEGER = 298,
     PLUS = 299,
     MINUS = 300,
     TIMES = 301,
     DIV = 302,
     REM = 303,
     NAME = 304,
     NUM = 305,
     STRING = 306
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
//#line 571 "yacc.y"

  bool bval;
  char *str;
  char **strs;
  input_sortdef_t *sortdef;
  input_fmla_t *fmla;
  input_formula_t *formula;
  input_clause_t *clause;
  input_literal_t *lit;
  input_literal_t **lits;
  input_atom_t *atom;
  int32_t ival;



/* Line 214 of yacc.c  */
//#line 692 "yacc.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
//#line 717 "yacc.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  100
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   202

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  62
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  33
/* YYNRULES -- Number of rules.  */
#define YYNRULES  118
/* YYNRULES -- Number of states.  */
#define YYNSTATES  177

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   306

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      56,    57,     2,     2,    59,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    53,    52,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    54,     2,    55,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    60,    58,    61,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    11,    13,    15,    19,    23,
      27,    32,    37,    40,    44,    49,    55,    59,    61,    64,
      67,    70,    73,    76,    79,    82,    83,    85,    87,    89,
      91,    93,    95,    97,    99,   101,   103,   105,   107,   109,
     111,   113,   115,   117,   119,   121,   122,   125,   127,   129,
     135,   136,   138,   140,   142,   144,   146,   148,   150,   152,
     154,   155,   156,   158,   160,   162,   164,   166,   171,   173,
     177,   181,   185,   189,   193,   196,   200,   202,   207,   209,
     211,   215,   217,   220,   225,   229,   234,   236,   238,   240,
     242,   244,   246,   248,   250,   252,   254,   256,   258,   260,
     264,   266,   268,   272,   274,   276,   280,   281,   283,   285,
     287,   289,   290,   291,   293,   294,   298,   299,   301
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      63,     0,    -1,    65,    64,    -1,    29,    -1,     1,    64,
      -1,    52,    -1,    29,    -1,    11,    49,    67,    -1,    12,
      49,    49,    -1,     6,    80,    71,    -1,    13,    85,    53,
      49,    -1,    14,    85,    53,    49,    -1,    15,    80,    -1,
      16,    80,    92,    -1,    19,    76,    91,    92,    -1,    17,
      93,    74,    91,    92,    -1,    18,    74,    94,    -1,    20,
      -1,    21,    83,    -1,    22,    72,    -1,    23,    73,    -1,
      24,    70,    -1,    26,    51,    -1,    27,    50,    -1,    28,
      66,    -1,    -1,     3,    -1,    11,    -1,    12,    -1,     6,
      -1,    13,    -1,    14,    -1,    15,    -1,    16,    -1,    17,
      -1,    19,    -1,    18,    -1,    20,    -1,    21,    -1,    22,
      -1,    23,    -1,    24,    -1,    26,    -1,    27,    -1,    28,
      -1,    -1,    36,    68,    -1,    69,    -1,    43,    -1,    54,
      50,    42,    50,    55,    -1,    -1,     3,    -1,    11,    -1,
       6,    -1,    15,    -1,     9,    -1,    10,    -1,    25,    -1,
       7,    -1,     8,    -1,    -1,    -1,     3,    -1,     5,    -1,
       3,    -1,    49,    -1,    75,    -1,    54,    87,    55,    75,
      -1,    80,    -1,    75,    30,    75,    -1,    75,    31,    75,
      -1,    75,    32,    75,    -1,    75,    33,    75,    -1,    75,
      34,    75,    -1,    35,    75,    -1,    56,    75,    57,    -1,
      77,    -1,    56,    87,    57,    77,    -1,    78,    -1,    79,
      -1,    78,    58,    79,    -1,    80,    -1,    35,    80,    -1,
      49,    56,    85,    57,    -1,    90,    81,    90,    -1,    82,
      56,    85,    57,    -1,    36,    -1,    37,    -1,    39,    -1,
      38,    -1,    41,    -1,    40,    -1,    44,    -1,    45,    -1,
      46,    -1,    47,    -1,    48,    -1,    84,    -1,    89,    -1,
      84,    59,    89,    -1,    86,    -1,    90,    -1,    86,    59,
      90,    -1,    88,    -1,    49,    -1,    88,    59,    49,    -1,
      -1,    90,    -1,    49,    -1,    50,    -1,    50,    -1,    -1,
      -1,    49,    -1,    -1,    60,    87,    61,    -1,    -1,    50,
      -1,    50,    50,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   624,   624,   625,   626,   629,   629,   631,   632,   633,
     634,   635,   636,   637,   638,   640,   641,   642,   643,   644,
     645,   646,   647,   648,   650,   653,   653,   654,   654,   654,
     655,   655,   655,   656,   656,   656,   657,   659,   659,   660,
     660,   660,   661,   661,   661,   664,   665,   667,   667,   669,
     671,   671,   672,   672,   672,   673,   673,   673,   676,   676,
     677,   679,   679,   679,   681,   681,   683,   684,   687,   688,
     689,   690,   691,   692,   693,   694,   697,   698,   701,   703,
     704,   707,   708,   711,   712,   715,   718,   718,   718,   718,
     719,   719,   721,   721,   721,   722,   722,   724,   726,   727,
     730,   732,   733,   736,   738,   739,   742,   742,   744,   744,
     746,   746,   748,   748,   750,   751,   754,   755,   756
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ALL", "BEST", "PROBABILITIES",
  "PREDICATE", "DIRECT", "INDIRECT", "CLAUSE", "RULE", "SORT", "SUBSORT",
  "CONSTD", "VAR", "ATOMD", "ASSERT", "ADD", "ASK", "ADD_CLAUSE", "MCSAT",
  "MCSAT_PARAMS", "RESET", "RETRACT", "DUMPTABLE", "SUMMARY", "LOAD",
  "VERBOSITY", "HELP", "QUIT", "IFF", "IMPLIES", "OR", "XOR", "AND", "NOT",
  "EQ", "NEQ", "LE", "LT", "GE", "GT", "DDOT", "INTEGER", "PLUS", "MINUS",
  "TIMES", "DIV", "REM", "NAME", "NUM", "STRING", "';'", "':'", "'['",
  "']'", "'('", "')'", "'|'", "','", "'{'", "'}'", "$accept", "command",
  "enddecl", "decl", "cmd", "sortdef", "sortval", "interval", "table",
  "witness", "resetarg", "retractarg", "formula", "fmla", "clause",
  "literals", "lits", "literal", "atom", "bop", "preop", "oarguments",
  "oargs", "arguments", "args", "names", "nms", "oarg", "arg", "addwt",
  "oname", "ofrozen", "onum2", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,    59,    58,    91,    93,    40,    41,   124,    44,
     123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    62,    63,    63,    63,    64,    64,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    67,    67,    68,    68,    69,
      70,    70,    70,    70,    70,    70,    70,    70,    71,    71,
      71,    72,    72,    72,    73,    73,    74,    74,    75,    75,
      75,    75,    75,    75,    75,    75,    76,    76,    77,    78,
      78,    79,    79,    80,    80,    80,    81,    81,    81,    81,
      81,    81,    82,    82,    82,    82,    82,    83,    84,    84,
      85,    86,    86,    87,    88,    88,    89,    89,    90,    90,
      91,    91,    92,    92,    93,    93,    94,    94,    94
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     2,     1,     1,     3,     3,     3,
       4,     4,     2,     3,     4,     5,     3,     1,     2,     2,
       2,     2,     2,     2,     2,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     2,     1,     1,     5,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     0,     1,     1,     1,     1,     1,     4,     1,     3,
       3,     3,     3,     3,     2,     3,     1,     4,     1,     1,
       3,     1,     2,     4,     3,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     3,     1,     1,     3,     0,     1,     1,     1,
       1,     0,     0,     1,     0,     3,     0,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,   114,
       0,     0,    17,   106,    61,     0,    50,     0,     0,    25,
       3,     0,     0,     6,     5,     4,    92,    93,    94,    95,
      96,   108,   109,    60,     0,     0,    45,     0,   108,     0,
     100,   101,     0,    12,   112,     0,     0,     0,     0,     0,
     116,    66,    68,     0,     0,   111,    76,    78,    79,    81,
      18,    97,    98,   107,    62,    63,    19,    64,    65,    20,
      51,    53,    55,    56,    52,    54,    57,    21,    22,    23,
      26,    29,    27,    28,    30,    31,    32,    33,    34,    36,
      35,    37,    38,    39,    40,    41,    42,    43,    44,    24,
       1,     2,     0,    58,    59,     9,     0,    86,    87,    89,
      88,    91,    90,     0,     0,     7,     8,     0,     0,     0,
     113,    13,   104,     0,   103,   111,    74,     0,     0,   117,
      16,     0,     0,     0,     0,     0,    82,     0,   110,   112,
       0,   106,     0,     0,    84,    48,     0,    46,    47,    10,
     102,    11,   115,     0,   112,     0,    75,   118,    69,    70,
      71,    72,    73,     0,    14,    80,    99,    83,    85,     0,
     105,    15,    67,    77,     0,     0,    49
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    21,    25,    22,    99,   115,   147,   148,    77,   105,
      66,    69,    50,    51,    55,    56,    57,    58,    52,   113,
      34,    60,    61,    39,    40,   123,   124,    62,    35,   139,
     121,    46,   130
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -64
static const yytype_int16 yypact[] =
{
      10,   -17,   141,   -39,   -32,     3,     3,   141,   141,   -20,
      77,   100,   -64,     3,    15,     1,    69,   -37,    23,    43,
     -64,    74,   -17,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,    21,   -64,    40,    34,   156,    57,    46,   -64,    44,
      37,   -64,    47,   -64,    50,    52,    77,   118,    52,   118,
      53,   168,   -64,   141,    52,    55,   -64,    48,   -64,   -64,
     -64,    51,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,   -64,     3,   -64,   -64,   -64,     3,   -64,   -64,   -64,
     -64,   -64,   -64,     3,    38,   -64,   -64,    58,     3,    62,
     -64,   -64,   -64,    67,    54,    55,   -64,    59,    11,    79,
     -64,   118,   118,   118,   118,   118,   -64,    73,   -64,    50,
     134,     3,    75,    80,   -64,   -64,    84,   -64,   -64,   -64,
     -64,   -64,   -64,    87,    50,   118,   -64,   -64,    85,    85,
      49,   105,   -64,   134,   -64,   -64,   -64,   -64,   -64,    98,
     -64,   -64,   168,   -64,    91,    96,   -64
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -64,   -64,   120,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,   -64,   106,   -46,   -64,    -9,   -64,    17,    -2,   -64,
     -64,   -64,   -64,    -4,   -64,   -35,   -64,    14,     2,    33,
     -63,   -64,   -64
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      33,   126,    42,   128,    67,    43,    44,    41,    41,    59,
      36,     1,    23,   127,    78,    63,     2,    37,    64,   137,
      65,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    24,    17,    18,    19,    20,
      45,   131,   132,   133,   134,   135,    80,   103,   104,    81,
      68,   136,    38,    32,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,   156,    96,
      97,    98,    70,    79,   100,    71,   164,   102,    72,    73,
      74,   145,   134,   135,    75,   158,   159,   160,   161,   162,
     106,   171,   146,   114,    76,   116,   118,   117,   142,   120,
     119,   122,   143,   129,    41,   138,   140,   149,    41,   172,
     141,   151,    47,   153,   155,   144,   132,   133,   134,   135,
     150,    26,    27,    28,    29,    30,    31,    32,   152,   157,
     163,    48,   167,    49,   169,    53,   170,   168,    59,   135,
     174,   175,   101,    63,    26,    27,    28,    29,    30,    31,
      32,   176,   125,    47,   173,   166,    54,   165,   154,     0,
       0,    59,    26,    27,    28,    29,    30,    31,    32,    53,
       0,     0,     0,     0,    49,     0,     0,     0,    26,    27,
      28,    29,    30,    31,    32,    26,    27,    28,    29,    30,
      31,    32,   107,   108,   109,   110,   111,   112,   131,   132,
     133,   134,   135
};

static const yytype_int16 yycheck[] =
{
       2,    47,     6,    49,     3,     7,     8,     5,     6,    11,
      49,     1,    29,    48,    51,    13,     6,    49,     3,    54,
       5,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    52,    26,    27,    28,    29,
      60,    30,    31,    32,    33,    34,     3,     7,     8,     6,
      49,    53,    49,    50,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    57,    26,
      27,    28,     3,    50,     0,     6,   139,    56,     9,    10,
      11,    43,    33,    34,    15,   131,   132,   133,   134,   135,
      56,   154,    54,    36,    25,    49,    59,    53,   102,    49,
      53,    49,   106,    50,   102,    50,    58,    49,   106,   155,
      59,    49,    35,    59,    55,   113,    31,    32,    33,    34,
     118,    44,    45,    46,    47,    48,    49,    50,    61,    50,
      57,    54,    57,    56,    50,    35,    49,    57,   140,    34,
      42,    50,    22,   141,    44,    45,    46,    47,    48,    49,
      50,    55,    46,    35,   163,   141,    56,   140,   125,    -1,
      -1,   163,    44,    45,    46,    47,    48,    49,    50,    35,
      -1,    -1,    -1,    -1,    56,    -1,    -1,    -1,    44,    45,
      46,    47,    48,    49,    50,    44,    45,    46,    47,    48,
      49,    50,    36,    37,    38,    39,    40,    41,    30,    31,
      32,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     6,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    26,    27,    28,
      29,    63,    65,    29,    52,    64,    44,    45,    46,    47,
      48,    49,    50,    80,    82,    90,    49,    49,    49,    85,
      86,    90,    85,    80,    80,    60,    93,    35,    54,    56,
      74,    75,    80,    35,    56,    76,    77,    78,    79,    80,
      83,    84,    89,    90,     3,     5,    72,     3,    49,    73,
       3,     6,     9,    10,    11,    15,    25,    70,    51,    50,
       3,     6,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    26,    27,    28,    66,
       0,    64,    56,     7,     8,    71,    56,    36,    37,    38,
      39,    40,    41,    81,    36,    67,    49,    53,    59,    53,
      49,    92,    49,    87,    88,    74,    75,    87,    75,    50,
      94,    30,    31,    32,    33,    34,    80,    87,    50,    91,
      58,    59,    85,    85,    90,    43,    54,    68,    69,    49,
      90,    49,    61,    59,    91,    55,    57,    50,    75,    75,
      75,    75,    75,    57,    92,    79,    89,    57,    57,    50,
      49,    92,    75,    77,    42,    50,    55
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

/* User initialization code.  */

/* Line 1251 of yacc.c  */
//#line 601 "yacc.y"
{
  yyargs.size = 0;
  yylits.size = 0;
  yyerrflg = false;
  //yylloc.first_line = yylloc.last_line = 1;
  //yylloc.first_column = yylloc.last_column = 0;
 }

/* Line 1251 of yacc.c  */
//#line 1961 "yacc.tab.c"

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1464 of yacc.c  */
//#line 624 "yacc.y"
    {if (yyerrflg) {yyerrflg=false; YYABORT;} else {YYACCEPT;};}
    break;

  case 3:

/* Line 1464 of yacc.c  */
//#line 625 "yacc.y"
    {yy_quit(); YYACCEPT;;}
    break;

  case 4:

/* Line 1464 of yacc.c  */
//#line 626 "yacc.y"
    {yyerrflg=false; YYABORT;;}
    break;

  case 7:

/* Line 1464 of yacc.c  */
//#line 631 "yacc.y"
    {yy_sort_decl((yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].sortdef));;}
    break;

  case 8:

/* Line 1464 of yacc.c  */
//#line 632 "yacc.y"
    {yy_subsort_decl((yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str));;}
    break;

  case 9:

/* Line 1464 of yacc.c  */
//#line 633 "yacc.y"
    {yy_pred_decl((yyvsp[(2) - (3)].atom), (yyvsp[(3) - (3)].bval));;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
//#line 634 "yacc.y"
    {yy_const_decl((yyvsp[(2) - (4)].strs), (yyvsp[(4) - (4)].str));;}
    break;

  case 11:

/* Line 1464 of yacc.c  */
//#line 635 "yacc.y"
    {yy_var_decl((yyvsp[(2) - (4)].strs), (yyvsp[(4) - (4)].str));;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
//#line 636 "yacc.y"
    {yy_atom_decl((yyvsp[(2) - (2)].atom));;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
//#line 637 "yacc.y"
    {yy_assert_decl((yyvsp[(2) - (3)].atom), (yyvsp[(3) - (3)].str));;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
//#line 638 "yacc.y"
    {yy_add_decl((yyvsp[(2) - (4)].clause), (yyvsp[(3) - (4)].str), (yyvsp[(4) - (4)].str));;}
    break;

  case 15:

/* Line 1464 of yacc.c  */
//#line 640 "yacc.y"
    {yy_add_fdecl((yyvsp[(2) - (5)].strs), (yyvsp[(3) - (5)].formula), (yyvsp[(4) - (5)].str), (yyvsp[(5) - (5)].str));;}
    break;

  case 16:

/* Line 1464 of yacc.c  */
//#line 641 "yacc.y"
    {yy_ask_fdecl((yyvsp[(2) - (3)].formula), (yyvsp[(3) - (3)].strs));;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
//#line 642 "yacc.y"
    {yy_mcsat_decl();;}
    break;

  case 18:

/* Line 1464 of yacc.c  */
//#line 643 "yacc.y"
    {yy_mcsat_params_decl((yyvsp[(2) - (2)].strs));;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
//#line 644 "yacc.y"
    {yy_reset((yyvsp[(2) - (2)].ival));;}
    break;

  case 20:

/* Line 1464 of yacc.c  */
//#line 645 "yacc.y"
    {yy_retract((yyvsp[(2) - (2)].str));;}
    break;

  case 21:

/* Line 1464 of yacc.c  */
//#line 646 "yacc.y"
    {yy_dumptables((yyvsp[(2) - (2)].ival));;}
    break;

  case 22:

/* Line 1464 of yacc.c  */
//#line 647 "yacc.y"
    {yy_load((yyvsp[(2) - (2)].str));;}
    break;

  case 23:

/* Line 1464 of yacc.c  */
//#line 648 "yacc.y"
    {yy_verbosity((yyvsp[(2) - (2)].str));;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
//#line 650 "yacc.y"
    {yy_help((yyvsp[(2) - (2)].ival));;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
//#line 653 "yacc.y"
    {(yyval.ival) = ALL;;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
//#line 653 "yacc.y"
    {(yyval.ival) = ALL;;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
//#line 654 "yacc.y"
    {(yyval.ival) = SORT;;}
    break;

  case 28:

/* Line 1464 of yacc.c  */
//#line 654 "yacc.y"
    {(yyval.ival) = SUBSORT;;}
    break;

  case 29:

/* Line 1464 of yacc.c  */
//#line 654 "yacc.y"
    {(yyval.ival) = PREDICATE;;}
    break;

  case 30:

/* Line 1464 of yacc.c  */
//#line 655 "yacc.y"
    {(yyval.ival) = CONSTD;;}
    break;

  case 31:

/* Line 1464 of yacc.c  */
//#line 655 "yacc.y"
    {(yyval.ival) = VAR;;}
    break;

  case 32:

/* Line 1464 of yacc.c  */
//#line 655 "yacc.y"
    {(yyval.ival) = ATOMD;;}
    break;

  case 33:

/* Line 1464 of yacc.c  */
//#line 656 "yacc.y"
    {(yyval.ival) = ASSERT;;}
    break;

  case 34:

/* Line 1464 of yacc.c  */
//#line 656 "yacc.y"
    {(yyval.ival) = ADD;;}
    break;

  case 35:

/* Line 1464 of yacc.c  */
//#line 656 "yacc.y"
    {(yyval.ival) = ADD_CLAUSE;;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
//#line 657 "yacc.y"
    {(yyval.ival) = ASK;;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
//#line 659 "yacc.y"
    {(yyval.ival) = MCSAT;;}
    break;

  case 38:

/* Line 1464 of yacc.c  */
//#line 659 "yacc.y"
    {(yyval.ival) = MCSAT_PARAMS;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
//#line 660 "yacc.y"
    {(yyval.ival) = RESET;;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
//#line 660 "yacc.y"
    {(yyval.ival) = RETRACT;;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
//#line 660 "yacc.y"
    {(yyval.ival) = DUMPTABLE;;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
//#line 661 "yacc.y"
    {(yyval.ival) = LOAD;;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
//#line 661 "yacc.y"
    {(yyval.ival) = VERBOSITY;;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
//#line 661 "yacc.y"
    {(yyval.ival) = HELP;;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
//#line 664 "yacc.y"
    {(yyval.sortdef) = NULL;;}
    break;

  case 46:

/* Line 1464 of yacc.c  */
//#line 665 "yacc.y"
    {(yyval.sortdef) = (yyvsp[(2) - (2)].sortdef);;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
//#line 667 "yacc.y"
    {(yyval.sortdef) = (yyvsp[(1) - (1)].sortdef);;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
//#line 667 "yacc.y"
    {(yyval.sortdef) = yy_sortdef("MIN","MAX");;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
//#line 669 "yacc.y"
    {(yyval.sortdef) = yy_sortdef((yyvsp[(2) - (5)].str), (yyvsp[(4) - (5)].str));;}
    break;

  case 50:

/* Line 1464 of yacc.c  */
//#line 671 "yacc.y"
    {(yyval.ival) = ALL;;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
//#line 671 "yacc.y"
    {(yyval.ival) = ALL;;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
//#line 672 "yacc.y"
    {(yyval.ival) = SORT;;}
    break;

  case 53:

/* Line 1464 of yacc.c  */
//#line 672 "yacc.y"
    {(yyval.ival) = PREDICATE;;}
    break;

  case 54:

/* Line 1464 of yacc.c  */
//#line 672 "yacc.y"
    {(yyval.ival) = ATOMD;;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
//#line 673 "yacc.y"
    {(yyval.ival) = CLAUSE;;}
    break;

  case 56:

/* Line 1464 of yacc.c  */
//#line 673 "yacc.y"
    {(yyval.ival) = RULE;;}
    break;

  case 57:

/* Line 1464 of yacc.c  */
//#line 673 "yacc.y"
    {(yyval.ival) = SUMMARY;;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
//#line 676 "yacc.y"
    {(yyval.bval) = true;;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
//#line 676 "yacc.y"
    {(yyval.bval) = false;;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
//#line 677 "yacc.y"
    {(yyval.bval) = true;;}
    break;

  case 61:

/* Line 1464 of yacc.c  */
//#line 679 "yacc.y"
    {(yyval.ival) = ALL;;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
//#line 679 "yacc.y"
    {(yyval.ival) = ALL;;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
//#line 679 "yacc.y"
    {(yyval.ival) = PROBABILITIES;;}
    break;

  case 64:

/* Line 1464 of yacc.c  */
//#line 681 "yacc.y"
    {(yyval.str) = "ALL";;}
    break;

  case 66:

/* Line 1464 of yacc.c  */
//#line 683 "yacc.y"
    {(yyval.formula) = yy_formula(NULL, (yyvsp[(1) - (1)].fmla));;}
    break;

  case 67:

/* Line 1464 of yacc.c  */
//#line 684 "yacc.y"
    {(yyval.formula) = yy_formula((yyvsp[(2) - (4)].strs), (yyvsp[(4) - (4)].fmla));;}
    break;

  case 68:

/* Line 1464 of yacc.c  */
//#line 687 "yacc.y"
    {(yyval.fmla) = yy_atom_to_fmla((yyvsp[(1) - (1)].atom));;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
//#line 688 "yacc.y"
    {(yyval.fmla) = yy_fmla(IFF, (yyvsp[(1) - (3)].fmla), (yyvsp[(3) - (3)].fmla));;}
    break;

  case 70:

/* Line 1464 of yacc.c  */
//#line 689 "yacc.y"
    {(yyval.fmla) = yy_fmla(IMPLIES, (yyvsp[(1) - (3)].fmla), (yyvsp[(3) - (3)].fmla));;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
//#line 690 "yacc.y"
    {(yyval.fmla) = yy_fmla(OR, (yyvsp[(1) - (3)].fmla), (yyvsp[(3) - (3)].fmla));;}
    break;

  case 72:

/* Line 1464 of yacc.c  */
//#line 691 "yacc.y"
    {(yyval.fmla) = yy_fmla(XOR, (yyvsp[(1) - (3)].fmla), (yyvsp[(3) - (3)].fmla));;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
//#line 692 "yacc.y"
    {(yyval.fmla) = yy_fmla(AND, (yyvsp[(1) - (3)].fmla), (yyvsp[(3) - (3)].fmla));;}
    break;

  case 74:

/* Line 1464 of yacc.c  */
//#line 693 "yacc.y"
    {(yyval.fmla) = yy_fmla(NOT, (yyvsp[(2) - (2)].fmla), NULL);;}
    break;

  case 75:

/* Line 1464 of yacc.c  */
//#line 694 "yacc.y"
    {(yyval.fmla) = (yyvsp[(2) - (3)].fmla);;}
    break;

  case 76:

/* Line 1464 of yacc.c  */
//#line 697 "yacc.y"
    {(yyval.clause) = yy_clause(NULL, (yyvsp[(1) - (1)].lits));;}
    break;

  case 77:

/* Line 1464 of yacc.c  */
//#line 699 "yacc.y"
    {(yyval.clause) = yy_clause((yyvsp[(2) - (4)].strs), (yyvsp[(4) - (4)].lits));;}
    break;

  case 78:

/* Line 1464 of yacc.c  */
//#line 701 "yacc.y"
    {(yyval.lits) = copy_yylits();;}
    break;

  case 79:

/* Line 1464 of yacc.c  */
//#line 703 "yacc.y"
    {pvector_push(&yylits, (yyvsp[(1) - (1)].lit));;}
    break;

  case 80:

/* Line 1464 of yacc.c  */
//#line 704 "yacc.y"
    {pvector_push(&yylits, (yyvsp[(3) - (3)].lit));;}
    break;

  case 81:

/* Line 1464 of yacc.c  */
//#line 707 "yacc.y"
    {(yyval.lit) = yy_literal(0,(yyvsp[(1) - (1)].atom));;}
    break;

  case 82:

/* Line 1464 of yacc.c  */
//#line 708 "yacc.y"
    {(yyval.lit) = yy_literal(1,(yyvsp[(2) - (2)].atom));;}
    break;

  case 83:

/* Line 1464 of yacc.c  */
//#line 711 "yacc.y"
    {(yyval.atom) = yy_atom((yyvsp[(1) - (4)].str), (yyvsp[(3) - (4)].strs), 0);;}
    break;

  case 84:

/* Line 1464 of yacc.c  */
//#line 712 "yacc.y"
    {pvector_push(&yyargs,(yyvsp[(1) - (3)].str));
                   pvector_push(&yyargs,(yyvsp[(3) - (3)].str));
                   (yyval.atom) = yy_atom("", copy_yyargs(), (yyvsp[(2) - (3)].ival));;}
    break;

  case 85:

/* Line 1464 of yacc.c  */
//#line 715 "yacc.y"
    {(yyval.atom) = yy_atom("", (yyvsp[(3) - (4)].strs), (yyvsp[(1) - (4)].ival));;}
    break;

  case 86:

/* Line 1464 of yacc.c  */
//#line 718 "yacc.y"
    {(yyval.ival) = EQ;;}
    break;

  case 87:

/* Line 1464 of yacc.c  */
//#line 718 "yacc.y"
    {(yyval.ival) = NEQ;;}
    break;

  case 88:

/* Line 1464 of yacc.c  */
//#line 718 "yacc.y"
    {(yyval.ival) = LT;;}
    break;

  case 89:

/* Line 1464 of yacc.c  */
//#line 718 "yacc.y"
    {(yyval.ival) = LE;;}
    break;

  case 90:

/* Line 1464 of yacc.c  */
//#line 719 "yacc.y"
    {(yyval.ival) = GT;;}
    break;

  case 91:

/* Line 1464 of yacc.c  */
//#line 719 "yacc.y"
    {(yyval.ival) = GE;;}
    break;

  case 92:

/* Line 1464 of yacc.c  */
//#line 721 "yacc.y"
    {(yyval.ival) = PLUS;;}
    break;

  case 93:

/* Line 1464 of yacc.c  */
//#line 721 "yacc.y"
    {(yyval.ival) = MINUS;;}
    break;

  case 94:

/* Line 1464 of yacc.c  */
//#line 721 "yacc.y"
    {(yyval.ival) = TIMES;;}
    break;

  case 95:

/* Line 1464 of yacc.c  */
//#line 722 "yacc.y"
    {(yyval.ival) = DIV;;}
    break;

  case 96:

/* Line 1464 of yacc.c  */
//#line 722 "yacc.y"
    {(yyval.ival) = REM;;}
    break;

  case 97:

/* Line 1464 of yacc.c  */
//#line 724 "yacc.y"
    {(yyval.strs) = copy_yyargs();;}
    break;

  case 98:

/* Line 1464 of yacc.c  */
//#line 726 "yacc.y"
    {pvector_push(&yyargs,(yyvsp[(1) - (1)].str));;}
    break;

  case 99:

/* Line 1464 of yacc.c  */
//#line 727 "yacc.y"
    {pvector_push(&yyargs,(yyvsp[(3) - (3)].str));;}
    break;

  case 100:

/* Line 1464 of yacc.c  */
//#line 730 "yacc.y"
    {(yyval.strs) = copy_yyargs();;}
    break;

  case 101:

/* Line 1464 of yacc.c  */
//#line 732 "yacc.y"
    {pvector_push(&yyargs,(yyvsp[(1) - (1)].str));;}
    break;

  case 102:

/* Line 1464 of yacc.c  */
//#line 733 "yacc.y"
    {pvector_push(&yyargs,(yyvsp[(3) - (3)].str));;}
    break;

  case 103:

/* Line 1464 of yacc.c  */
//#line 736 "yacc.y"
    {(yyval.strs) = copy_yyargs();;}
    break;

  case 104:

/* Line 1464 of yacc.c  */
//#line 738 "yacc.y"
    {pvector_push(&yyargs, (yyvsp[(1) - (1)].str));;}
    break;

  case 105:

/* Line 1464 of yacc.c  */
//#line 739 "yacc.y"
    {pvector_push(&yyargs, (yyvsp[(3) - (3)].str));;}
    break;

  case 106:

/* Line 1464 of yacc.c  */
//#line 742 "yacc.y"
    {(yyval.str)=NULL;;}
    break;

  case 111:

/* Line 1464 of yacc.c  */
//#line 746 "yacc.y"
    {(yyval.str) = "DBL_MAX";;}
    break;

  case 112:

/* Line 1464 of yacc.c  */
//#line 748 "yacc.y"
    {(yyval.str)=NULL;;}
    break;

  case 114:

/* Line 1464 of yacc.c  */
//#line 750 "yacc.y"
    {(yyval.strs)=NULL;;}
    break;

  case 115:

/* Line 1464 of yacc.c  */
//#line 751 "yacc.y"
    {(yyval.strs) = (yyvsp[(2) - (3)].strs);;}
    break;

  case 116:

/* Line 1464 of yacc.c  */
//#line 754 "yacc.y"
    {(yyval.strs)=NULL;;}
    break;

  case 117:

/* Line 1464 of yacc.c  */
//#line 755 "yacc.y"
    {pvector_push(&yyargs, (yyvsp[(1) - (1)].str)); (yyval.strs) = copy_yyargs();;}
    break;

  case 118:

/* Line 1464 of yacc.c  */
//#line 756 "yacc.y"
    {pvector_push(&yyargs, (yyvsp[(1) - (2)].str)); pvector_push(&yyargs, (yyvsp[(2) - (2)].str));
                (yyval.strs) = copy_yyargs();;}
    break;



/* Line 1464 of yacc.c  */
//#line 2914 "yacc.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1684 of yacc.c  */
//#line 761 "yacc.y"


int isidentchar(int c) {
  return !(isspace(c) || iscntrl(c) || c =='(' || c == ')'
	   || c == '|' || c == '[' || c == ']' || c == '{'
	   || c == '}' || c == ',' || c == ';' || c == ':');
}

int yygetc(parse_input_t *input) {
  int c;
  if (input->kind == INFILE) {
    return getc(input->input.in_file.fps);
  } else {
    c = input->input.in_string.string[input->input.in_string.index++];
    return c == '\0' ? EOF : c;
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
  int c, cc;
  int32_t i;
  char *nstr;
  static bool ddot_wait = false;

  if (ddot_wait) {
    ddot_wait = false;
    return DDOT;
  }
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
	if (have_dot) {
	  yyerror("Number has two decimal points");
	} else {
	  // Do we have '..'?
	  if ((cc = yygetc(parse_input)) == '.') {
	    if (i == 0) {
	      return DDOT;
	    } else {
	      ddot_wait = true;
	      break;
	    }
	  } else {
	    yyungetc(cc, parse_input);
	  }
	  have_dot = 1;
	}
      };
      yystrbuf_add(c, i++);
      c = yygetc(parse_input);
      ++yylloc.last_column;
    } while (c != EOF && (isdigit(c) || c == '.'));
    yystrbuf[i] = '\0';
    if (! ddot_wait) {
      yyungetc(c, parse_input);
      --yylloc.last_column;
    }
    yylval.str = yystrbuf;
    nstr = (char *) safe_malloc((strlen(yylval.str)+1) * sizeof(char));
    strcpy(nstr, yylval.str);
    yylval.str = nstr;
    if (! have_digit) {
      if (strlen(yylval.str) == 1) {
	switch (yylval.str[0]) {
	case '+': return PLUS;
	case '-': return MINUS;
	}
      }
      safe_free(nstr);
      yyerror("Syntax error: '.', '+', '-' must be followed by parens or digits");
    } else {
      return NUM;
    }
  }
  if (isalpha(c)) {
    do {
      yystrbuf_add(c, i++);
      c = yygetc(parse_input);
      ++yylloc.last_column;      
    } while (c != EOF && isidentchar(c));
    yystrbuf[i] = '\0';
    yyungetc(c, parse_input);
    --yylloc.last_column;
    yylval.str = yystrbuf;
    if (strcasecmp(yylval.str, "PREDICATE") == 0)
      return PREDICATE;
    else if (strcasecmp(yylval.str, "OBSERVABLE") == 0
	     || strcasecmp(yylval.str, "DIRECT") == 0)
      return DIRECT;
    else if (strcasecmp(yylval.str, "HIDDEN") == 0
	     || strcasecmp(yylval.str, "INDIRECT") == 0)
      return INDIRECT;
    else if (strcasecmp(yylval.str, "SORT") == 0)
      return SORT;
    else if (strcasecmp(yylval.str, "SUBSORT") == 0)
      return SUBSORT;
    else if (strcasecmp(yylval.str, "CONST") == 0)
      return CONSTD;
    else if (strcasecmp(yylval.str, "VAR") == 0)
      return VAR;
    else if (strcasecmp(yylval.str, "ATOM") == 0)
      return ATOMD;
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
    else if (strcasecmp(yylval.str, "INTEGER") == 0)
      return INTEGER;
    else if (strcasecmp(yylval.str, "SUMMARY") == 0)
      return SUMMARY;
    else if (strcasecmp(yylval.str, "LOAD") == 0)
      return LOAD;
    else if (strcasecmp(yylval.str, "VERBOSITY") == 0)
      return VERBOSITY;
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
    else if (strcasecmp(yylval.str, "XOR") == 0)
      return XOR;
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
  // not isalpha(c)
  if (c ==  '=') {
    c = yygetc(parse_input);
    if (c == '>') {
      return IMPLIES;
    } else {
      yyungetc(c, parse_input);
      return EQ;
    }
  }
  if (c == '<') {
    if ((cc = yygetc(parse_input)) == '=') {
      return LE;
    } else {
      yyungetc(cc, parse_input);
      return LT;
    }
  }
  if (c == '>') {
    if ((cc = yygetc(parse_input)) == '=') {
      return GE;
    } else {
      yyungetc(cc, parse_input);
      return GT;
    }
  }
  if (c == '/') {
    if ((cc = yygetc(parse_input)) == '=') {
      return NEQ;
    } else {
      yyungetc(cc, parse_input);
      --yylloc.last_column;
      nstr = (char *) safe_malloc(2 * sizeof(char));
      strcpy(nstr, "/");
      yylval.str = nstr;
      return DIV;
    }
  }
  if (c == '*') {
    return TIMES;
  }
  if (c == '%') {
    return REM;
  }
  if (c == '\"') {
    // At the moment, escapes not recognized
    do {
      c = yygetc(parse_input);
      ++yylloc.last_column;      
      if (c == '\"' || c == EOF) {
        break;
      } else {
        yystrbuf_add(c, i++);
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
    printf("foo");
    do {
      c = yygetc(parse_input);
      ++yylloc.last_column;      
      if (c == '\'' || c == EOF) {
        break;
      } else {
        yystrbuf_add(c, i++);
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
    if ((cc = yygetc(parse_input)) == '=') {
      return NEQ;
    } else {
      yyungetc(cc, parse_input);
      --yylloc.last_column;
      return NOT;
    }
  }
  if (isspace(c) || c == ',' ||  c == ';' || c == '(' || c == ')' || c == '[' || c == ']'
      || c == '{' || c == '}' || c == ':' || c == '\0' || c == EOF) {
    return c;
  }
  /* If we get here, simply collect until space, comma, paren, bracket, semicolon, \0, EOF
     and return a NAME */
  yystrbuf_add(c, i++);
  do {
    c = yygetc(parse_input);
    ++yylloc.last_column;      
    if (isspace(c) || c == ',' || c == ';' || c == '(' || c == ')' || c == '[' || c == ']'
	|| c == ':' || c == '\0' || c == EOF) {
      break;
    } else {
      yystrbuf_add(c, i++);
    }
  } while (true);
  yyungetc(c, parse_input);
  --yylloc.last_column;
  yystrbuf[i] = '\0';
  nstr = (char *) safe_malloc((strlen(yystrbuf)+1) * sizeof(char));
  strcpy(nstr, yystrbuf);
  yylval.str = nstr;
  //printf("returning %s\n", nstr);
  //fflush(stdout);
  return NAME;
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
  case CONSTD: {
    free_const_decl_data();
    break;
  }
  case VAR: {
    free_var_decl_data();
    break;
  }
  case ATOMD: {
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

