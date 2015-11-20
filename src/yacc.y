/* Parser for standalone PCE */

%{
#include <ctype.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "memalloc.h"
#include "samplesat.h"
#include "mcsat.h"
#include "input.h"
#include "parser.h"
#include "vectors.h"
#include "tables.h"
#include "weight_learning.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YYINCLUDED_STDLIB_H 1
//#define YYLTYPE_IS_TRIVIAL 0
#define TPARAMS 1

extern void free_parse_data();

static bool yyerrflg = false;

// These are buffers for holding things until they are malloc'ed
#define INIT_YYSTRBUF_SIZE 200
uint32_t yystrbuf_size = INIT_YYSTRBUF_SIZE;
char *yystrbuf = NULL;
static pvector_t yyargs;
static pvector_t yylits;
parse_input_t *parse_input;

void yyerror (const char *str);

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
      arg[i] = yyargs.data[i];
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

double yy_get_float(char *str) {
  char *end;
  double val;
  val = strtod(str, &end);
  if (end == str || end[0] != '\0') {
    yyerror("Invalid floating point");
  }
  return val;
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
#if 0
  input_atom_t *atom;
  
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  atom->pred = pred;
  atom->args = args;
  atom->builtinop = builtinop;
  return atom;
#else
  return make_atom(pred, args, builtinop);
#endif
};

input_formula_t *yy_formula (char **vars, input_fmla_t *fmla) {
#if 0
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
#else
  return make_formula(vars, fmla);
#endif
};

input_fmla_t *yy_fmla (int32_t op, input_fmla_t *arg1, input_fmla_t *arg2) {
#if 0
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
#else
  return make_fmla(op, arg1, arg2);
#endif
};

input_fmla_t *yy_atom_to_fmla (input_atom_t *atom) {
#if 0
  input_fmla_t *fmla;
  input_ufmla_t *ufmla;

  fmla = (input_fmla_t *) safe_malloc(sizeof(input_fmla_t));
  ufmla = (input_ufmla_t *) safe_malloc(sizeof(input_ufmla_t));
  fmla->atomic = true;
  fmla->ufmla = ufmla;
  ufmla->atom = atom;
  return fmla;
#else
  return atom_to_fmla(atom);
#endif
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

/*
 * What if we want 'wt' to be an arithmetic expression?  Should we
 * calculate as we parse?  Seems like the right thing...
 */
void yy_add_fdecl (char **frozen, input_formula_t *formula, double wt, char *source) {
  input_command.kind = ADD;
  input_command.decl.add_fdecl.frozen = frozen;
  input_command.decl.add_fdecl.formula = formula;
  input_command.decl.add_fdecl.source = source;
  input_command.decl.add_fdecl.weight = wt;
/*
  if (wt == DBL_MAX)
    fprintf(stderr, "yy_add_decl wt = DBL_MAX\n");
  else
    fprintf(stderr, "yy_add_decl wt = %f\n", wt);
*/
#if 0
  if (strcmp(wt, "DBL_MAX") == 0) {
    input_command.decl.add_fdecl.weight = DBL_MAX;
  } else {
    /* Here, we should introduce expressions: */
    input_command.decl.add_fdecl.weight = strtod(wt, NULL);
    safe_free(wt);
  }
#endif
};

// for weight learning
 void yy_learn_fdecl (char **frozen, input_formula_t *formula, char *prob, char *source) {
  input_command.kind = LEARN;
  input_command.decl.add_fdecl.frozen = frozen;
  input_command.decl.add_fdecl.formula = formula;
  input_command.decl.add_fdecl.source = source;
  if (strcmp(prob, "NA") == 0) {
    input_command.decl.add_fdecl.weight = NOT_AVAILABLE;
  } else {
    input_command.decl.add_fdecl.weight = strtod(prob, NULL);
    safe_free(prob);
  }
};
  
//void yy_add_decl (input_clause_t *clause, char *wt, char *source) {
void yy_add_decl (input_clause_t *clause, double wt, char *source) {
  input_command.kind = ADD_CLAUSE;
  input_command.decl.add_decl.clause = clause;
  input_command.decl.add_decl.source = source;
/*
  if (wt == DBL_MAX)
    fprintf(stderr, "yy_add_decl wt = DBL_MAX\n");
  else
    fprintf(stderr, "yy_add_decl wt = %f\n", wt);
*/
//  if (strcmp(wt, "DBL_MAX") == 0) {
  input_command.decl.add_decl.weight = wt;
//  } else {
//    input_command.decl.add_decl.weight = atof(wt);
//    safe_free(wt);
//  }
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
  
//void yy_ask_decl (input_clause_t *clause, char *thresholdstr, char *allstr, char *numsamp) {
//  double threshold;
//  input_command.kind = ASK_CLAUSE;
//  input_command.decl.ask_decl.clause = clause;
//  threshold = atof(thresholdstr);
//  if (0.0 <= threshold && threshold <= 1.0) {
//    input_command.decl.ask_decl.threshold = threshold;
//  } else {
//    free_clause(clause);
//    safe_free(thresholdstr);
//    safe_free(allstr);
//    safe_free(numsamp);
//    yyerror("ask_clause: threshold must be between 0.0 and 1.0");
//    return;
//  }
//  safe_free(thresholdstr);
//  if (allstr == NULL || strcasecmp(allstr, "BEST") == 0) {
//    input_command.decl.ask_decl.all = false;
//  } else if (strcasecmp(allstr, "ALL") == 0) {
//    input_command.decl.ask_decl.all = true;
//  } else {
//    free_clause(clause);
//    safe_free(allstr);
//    safe_free(numsamp);
//    yyerror("ask_clause: which must be 'all' or 'best'");
//    return;
//  }
//  safe_free(allstr);
//  if (numsamp == NULL) {
//    input_command.decl.ask_decl.num_samples = DEFAULT_MAX_SAMPLES;
//  } else if (yy_check_nat(numsamp)) {
//    input_command.decl.ask_decl.num_samples = atoi(numsamp);
//  } else {
//    free_clause(clause);
//    safe_free(numsamp);
//    yyerror("ask_clause: numsamps must be an integer");
//  }
//  safe_free(numsamp);
//};

/* For weight learning */
void yy_train_decl (int32_t alg, char *file) {
  input_command.kind = TRAIN;
  input_command.decl.train_decl.alg = alg;
  input_command.decl.train_decl.file = file;
}
  
/* The params map to sa_probability, sa_temperature, rvar_probability,
 * max_flips, max_extra_flips, and max_samples, in that order.  Missing
 * args get default values.
 */
void yy_mcsat_decl () {
  //yy_get_mcsat_params(params);
  input_command.kind = MCSAT;
}

/*
 * Look for legal set variables and initialize input_command appropriately:
 */

void yy_set_decl (char *name, char * v) {
  int k;
  input_command.kind = SET;
  //  output(" name = %s\n", name);
  k = -1;
  if (strcasecmp(name, "MAX_SAMPLES") == 0)
    k = MAX_SAMPLES;
  else if (strcasecmp(name, "SA_PROBABILITY") == 0)
    k = SA_PROBABILITY;
  else if (strcasecmp(name, "SA_TEMPERATURE") == 0)
    k = SA_TEMPERATURE;
  else if (strcasecmp(name, "RVAR_PROBABILITY") == 0)
    k = RVAR_PROBABILITY;
  else if (strcasecmp(name, "MAX_FLIPS") == 0)
    k = MAX_FLIPS;
  else if (strcasecmp(name, "MAX_EXTRA_FLIPS") == 0)
    k = MAX_EXTRA_FLIPS;
  else if (strcasecmp(name, "MCSAT_TIMEOUT") == 0)
    k = MCSAT_TIMEOUT;
  else if (strcasecmp(name, "BURN_IN_STEPS") == 0)
    k = BURN_IN_STEPS;
  else if (strcasecmp(name, "SAMP_INTERVAL") == 0)
    k = SAMP_INTERVAL;
  else if (strcasecmp(name, "MAX_TRAINING_ITER") == 0)
    k = MAX_TRAINING_ITER;
  else if (strcasecmp(name, "MIN_ERROR") == 0)
    k = MIN_ERROR;
  else if (strcasecmp(name, "LEARNING_RATE") == 0)
    k = LEARNING_RATE;
  else if (strcasecmp(name, "REPORT_RATE") == 0)
    k = REPORT_RATE;
  else {
    printf("set: Unrecognized variable name: %s\n", name);
    yyerror("bad set command, value");
  }
  /* else what? */
  input_command.decl.set_decl.param = k;
  input_command.decl.set_decl.value = yy_get_float(v);
}

void yy_mcsat_params_decl (char **params) {
  int32_t arglen = 0;
  
  input_command.kind = MCSAT_PARAMS;
  /* Determine num_params */
  if (params != NULL) {
    while (arglen <= 9 && (params[arglen] != NULL)) {
      arglen++;
    }
  }
  if (arglen > 9) {
    yyerror("mcsat_params: too many args");
  }
  input_command.decl.mcsat_params_decl.num_params = arglen;
  /* 0: max_samples */
  if (arglen > 0 && strcmp(params[0], "") != 0) {
    if (yy_check_nat(params[0])) {
      input_command.decl.mcsat_params_decl.max_samples = atoi(params[0]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_samples = -1;
  }
  /* 1: sa_probability */
  if (arglen > 1 && strcmp(params[1], "") != 0) {
    double prob = yy_get_float(params[1]);
    if (0.0 <= prob && prob <= 1.0) {
      input_command.decl.mcsat_params_decl.sa_probability = prob;
    } else {
    yyerror("sa_probability should be between 0.0 and 1.0");
    }
  } else {
    input_command.decl.mcsat_params_decl.sa_probability = -1;
  }
  /* 2: sa_temperature */
  if (arglen > 2 && strcmp(params[2], "") != 0) {
    double temp = yy_get_float(params[2]);
    if (temp > 0.0) {
      input_command.decl.mcsat_params_decl.sa_temperature = temp;
    } else {
      yyerror("sa_temperature should be greater than 0.0");
    }
  } else {
    input_command.decl.mcsat_params_decl.sa_temperature = -1;
  }
  /* 3: rvar_probability */
  if (arglen > 3 && strcmp(params[3], "") != 0) {
    double prob = yy_get_float(params[3]);
    if (0.0 <= prob && prob <= 1.0) {
      input_command.decl.mcsat_params_decl.rvar_probability = prob;
    } else {
      yyerror("rvar_probability should be between 0.0 and 1.0");
    }
  } else {
    input_command.decl.mcsat_params_decl.rvar_probability = -1;
  }
  /* 4: max_flips */
  if (arglen > 4 && strcmp(params[4], "") != 0) {
    if (yy_check_nat(params[4])) {
      input_command.decl.mcsat_params_decl.max_flips = atoi(params[4]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_flips = -1;
  }
  /* 5: max_extra_flips */
  if (arglen > 5 && strcmp(params[5], "") != 0) {
    if (yy_check_nat(params[5])) {
      input_command.decl.mcsat_params_decl.max_extra_flips = atoi(params[5]);
    }
  } else {
    input_command.decl.mcsat_params_decl.max_extra_flips = -1;
  }
  /* 6: timeout */
  if (arglen > 6 && strcmp(params[6], "") != 0) {
    if (yy_check_nat(params[6])) {
      input_command.decl.mcsat_params_decl.timeout = atoi(params[6]);
    }
  } else {
    input_command.decl.mcsat_params_decl.timeout = -1;
  }
  /* 7: num of burn_in_steps */
  if (arglen > 7 && strcmp(params[7], "") != 0) {
    if (yy_check_nat(params[7])) {
      input_command.decl.mcsat_params_decl.burn_in_steps = atoi(params[7]);
    }
  } else {
    input_command.decl.mcsat_params_decl.burn_in_steps = -1;
  }
  /* 8: sampling interval */
  if (arglen > 8 && strcmp(params[8], "") != 0) {
    if (yy_check_nat(params[8])) {
      input_command.decl.mcsat_params_decl.samp_interval = atoi(params[8]);
    }
  } else {
    input_command.decl.mcsat_params_decl.samp_interval = -1;
  }
  free_strings(params);
}

/* 
 * Command syntax for train_params is:
 *
 * train_params <max_iter> <stopping_err> <learning_rate> <reporting_interval> 
 *
 * How do we select gradient vs. lbfgs?  Suggest an optional arg to
 * 'train' that indicates the algorithm to use.
 */

#if TPARAMS
void yy_train_params_decl (char **params) {
  int32_t arglen = 0;
  
  input_command.kind = TRAIN_PARAMS;
  /* Determine num_params */
  if (params != NULL) {
    while (arglen <= 4 && (params[arglen] != NULL)) {
      arglen++;
    }
  }
  if (arglen > 4) {
    yyerror("train_params: too many args");
  }
  input_command.decl.train_params_decl.num_params = arglen;

  /* 0: max_iter */
  if (arglen > 0 && strcmp(params[0], "") != 0) {
    if (yy_check_nat(params[0])) {
      input_command.decl.train_params_decl.max_iter = atoi(params[0]);
    }
  } else {
    input_command.decl.train_params_decl.max_iter = -1;
  }
  /* 1: stopping_error */
  if (arglen > 1 && strcmp(params[1], "") != 0) {
    double err = yy_get_float(params[1]);
//    printf("arg was %s, err set to %f\n", params[1], err);
    if (0.0 < err && err <= 1.0) {
      input_command.decl.train_params_decl.stopping_error = err;
    } else {
    yyerror("stopping error should be between 0.0 and 1.0");
    }
  } else {
    input_command.decl.train_params_decl.stopping_error = -1;
  }
  /* 2: learning_rate */
  if (arglen > 2 && strcmp(params[2], "") != 0) {
    double rate = yy_get_float(params[2]);
//    printf("arg was %s, rate set to %f\n", params[2], rate);
    if (rate > 0.0) {
      input_command.decl.train_params_decl.learning_rate = rate;
    } else {
      yyerror("learning rate should be greater than 0.0");
    }
  } else {
    input_command.decl.train_params_decl.learning_rate = -1;
  }
  /* 3: reporting interval for weight learning - by default, every 100 iterations: */
  if (arglen > 3 && strcmp(params[3], "") != 0) {
    if (yy_check_nat(params[3])) {
      input_command.decl.train_params_decl.reporting = atoi(params[3]);
    }
  } else {
    input_command.decl.train_params_decl.reporting = -1;
  }

  free_strings(params);
};
#endif

void yy_mwsat_decl () {
  //yy_get_mcsat_params(params);
  input_command.kind = MWSAT;
}

void yy_mwsat_params_decl (char **params) {
  int32_t arglen = 0;
  
  input_command.kind = MWSAT_PARAMS;
  /* Determine num_params */
  if (params != NULL) {
    while (arglen <= 4 && (params[arglen] != NULL)) {
      arglen++;
    }
  }
  if (arglen > 4) {
    yyerror("mcsat_params: too many args");
  }
  input_command.decl.mwsat_params_decl.num_params = arglen;
  /* 0: max_trials */
  if (arglen > 0 && strcmp(params[0], "") != 0) {
    if (yy_check_nat(params[0])) {
      input_command.decl.mwsat_params_decl.num_trials = atoi(params[0]);
    }
  } else {
    input_command.decl.mwsat_params_decl.num_trials = -1;
  }
  /* 1: rvar_probability */
  if (arglen > 1 && strcmp(params[1], "") != 0) {
    double prob = yy_get_float(params[1]);
    if (0.0 <= prob && prob <= 1.0) {
      input_command.decl.mwsat_params_decl.rvar_probability = prob;
    } else {
      yyerror("rvar_probability should be between 0.0 and 1.0");
    }
  } else {
    input_command.decl.mwsat_params_decl.rvar_probability = -1;
  }
  /* 2: max_flips */
  if (arglen > 2 && strcmp(params[2], "") != 0) {
    if (yy_check_nat(params[2])) {
      input_command.decl.mwsat_params_decl.max_flips = atoi(params[2]);
    }
  } else {
    input_command.decl.mwsat_params_decl.max_flips = -1;
  }
  /* 3: timeout */
  if (arglen > 3 && strcmp(params[3], "") != 0) {
    if (yy_check_nat(params[3])) {
      input_command.decl.mwsat_params_decl.timeout = atoi(params[6]);
    }
  } else {
    input_command.decl.mwsat_params_decl.timeout = -1;
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
%token CONSTD
%token VAR
%token ATOMD
%token ASSERT
%token ADD
%token ASK
%token ADD_CLAUSE
//%token ASK_CLAUSE
%token MCSAT
%token MCSAT_PARAMS
%token TRAIN_PARAMS
%token MWSAT
%token MWSAT_PARAMS
%token RESET
%token RETRACT
%token DUMPTABLE
%token SUMMARY
%token QINST
%token LOAD
%token VERBOSITY
%token HELP
%token QUIT
%token TRAIN
%token LEARN
%token SET
%left IFF
%right IMPLIES
%left OR
%left XOR
%left AND
%nonassoc NOT
%nonassoc EQ
%nonassoc NEQ
%nonassoc LE
%nonassoc LT
%nonassoc GE
%nonassoc GT
%nonassoc DDOT
%token INTEGER
%token PLUS
%token MINUS
%token TIMES
%token DIV
%token REM
%token NAME
%token NUM
%token STRING
%token LBFGS
%token GRADIENT

%token MAX_SAMPLES
%token SA_PROBABILITY
%token SA_TEMPERATURE
%token RVAR_PROBABILITY
%token MAX_FLIPS
%token MAX_EXTRA_FLIPS
%token MCSAT_TIMEOUT
%token BURN_IN_STEPS
%token SAMP_INTERVAL
%token MAX_TRAINING_ITER
%token MIN_ERROR
%token LEARNING_RATE
%token REPORT_RATE

%token ENDNUM
%left '+' '-'
%left '*' '/'
%left EXP LN SQRT


%union
{
  bool bval;
  char *str;
  char **strs;
  struct input_sortdef_s *sortdef;
  struct input_fmla_s *fmla;
  struct input_formula_s *formula;
  struct input_clause_s *clause;
  struct input_literal_s *lit;
  struct input_literal_s **lits;
  struct input_atom_s *atom;
  int32_t ival;
  double dval;
}

%type <str> arg NAME NUM STRING oarg oname retractarg learnprob trainarg
%type <strs> arguments names oarguments onum2 ofrozen
%type <sortdef> sortdef sortval interval
%type <formula> formula
%type <fmla> fmla
%type <clause> clause
%type <lit> literal
%type <lits> literals
%type <atom> atom
%type <bval> witness
%type <ival> cmd table resetarg bop preop EQ NEQ LT LE GT GE PLUS MINUS TIMES DIV REM 
%type <dval> addwt expr

/*
%type <ival> cmd table resetarg bop preop EQ NEQ LT LE GT GE PLUS MINUS TIMES DIV REM LBFGS GRADIENT  set_param trainalg
MAX_SAMPLES SA_PROBABILITY RVAR_PROBABILITY MAX_FLIPS MAX_EXTRA_FLIPS MCSAT_TIMEOUT BURN_IN_STEPS SAMP_INTERVAL MAX_TRAINING_ITER MIN_ERROR LEARNING_RATE REPORT_RATE 
*/

%locations

%start command

%initial-action {
  yyargs.size = 0;
  yylits.size = 0;
  yyerrflg = false;
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
   LEARN clause [probability];
*/

/* Grammar */
%%

command: decl enddecl {if (yyerrflg) {yyerrflg=false; YYABORT;} else {YYACCEPT;}}
       | QUIT {yy_quit(); YYACCEPT;}
       | error enddecl {yyerrflg=false; YYABORT;}
       ;

enddecl: ';' | QUIT;

decl: SORT NAME sortdef {yy_sort_decl($2, $3);}
    | SUBSORT NAME NAME {yy_subsort_decl($2, $3);}
    | PREDICATE atom witness {yy_pred_decl($2, $3);}
    | CONSTD arguments ':' NAME {yy_const_decl($2, $4);}
    | VAR arguments ':' NAME {yy_var_decl($2, $4);}
    | ATOMD atom {yy_atom_decl($2);}
    | ASSERT atom oname {yy_assert_decl($2, $3);}
    | ADD_CLAUSE clause addwt oname {yy_add_decl($2, $3, $4);}
    | ADD_CLAUSE clause '{' expr '}'  oname {yy_add_decl($2, $4, $6);}
//    | ASK_CLAUSE clause NUM oarg oarg {yy_ask_decl($2, $3, $4, $5);}
/* Below, for the ADD production, instead of 'addwt', use a production
 * that allows arithmetic operations and can compute them on the fly.
 * This allows us to use a preprocessor to substitute different values
 * during a survey:
 */
    | ADD ofrozen formula addwt oname {yy_add_fdecl($2, $3, $4, $5);}
    | ADD ofrozen formula '{' expr '}' oname {yy_add_fdecl($2, $3, $5, $7);}
    | ASK formula onum2 {yy_ask_fdecl($2, $3);}
// Weight learning
    | LEARN ofrozen formula learnprob oname {yy_learn_fdecl($2, $3, $4, $5);}
    | TRAIN trainarg {yy_train_decl(GRADIENT, $2);}
    | TRAIN trainarg GRADIENT {yy_train_decl(GRADIENT, $2);}
    | TRAIN trainarg LBFGS {yy_train_decl(LBFGS, $2);}
    | MCSAT {yy_mcsat_decl();}
    | MCSAT_PARAMS oarguments {yy_mcsat_params_decl($2);}
    | TRAIN_PARAMS oarguments {yy_train_params_decl($2);}
    | MWSAT {yy_mwsat_decl();}
    | MWSAT_PARAMS oarguments {yy_mwsat_params_decl($2);}
    | RESET resetarg {yy_reset($2);}
    | SET NAME NUM {yy_set_decl($2, $3);}
    | RETRACT retractarg {yy_retract($2);}
    | DUMPTABLE table {yy_dumptables($2);}
    | LOAD STRING {yy_load($2);}
    | VERBOSITY NUM {yy_verbosity($2);}
// Need to reference @$ in order for locations to be generated
    | HELP cmd {yy_help($2);}
    ;

cmd: /* empty */ {$$ = ALL;} | ALL {$$ = ALL;}
     | SET {$$ = SET;}
     | SORT {$$ = SORT;} | SUBSORT {$$ = SUBSORT;} | PREDICATE {$$ = PREDICATE;}
     | CONSTD {$$ = CONSTD;} | VAR {$$ = VAR;} | ATOMD {$$ = ATOMD;}
     | ASSERT {$$ = ASSERT;} | ADD {$$ = ADD;} | ADD_CLAUSE {$$ = ADD_CLAUSE;}
     | ASK {$$ = ASK;}
     //| ASK_CLAUSE {$$ = ASK_CLAUSE;}
     | MCSAT {$$ = MCSAT;} | MCSAT_PARAMS {$$ = MCSAT_PARAMS;} 
     | MWSAT {$$ = MWSAT;} | MWSAT_PARAMS {$$ = MWSAT_PARAMS;}
     | RESET {$$ = RESET;} | RETRACT {$$ = RETRACT;} | DUMPTABLE {$$ = DUMPTABLE;}
     | LOAD {$$ = LOAD;} | VERBOSITY {$$ = VERBOSITY;} | HELP {$$ = HELP;}
     | LEARN {$$ = LEARN;} |  TRAIN {$$ = TRAIN;} | TRAIN_PARAMS {$$ = TRAIN_PARAMS;};

sortdef: /* empty */ {$$ = NULL;}
       | EQ sortval {$$ = $2;} ;

sortval: interval {$$ = $1;} | INTEGER {$$ = yy_sortdef("MIN","MAX");};

interval: '[' NUM DDOT NUM ']' {$$ = yy_sortdef($2, $4);} ;

table: /* empty */ {$$ = ALL;} | ALL {$$ = ALL;}
       | SORT {$$ = SORT;} | PREDICATE {$$ = PREDICATE;} | ATOMD {$$ = ATOMD;}
       | CLAUSE {$$ = CLAUSE;} | RULE {$$ = RULE;} | SUMMARY {$$ = SUMMARY;}
       | QINST {$$ = QINST;}
       ;

witness: DIRECT {$$ = true;} | INDIRECT {$$ = false;}
       | /* empty */ {$$ = true;};

resetarg: /* empty */ {$$ = ALL;} | ALL {$$ = ALL;} | PROBABILITIES {$$ = PROBABILITIES;};

retractarg: ALL {$$ = "ALL";} | NAME;

formula: fmla {$$ = yy_formula(NULL, $1);}
       | '[' names ']' fmla {$$ = yy_formula($2, $4);}
       ;

fmla: atom {$$ = yy_atom_to_fmla($1);}
    | fmla IFF fmla {$$ = yy_fmla(IFF, $1, $3);}
    | fmla IMPLIES fmla {$$ = yy_fmla(IMPLIES, $1, $3);}
    | fmla OR fmla {$$ = yy_fmla(OR, $1, $3);}
    | fmla XOR fmla {$$ = yy_fmla(XOR, $1, $3);}
    | fmla AND fmla {$$ = yy_fmla(AND, $1, $3);}
    | NOT fmla {$$ = yy_fmla(NOT, $2, NULL);}
    | '(' fmla ')' {$$ = $2;}
    ;

clause: literals {$$ = yy_clause(NULL, $1);};
      | '(' names ')'
        literals {$$ = yy_clause($2, $4);};

literals: lits {$$ = copy_yylits();};

lits: literal {pvector_push(&yylits, $1);}
    | lits '|' literal {pvector_push(&yylits, $3);}
    ;

literal: atom {$$ = yy_literal(0,$1);}
       | NOT atom {$$ = yy_literal(1,$2);}
       ;

atom: NAME '(' arguments ')' {$$ = yy_atom($1, $3, 0);}
    | arg bop arg {pvector_push(&yyargs,$1);
                   pvector_push(&yyargs,$3);
                   $$ = yy_atom("", copy_yyargs(), $2);}
    | preop '(' arguments ')' {$$ = yy_atom("", $3, $1);}
    ;

bop: EQ {$$ = EQ;} | NEQ {$$ = NEQ;} | LT {$$ = LT;} | LE {$$ = LE;}
   | GT {$$ = GT;} | GE {$$ = GE;} ;

preop: PLUS {$$ = PLUS;} | MINUS {$$ = MINUS;} | TIMES {$$ = TIMES;}
     | DIV {$$ = DIV;} | REM {$$ = REM;} ;

oarguments: oargs {$$ = copy_yyargs();};

oargs: oarg {pvector_push(&yyargs,$1);}
      | oargs ',' oarg {pvector_push(&yyargs,$3);}
      ;

arguments: args {$$ = copy_yyargs();};

args: arg {pvector_push(&yyargs,$1);}
      | args ',' arg {pvector_push(&yyargs,$3);}
      ;

names: nms {$$ = copy_yyargs();};
   
nms: NAME {pvector_push(&yyargs, $1);}
      | nms ',' NAME {pvector_push(&yyargs, $3);}
      ;

oarg: /*empty*/ {$$=NULL;} | arg;

arg: NAME | NUM; // returns string from yystrings

/*
 * This has to change to support arithmetic expressions:
 */

//  addwt: NUM | /* empty */ {$$ = "DBL_MAX";};

/* These productions turn weights into doubles rather than strings,
   and allow various expressions to be used to compute rule weights.
   The main idea is to allow preprocessors to perform substitutions
   that can scale a number of different weights by the same scale
   factor, or progress surveys by log probabilities instead of simply
   a linear scale:
*/

addwt: NUM { $$ = strtod($1, NULL); } | /* empty */ {$$ = DBL_MAX;};
//addwt: expr | /* empty */ {$$ = DBL_MAX;};

expr: NUM { $$ = strtod($1, NULL); }
     | expr PLUS expr { $$ = $1 + $3; } 
     | expr MINUS expr { $$ = $1 - $3; }
     | expr TIMES expr { $$ = $1 * $3; }
     | expr DIV expr { $$ = $1 / $3; }
     | EXP '(' expr ')' { $$ = exp($3); }
     | LN '(' expr ')' { $$ = log($3); }



learnprob: NUM | /* empty */ {$$ = "NA";};

oname: /* empty */ {$$=NULL;} | NAME;

ofrozen: /*empty */ {$$=NULL;}
      | '{' names '}' {$$ = $2;}
      ;

onum2: /* empty */ {$$=NULL;}
     | NUM {pvector_push(&yyargs, $1); $$ = copy_yyargs();}
     | NUM NUM {pvector_push(&yyargs, $1); pvector_push(&yyargs, $2);
                $$ = copy_yyargs();}

//onum: /* empty */ {$$=NULL;} | NUM;

trainarg: /* empty */ {$$ = NULL;} | STRING;

%%

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
        ++yylloc.last_line;
        yylloc.last_column = 0;
        if (yynerrs && (input_is_stdin(parse_input))) {
          return ';';
        }
      } else {
        ++yylloc.last_column;
      }
    } while (isspace(c));
    if (c == '#') {
      do {
        c = yygetc(parse_input);
        if (c == '\n') {
          ++yylloc.last_line;
          yylloc.last_column = 0;
        } else {
          ++yylloc.last_column;
        }
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
//    else if (strcasecmp(yylval.str, "ASK_CLAUSE") == 0)
//      return ASK_CLAUSE;
	/* Weight learning */
     else if (strcasecmp(yylval.str, "LEARN") == 0)
      return LEARN;
    else if (strcasecmp(yylval.str, "TRAIN") == 0)
      return TRAIN;
    else if (strcasecmp(yylval.str, "TRAIN_PARAMS") == 0)
      return TRAIN_PARAMS;
    else if (strcasecmp(yylval.str, "MCSAT") == 0)
      return MCSAT;
    else if (strcasecmp(yylval.str, "MCSAT_PARAMS") == 0)
      return MCSAT_PARAMS;
    else if (strcasecmp(yylval.str, "MWSAT") == 0)
      return MWSAT;
    else if (strcasecmp(yylval.str, "MWSAT_PARAMS") == 0)
      return MWSAT_PARAMS;
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
    else if (strcasecmp(yylval.str, "QINST") == 0)
          return QINST;
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
    else if (strcasecmp(yylval.str, "SET") == 0)
      return SET;
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
    else if (strcasecmp(yylval.str, "LBFGS") == 0)
      return LBFGS;
    else if (strcasecmp(yylval.str, "GRADIENT") == 0)
      return GRADIENT;
    else if (strcasecmp(yylval.str, "EXP") == 0)
      return EXP;
    else if (strcasecmp(yylval.str, "LN") == 0)
      return LN;
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
    /* At the moment, escapes not recognized */
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
    /* At the moment, escapes not recognized */
    //printf("foo");
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

// Weight learning
void free_train_decl_data() {
  safe_free(input_command.decl.train_decl.file);
}

void free_learn_decl_data() {
  free_formula(input_command.decl.add_fdecl.formula);
}

void free_mcsat_decl_data() {
  // Nothing to do here
}

void free_mcsat_params_decl_data() {
  // Nothing to do here
}

void free_train_params_decl_data() {
  // Nothing to do here
}

void free_mwsat_decl_data() {
  // Nothing to do here
}

void free_mwsat_params_decl_data() {
  // Nothing to do here
}

void free_reset_decl_data() {
  // Nothing to do here
}

void free_set_decl_data() {
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
#if TPARAMS
  case TRAIN_PARAMS: {
    free_train_params_decl_data();
    break;
  }
#endif
  case MWSAT: {
    free_mwsat_decl_data();
    break;
  }
  case MWSAT_PARAMS: {
    free_mwsat_params_decl_data();
	break;
  }
// Weight learning
  case TRAIN: {
    free_train_decl_data();
    break;
  }
   case LEARN: {
    free_learn_decl_data();
    break;
  }
  case RESET: {
    free_reset_decl_data();
    break;
  }
  case SET: {
    free_set_decl_data();
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

void yyerror (const char *str) {
  if (parse_input->kind == INFILE) {
    printf("%s:%d:%d: %s: %s\n", parse_input->input.in_file.file,
           yylloc.last_line, yylloc.last_column, str, yylval.str);
  } else {
    printf("%s:%d:%d: %s: %s\n", parse_input->input.in_string.string,
           yylloc.last_line, yylloc.last_column, str, yylval.str);
  }
  input_command.kind = 0;
  yyerrflg = true;
};

// Local variables:
// c-electric-flag: nil
// End:
