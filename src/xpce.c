#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <pthread.h>

#include <json/json.h>
#include <json/json_object_private.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

#include "xmlrpc-config.h"  /* information about this build environment */
#include "memalloc.h"
#include "vectors.h"
#include "utils.h"
#include "print.h"
#include "input.h"
#include "parser.h"
#include "cnf.h"
#include "xpce.h"
#include "SFMT.h"

extern int yyparse ();

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static xmlrpc_server_abyss_t * serverToTerminateP;

samp_table_t samp_table;

// sort-decl := {"sort": NAME, "super": NAME} # "super" is optional,
//                     # so this call can declare a sort or a subsort

// predicate-decl := {"predicate": NAME, "arguments": NAMES, "observable": BOOL}
//                     # arguments is a list of sort names.

// const-decl := {"names": NAMES, "sort": SORT}

// assert-decl := {"assert": FACT}

// add-decl := {"formula": FORMULA, "weight": NUM, "source": NAME} # weight is optional, default DBL_MAX
//                                                                 # source is also optional

// ask-decl := {"formula": FORMULA, "threshold": NUM, "maxresults": NUM}
//    # threshold optional, default 0.0, maxresults optional, default 0 (means return all instances)

// mcsat-decl := {}

// reset-decl := {"reset": RESET}

// retract-decl := {"retract": NAME}

// load-decl := {"load": STRING}

// FORMULA := ATOM |
//      {"not": FORMULA} |
//            {"and": [FORMULA, FORMULA]} |
//            {"or": [FORMULA, FORMULA]} |
//            {"implies": [FORMULA, FORMULA]} |
//            {"iff": [FORMULA, FORMULA]}

// ATOM := {"atom": {"functor": NAME, "arguments": ARGUMENTS}}

// FACT := {"fact": {"functor": NAME, "arguments": CONSTANTS}}

// RESET :=  "all" | "probabilities"

// NAMES := [NAME++',']
// ARGUMENTS := [ARGUMENT++',']
// CONSTANTS := [CONSTANT++',']
// NUM := ['+'|'-'] simple floating point number
// NAME := chars except whitespace parens ':' ',' ';'
// ARGUMENT := NAME | {"var":  NAME}


// Every call returns a JSONObject of the form:
// RETURN := {"error": ERRORMESSAGE | "warning": WARNINGMESSAGE | "result": RESULT}
// ERRORMESSAGE   := a string containing the error message
// WARNINGMESSAGE := a string containing the warning message
// RESULT := a JSONObject containing a possible return object


// In general, methods follow the following steps:
//  1. Call xmlrpc_decompose_value to get the JSON string
//  2.   Call json_tokener_parse to get the JSON object
//  3.   Check that the JSON object is as expected
//  4.   Call json_object_get_XXX to get at the pieces of the JSON object
//  5.     Invoke the corresponding MCSAT functions
//  6.   Convert results (including errors) into JSON
//  7. Convert results into a string
//  8. Convert restuls to a xmlrpc_value with xmlrpc_build_value
//  9. Return those results

//  A. Call xpce_parse_decl for 1. & 2.
//  B. 3. TBD
//  C. 

struct json_object *
xpce_parse_decl(xmlrpc_env * const envP,
		xmlrpc_value * const paramArrayP) {
  char *declstr;
  //  char *declcopy;
  struct json_object *decl;
  
  xmlrpc_decompose_value(envP, paramArrayP, "(s)", &declstr);
  if (envP->fault_occurred)
    return NULL;
  decl = json_tokener_parse(declstr);
  safe_free(declstr);
  return decl;
}

// Recursively call jsaon_object_put to free up objects
void json_object_put_all(struct json_object *obj) {
  int32_t i;
  struct json_object_iter iter;
  
  if (json_object_is_type(obj, json_type_array)) {
    for (i = 0; i < json_object_array_length(obj); i++) {
      json_object_put_all(json_object_array_get_idx(obj, i));
    }
  } else if (json_object_is_type(obj, json_type_object)) {
    json_object_object_foreachC(obj, iter) {
      json_object_put_all(iter.val);
    }
  }
  json_object_put(obj);
}

// Creates a JSON object of form
//  {"result": RESULT, "error": ERROR_STRING, "warning": WARNING_STRING}
// All entries are optional; successful results are often the empty object {}
static xmlrpc_value *
xpce_result(xmlrpc_env * const envP,
	    struct json_object *result) {
  struct json_object *ret;
  xmlrpc_value *xret;
  char *err, *warn;
  const char *retstr;
  
  ret = json_object_new_object();
  if (result != NULL) {
    json_object_object_add(ret, "result", result);
  }
  if (error_buffer.size > 0) {
    err = get_string_from_buffer(&error_buffer);
    json_object_object_add(ret, "error", json_object_new_string(err));
    safe_free(err);
  }
  if (warning_buffer.size > 0) {
    warn = get_string_from_buffer(&warning_buffer);
    json_object_object_add(ret, "warning", json_object_new_string(warn));
    safe_free(warn);
  }
  retstr = json_object_to_json_string(ret);
  xret = xmlrpc_build_value(envP, "s", retstr);
  json_object_put(ret);
  return xret;
}

static xmlrpc_value *
xpce_sort(xmlrpc_env * const envP,
	  xmlrpc_value * const paramArrayP,
	  void * const serverInfo ATTR_UNUSED,
	  void * const channelInfo ATTR_UNUSED) {
  struct json_object *sortdecl, *sortobj, *superobj;
  char *sort, *super;

  printf("In xpce.sort\n");
  sortdecl = xpce_parse_decl(envP, paramArrayP);
  // sortdecl should be of the form  {"name": NAME, "super": NAME}, where super is optional
  sortobj = json_object_object_get(sortdecl, "name");
  if (sortobj == NULL) {
    mcsat_err("Bad argument: expected {\"name\": NAME}\n");
    json_object_put(sortdecl);
    return xpce_result(envP, NULL);
  }
  sort = (char *) json_object_get_string(sortobj);
  superobj = json_object_object_get(sortdecl, "super");
  if (superobj == NULL) {
    pthread_mutex_lock(&mutex);
    fflush(stdout);
    add_sort(&samp_table.sort_table, sort);
    pthread_mutex_unlock(&mutex);
  } else {
    super = (char *) json_object_get_string(superobj);
    pthread_mutex_lock(&mutex);
    add_subsort(&samp_table.sort_table, sort, super);
    pthread_mutex_unlock(&mutex);
  }
  json_object_put(sortdecl);
  return xpce_result(envP, NULL);
}

static xmlrpc_value *
xpce_predicate(xmlrpc_env * const envP,
	       xmlrpc_value * const paramArrayP,
	       void * const serverInfo ATTR_UNUSED,
	       void * const channelInfo ATTR_UNUSED) {

  struct json_object *preddecl, *predobj, *argsobj, *arg, *observableobj;
  char *pred, **sorts;
  bool observable;
  int32_t i;
  
  printf("In xpce.predicate\n");
  preddecl = xpce_parse_decl(envP, paramArrayP);
  // should have form {"predicate": NAME, "arguments": NAMES, "observable": BOOL}
  if (envP->fault_occurred) {
    mcsat_err("Bad argument: expected {\"predicate\": NAME, \"arguments\": NAMES, \"observable\": BOOL}\n");
    json_object_put(preddecl);
    return xpce_result(envP, NULL);
  }
  if ((predobj = json_object_object_get(preddecl, "predicate")) == NULL
      || (argsobj = json_object_object_get(preddecl, "arguments")) == NULL
      || (observableobj = json_object_object_get(preddecl, "observable")) == NULL) {
    mcsat_err("Bad argument: expected {\"predicate\": NAME, \"arguments\": NAMES, \"observable\": BOOL}\n");
    json_object_put(preddecl);
    return xpce_result(envP, NULL);
  }
  pred = (char *) json_object_get_string(predobj);
  if (!json_object_is_type(argsobj, json_type_array)) {
    mcsat_err("Bad arguments, should be array of names\n");
    json_object_put(preddecl);
    return xpce_result(envP, NULL);
  }
  sorts = (char **) safe_malloc((json_object_array_length(argsobj) + 1) * sizeof(char *));
  for (i = 0; i < json_object_array_length(argsobj); i++) {
    arg = json_object_array_get_idx(argsobj, i);
    sorts[i] = (char*) json_object_get_string(arg);
  }
  sorts[i] = NULL;
  observable = json_object_get_boolean(observableobj);
  pthread_mutex_lock(&mutex);
  add_predicate(pred, sorts, observable, &samp_table);
  pthread_mutex_unlock(&mutex);
  safe_free(sorts);
  json_object_put(preddecl);
  return xpce_result(envP, NULL);
}


static xmlrpc_value *
xpce_constdecl(xmlrpc_env * const envP,
	       xmlrpc_value * const paramArrayP,
	       void * const serverInfo ATTR_UNUSED,
	       void * const channelInfo ATTR_UNUSED) {
  struct json_object *constdecl, *namesobj, *sortobj, *nameobj;
  char *name, *sort;
  int32_t i;

  printf("In xpce.const\n");
  constdecl = xpce_parse_decl(envP, paramArrayP);
  // should have form {"names": NAMES, "sort": SORT}
  if ((namesobj = json_object_object_get(constdecl, "names")) == NULL
      || !json_object_is_type(namesobj, json_type_array)
      || (sortobj = json_object_object_get(constdecl, "sort")) == NULL) {
    mcsat_err("xpce.const: expected {\"names\": NAMES, \"sort\": NAME}\n");
    json_object_put(constdecl);
    return xpce_result(envP, NULL);
  }
  sort = (char *) json_object_get_string(sortobj);
  for (i = 0; i < json_object_array_length(namesobj); i++) {
    nameobj = json_object_array_get_idx(namesobj, i);
    name = (char *) json_object_get_string(nameobj);
    pthread_mutex_lock(&mutex);
    add_constant(name, sort, &samp_table);
    pthread_mutex_unlock(&mutex);
  }
  json_object_put(constdecl);
  return xpce_result(envP, NULL);
}

char * xpce_json_valid_name(struct json_object *obj) {
  const char *str;
  
  if (!json_object_is_type(obj, json_type_string)) {
    return NULL;
  }
  str = json_object_get_string(obj);
  // Need to check for valid ID here
  
  // Make a copy
  return strdup(str);
}

char ** xpce_json_constant_array(struct json_object *obj) {
  struct json_object *jarg;
  char **arg;
  int32_t i;
  
  if (!json_object_is_type(obj, json_type_array)) {
    return NULL;
  }

  arg = (char **) safe_malloc((json_object_array_length(obj)+1) * sizeof(char *));
  for (i = 0; i < json_object_array_length(obj); i++) {
    jarg = json_object_array_get_idx(obj, i);
    if ((arg[i] = xpce_json_valid_name(jarg)) == NULL) {
      safe_free(arg);
      return NULL;
    }
  }
  arg[i] = NULL;
  return arg;
}

static pvector_t formula_vars;

void push_new_var(char *var) {
  int32_t i;
  
  for (i = 0; i < formula_vars.size; i++) {
    if (strcmp(var, (char *) formula_vars.data[i]) == 0) {
      return;
    }
  }
  pvector_push(&formula_vars, strdup(var));
}

char **copy_formula_vars() {
  char **var;
  int32_t i;

  var = (char **) safe_malloc((formula_vars.size + 1) * sizeof(char *));
  for (i = 0; i<formula_vars.size; i++) {
    if (formula_vars.data[i] == NULL) {
      var[i] = "";
    } else {
      var[i] = (char *) formula_vars.data[i];
    }
  }
  var[i] = NULL;
  return var;
}

static pvector_t fixed_preds;

void push_new_fixed_pred(char *pred) {
  int32_t i;
  
  for (i = 0; i < fixed_preds.size; i++) {
    if (strcmp(pred, (char *) fixed_preds.data[i]) == 0) {
      return;
    }
  }
  pvector_push(&fixed_preds, strdup(pred));
}

char **copy_fixed_preds() {
  char **var;
  int32_t i;

  var = (char **) safe_malloc((fixed_preds.size + 1) * sizeof(char *));
  for (i = 0; i<fixed_preds.size; i++) {
    if (fixed_preds.data[i] == NULL) {
      var[i] = "";
    } else {
      var[i] = (char *) fixed_preds.data[i];
    }
  }
  var[i] = NULL;
  return var;
}

char ** xpce_json_constants_array(struct json_object *obj) {
  struct json_object *jarg;
  char **arg;
  int32_t i;
  
  if (!json_object_is_type(obj, json_type_array)) {
    return NULL;
  }

  arg = (char **) safe_malloc((json_object_array_length(obj)+1) * sizeof(char *));
  for (i = 0; i < json_object_array_length(obj); i++) {
    jarg = json_object_array_get_idx(obj, i);
    if (! json_object_is_type(jarg, json_type_string)) {
      return NULL;
    }
    if ((arg[i] = xpce_json_valid_name(jarg)) == NULL) {
      safe_free(arg);
      return NULL;
    }
  }
  arg[i] = NULL;
  return arg;
}

char ** xpce_json_arguments_array(struct json_object *obj) {
  struct json_object *jarg, *jval;
  char **arg;
  int32_t i;
  
  if (!json_object_is_type(obj, json_type_array)) {
    return NULL;
  }

  arg = (char **) safe_malloc((json_object_array_length(obj)+1) * sizeof(char *));
  for (i = 0; i < json_object_array_length(obj); i++) {
    jarg = json_object_array_get_idx(obj, i);
    if (json_object_is_type(jarg, json_type_string)) {
      // A constant
      if ((arg[i] = xpce_json_valid_name(jarg)) == NULL) {
	safe_free(arg);
	return NULL;
      }
    } else if (json_object_is_type(jarg, json_type_object)
	       && (jval = json_object_object_get(jarg, "var")) != NULL) {
      if ((arg[i] = xpce_json_valid_name(jval)) == NULL) {
	safe_free(arg);
	return NULL;
      }
      push_new_var(arg[i]);
    } else {
      safe_free(arg);
      return NULL;
    }
  }
  arg[i] = NULL;
  return arg;
}

char ** xpce_json_fixedpreds_array(struct json_object *obj) {
  struct json_object *jpred;
  char **pred;
  int32_t i;
  
  if (!json_object_is_type(obj, json_type_array)) {
    return NULL;
  }

  pred = (char **) safe_malloc((json_object_array_length(obj)+1) * sizeof(char *));
  for (i = 0; i < json_object_array_length(obj); i++) {
    jpred = json_object_array_get_idx(obj, i);
    if (json_object_is_type(jpred, json_type_string)) {
      if ((pred[i] = xpce_json_valid_name(jpred)) == NULL) {
	safe_free(pred);
	return NULL;
      }
    } else {
      safe_free(pred);
      return NULL;
    }
  }
  pred[i] = NULL;
  return pred;
}

input_atom_t *
json_fact_to_input_atom(struct json_object *jfact) {
  struct json_object *predobj, *argsobj;
  char *pred, **args;
  input_atom_t *atom;

  if (! json_object_is_type(jfact, json_type_object)
      || ((predobj = json_object_object_get(jfact, "predicate")) == NULL)
      || ((pred = xpce_json_valid_name(predobj)) == NULL)
      || ((argsobj = json_object_object_get(jfact, "arguments")) == NULL)
      || (! json_object_is_type(argsobj, json_type_array))
      || ((args = xpce_json_constants_array(argsobj)) == NULL)) {
    mcsat_err("Expected a FACT of form {\"predicate\": NAME, \"arguments\": CONSTANTS}");
    return NULL;
  }
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  
  atom->pred = pred;
  atom->args = args;
  return atom;
}

input_atom_t *
json_atom_to_input_atom(struct json_object *jatom) {
  struct json_object *predobj, *argsobj;
  char *pred, **args;
  input_atom_t *atom;

  if (! json_object_is_type(jatom, json_type_object)
      || ((predobj = json_object_object_get(jatom, "predicate")) == NULL)
      || ((pred = xpce_json_valid_name(predobj)) == NULL)
      || ((argsobj = json_object_object_get(jatom, "arguments")) == NULL)
      || (! json_object_is_type(argsobj, json_type_array))
      || ((args = xpce_json_arguments_array(argsobj)) == NULL)) {
    mcsat_err("Expected an ATOM of form {\"predicate\": NAME, \"arguments\": ARGUMENTS}}\n");
    return NULL;
  }
  atom = (input_atom_t *) safe_malloc(sizeof(input_atom_t));
  atom->pred = pred;
  atom->args = args;
  // FIXME
  atom->builtinop = 0;
  return atom;
}

input_fmla_t *
json_binary_prop_to_input_fmla(int32_t op, struct json_object *args);

input_fmla_t *
json_formula_to_input_fmla(struct json_object *jfmla) {
  struct json_object *args;
  input_fmla_t *arg;
  input_atom_t *atom;
  
  if (! json_object_is_type(jfmla, json_type_object)) {
    mcsat_err("Expected FORMULA to be a JSON object:\n %s\n",
	      json_object_to_json_string(jfmla));
    return NULL;
  }
  if ((args = json_object_object_get(jfmla, "not")) != NULL) {
    if ((arg = json_formula_to_input_fmla(args)) != NULL) {
      return yy_fmla(NOT, arg, NULL);
    } else {
      return NULL;
    }
  } else if ((args = json_object_object_get(jfmla, "and")) != NULL) {
    return json_binary_prop_to_input_fmla(AND, args);
  } else if ((args = json_object_object_get(jfmla, "or")) != NULL) {
    return json_binary_prop_to_input_fmla(OR, args);
  } else if ((args = json_object_object_get(jfmla, "implies")) != NULL) {
    return json_binary_prop_to_input_fmla(IMPLIES, args);
  } else if ((args = json_object_object_get(jfmla, "xor")) != NULL) {
    return json_binary_prop_to_input_fmla(XOR, args);
  } else if ((args = json_object_object_get(jfmla, "iff")) != NULL) {
    return json_binary_prop_to_input_fmla(IFF, args);
  } else if ((args = json_object_object_get(jfmla, "atom")) != NULL) {
    if ((atom = json_atom_to_input_atom(args)) == NULL) {
      return NULL;
    } else {
      return yy_atom_to_fmla(atom);
    }
  } else {
    mcsat_err("Expected a FORMULA starting with \"not\", \"and\", \"or\", \"implies\", \"xor\",\"iff\", or \"atom\":\n %s",
	      json_object_to_json_string(jfmla));
    return NULL;
  }
}

input_fmla_t *
json_binary_prop_to_input_fmla(int32_t op, struct json_object *args) {
  input_fmla_t *arg1, *arg2;
  
  if (json_object_is_type(args, json_type_array)
      && (json_object_array_length(args) == 2)) {
    if ((arg1 = json_formula_to_input_fmla
	 (json_object_array_get_idx(args, 0))) != NULL) {
      if ((arg2 = json_formula_to_input_fmla
	   (json_object_array_get_idx(args, 1))) != NULL) {
	return yy_fmla(op, arg1, arg2);
      } else {
	free_fmla(arg1);
	return NULL;
      }
    } else {
      return NULL;
    }
  } else {
    mcsat_err("'%s' expects two arguments\n",
	      op==AND?"and":op==OR?"or":op==IMPLIES?"implies":op==XOR?"xor":"iff");
    return NULL;
  }
}

input_formula_t *json_formula_to_input_formula(struct json_object *jfmla) {
  input_fmla_t *fmla;

  formula_vars.size = 0;
  fmla = json_formula_to_input_fmla(jfmla);
  if (fmla == NULL) {
    return NULL;
  } else {
    pvector_push(&formula_vars, NULL);
    return yy_formula((char **)formula_vars.data, fmla);
  }
}
  

static xmlrpc_value *
xpce_assert(xmlrpc_env * const envP,
	    xmlrpc_value * const paramArrayP,
	    void * const serverInfo ATTR_UNUSED,
	    void * const channelInfo ATTR_UNUSED) {

  struct json_object *assertdecl, *factobj, *sourceobj;
  input_atom_t *atom;
  char *source;

  printf("In xpce.assert\n");
  assertdecl = xpce_parse_decl(envP, paramArrayP);
  if (!json_object_is_type(assertdecl, json_type_object)) {
    mcsat_err("Bad argument: expected FACT\n");
    json_object_put(assertdecl);
    return xpce_result(envP, NULL);
  }
    
  factobj = json_object_object_get(assertdecl, "fact");
  if (factobj == NULL) {
    mcsat_err("Bad argument: expected FACT");
    json_object_put(assertdecl);
    return xpce_result(envP, NULL);
  }
  if ((atom = json_fact_to_input_atom(factobj)) == NULL) {
    json_object_put(assertdecl);
    return xpce_result(envP, NULL);
  }
  // OK if source is NULL
  sourceobj = json_object_object_get(assertdecl, "source");
  if (sourceobj == NULL) {
    source = NULL;
  } else {
    if ((source = xpce_json_valid_name(sourceobj)) == NULL) {
      mcsat_err("Source is not a valid NAME\n");
      json_object_put(assertdecl);
      return xpce_result(envP, NULL);
    }
    source = (char *) json_object_get_string(sourceobj);
  }
  pthread_mutex_lock(&mutex);
  assert_atom(&samp_table, atom, source);
  pthread_mutex_unlock(&mutex);

  json_object_put(assertdecl);
  free_atom(atom);
  return xpce_result(envP, NULL);
}

static xmlrpc_value *
xpce_add(xmlrpc_env * const envP,
	 xmlrpc_value * const paramArrayP,
	 void * const serverInfo ATTR_UNUSED,
	 void * const channelInfo ATTR_UNUSED) {
  struct json_object *adddecl, *fixedobj, *fmlaobj, *weightobj, *sourceobj;
  input_formula_t *fmla;
  double weight;
  char *source, **fixed;

  printf("In xpce.add\n");
  adddecl = xpce_parse_decl(envP, paramArrayP);
  if (!json_object_is_type(adddecl, json_type_object)) {
    mcsat_err("Bad argument: expected {\"fixed\": NAMES, \"formula\": FORMULA, \"weight\": NUM, \"source\": NAME}\n");
    json_object_put(adddecl);
    return xpce_result(envP, NULL);
  }
  fixedobj = json_object_object_get(adddecl, "fixed");
  if (fixedobj == NULL) {
    fixed = NULL;
  } else {
    fixed = xpce_json_fixedpreds_array(fixedobj);
  }
  fmlaobj = json_object_object_get(adddecl, "formula");
  if (fmlaobj == NULL) {
    mcsat_err("Bad argument: expected {\"formula\": FORMULA}\n");
    json_object_put(adddecl);
    return xpce_result(envP, NULL);
  }
  if ((fmla = json_formula_to_input_formula(fmlaobj)) == NULL) {
    json_object_put(adddecl);
    return xpce_result(envP, NULL);
  }
  weightobj = json_object_object_get(adddecl, "weight");
  // If weight is NULL, use DBL_MAX
  if (weightobj == NULL) {
    weight = DBL_MAX;
  } else {
    if (json_object_is_type(weightobj, json_type_double)) {
      weight = json_object_get_double(weightobj);
    } else if (json_object_is_type(weightobj, json_type_int)) {
      weight = json_object_get_int(weightobj);
    } else {
      mcsat_err("Bad argument: \"weight\" should be DOUBLE\n");
      json_object_put(adddecl);
      return xpce_result(envP, NULL);
    }
  }
  // OK if source is NULL
  sourceobj = json_object_object_get(adddecl, "source");
  if (sourceobj == NULL) {
    source = NULL;
  } else {
    if ((source = xpce_json_valid_name(sourceobj)) == NULL) {
      mcsat_err("Source is not a valid NAME\n");
      json_object_put(adddecl);
      return xpce_result(envP, NULL);
    }
    source = (char *) json_object_get_string(sourceobj);
  }
  pthread_mutex_lock(&mutex);
  add_cnf(fixed, fmla, weight, source, true);
  pthread_mutex_unlock(&mutex);
  
  free_formula(fmla);
  json_object_put(adddecl);
  return xpce_result(envP, NULL);
}

// JSON substitution functions

struct json_object *
subst_into_json_tuple(struct json_object *tuple,
		      var_entry_t **vars,
		      int32_t *qsubst);

struct json_object *
subst_into_json_atom(struct json_object *atom,
		     var_entry_t **vars,
		     int32_t *qsubst) {
  struct json_object *val, *natom, *nval, *args, *nargs, *arg, *pred, *nconst, *var;
  char *cname;
  int32_t i, j;

  val = json_object_object_get(atom, "atom");
  args = json_object_object_get(val, "arguments");
  nargs = json_object_new_array();
  for (i = 0; i < json_object_array_length(args); i++) {
    arg = json_object_array_get_idx(args, i);
    if (json_object_is_type(arg, json_type_object)
	&& (var = json_object_object_get(arg, "var")) != NULL) {
      for (j = 0; vars[j] != NULL; j++) {
	if (strcmp(vars[j]->name, json_object_get_string(var)) == 0) {
	  cname = const_name(qsubst[j], &samp_table.const_table);
	  nconst = json_object_new_string(strdup(cname));
	  json_object_array_add(nargs,nconst);
	  break;
	}
      }
      // Check that we found the variable
      assert(vars[j] != NULL);
    } else {
      cname = (char *) json_object_get_string(arg);
      nconst = json_object_new_string(strdup(cname));
      json_object_array_add(nargs, nconst);
    }
  }
  nval = json_object_new_object();
  pred = json_object_object_get(val, "predicate");
  json_object_object_add(nval, "predicate",
			 json_object_new_string(strdup(json_object_get_string(pred))));
  json_object_object_add(nval, "arguments", nargs);
  
  natom = json_object_new_object();
  json_object_object_add(natom, "atom", nval);
  return natom;
}

struct json_object *
subst_into_json_fmla(struct json_object *fmla,
		     var_entry_t **vars,
		     int32_t *subst) {
  struct json_object *nfmla, *args, *nargs;

  nfmla = NULL;
  if ((args = json_object_object_get(fmla, "not")) != NULL) {
    nfmla = json_object_new_object();
    nargs = subst_into_json_fmla(args, vars, subst);
    json_object_object_add(nfmla, "not", nargs);
  } else if ((args = json_object_object_get(fmla, "and")) != NULL) {
    nfmla = json_object_new_object();
    nargs = subst_into_json_tuple(args, vars, subst);
    json_object_object_add(nfmla, "and", nargs);
  } else if ((args = json_object_object_get(fmla, "or")) != NULL) {
    nfmla = json_object_new_object();
    nargs = subst_into_json_tuple(args, vars, subst);
    json_object_object_add(nfmla, "or", nargs);
  } else if ((args = json_object_object_get(fmla, "implies")) != NULL) {
    nfmla = json_object_new_object();
    nargs = subst_into_json_tuple(args, vars, subst);
    json_object_object_add(nfmla, "implies", nargs);
  } else if ((args = json_object_object_get(fmla, "xor")) != NULL) {
    nfmla = json_object_new_object();
    nargs = subst_into_json_tuple(args, vars, subst);
    json_object_object_add(nfmla, "xor", nargs);
  } else if ((args = json_object_object_get(fmla, "iff")) != NULL) {
    nfmla = json_object_new_object();
    nargs = subst_into_json_tuple(args, vars, subst);
    json_object_object_add(nfmla, "iff", nargs);
  } else if ((args = json_object_object_get(fmla, "atom")) != NULL) {
    nfmla = subst_into_json_atom(fmla, vars, subst);
  } else {
    mcsat_err("Bad formuls\n");
  }
  return nfmla;
}

struct json_object *
subst_into_json_tuple(struct json_object *tuple,
		      var_entry_t **vars,
		      int32_t *qsubst) {
  struct json_object *ntuple, *elt, *nelt;
  int32_t i;

  ntuple = json_object_new_array();
  for (i = 0; i < json_object_array_length(tuple); i++) {
    elt = json_object_array_get_idx(tuple, i);
    nelt = subst_into_json_fmla(elt, vars, qsubst);
    json_object_array_add(ntuple, nelt);
  }
  return ntuple;
}
    

static xmlrpc_value *
xpce_ask(xmlrpc_env * const envP,
	 xmlrpc_value * const paramArrayP,
	 void * const serverInfo ATTR_UNUSED,
	 void * const channelInfo ATTR_UNUSED) {

  struct json_object *askdecl, *fmlaobj, *thresholdobj, *maxresultsobj, *answer,
    *finfo, *fsubst, *finst;
  input_formula_t *fmla;
  samp_query_instance_t *qinst;
  double threshold, prob;
  int32_t maxresults, i, j, *qsubst;
  char *cname;

  // Get JSON form
  askdecl = xpce_parse_decl(envP, paramArrayP);
  cprintf(1, "In xpce.ask:\n  %s\n", json_object_to_json_string(askdecl));
  if (!json_object_is_type(askdecl, json_type_object)) {
    mcsat_err("Bad argument: expected {\"formula\": FORMULA, \"threshold\": NUM, \"maxresults\": NAME}\n");
    json_object_put(askdecl);
    return xpce_result(envP, NULL);
  }
  fmlaobj = json_object_object_get(askdecl, "formula");
  if (fmlaobj == NULL) {
    mcsat_err("Bad argument: expected {\"formula\": FORMULA}");
    json_object_put(askdecl);
    return xpce_result(envP, NULL);
  }
  // Convert JSON formula to input_formula_t
  fmla = json_formula_to_input_formula(fmlaobj);
  if (fmla == NULL) {
    json_object_put(askdecl);
    return xpce_result(envP, NULL);
  }
  thresholdobj = json_object_object_get(askdecl, "threshold");
  // If threshold is NULL, use 0.0
  if (thresholdobj == NULL) {
    threshold = 0.0;
  } else {
    if (json_object_is_type(thresholdobj, json_type_double)) {
      threshold = json_object_get_double(thresholdobj);
    } else if (json_object_is_type(thresholdobj, json_type_int)) {
      threshold = json_object_get_int(thresholdobj);
    } else {
      mcsat_err("Bad argument: \"threshold\" should be DOUBLE or INT\n");
      json_object_put(askdecl);
      return xpce_result(envP, NULL);
    }
  }
  if (threshold < 0.0 || threshold > 1.0) {
    mcsat_err("Threshold should be a probability value between 0.0 and 1.0");
    json_object_put(askdecl);
    return xpce_result(envP, NULL);
  }
  // OK if maxresults is NULL
  maxresultsobj = json_object_object_get(askdecl, "maxresults");
  if (maxresultsobj == NULL) {
    maxresults = 0;
  } else {
    if (!json_object_is_type(maxresultsobj, json_type_int)) {
      mcsat_err("\"maxresults\" should be INT\n");
      json_object_put(askdecl);
      return xpce_result(envP, NULL);
    }
    maxresults = json_object_get_int(maxresultsobj);
  }
  // Get a lock
  pthread_mutex_lock(&mutex);
  // Now ask - sets up query tables and runs mcsat
  ask_cnf(fmla, threshold, maxresults);
  // Results are in ask_buffer - create JSON objects representing the instances
  // E.g., [{"subst": {"x": "Dan", "y": "Ron"}, "formula-instance": FORMULA, "probability": 0.7}, ...]
  answer = json_object_new_array();
  for (i = 0; i < ask_buffer.size; i++) {
    qinst = (samp_query_instance_t *) ask_buffer.data[i];
    qsubst = qinst->subst;
    finfo = json_object_new_object();
    fsubst = json_object_new_object();
    for (j = 0; fmla->vars[j] != NULL; j++) {
      cname = const_name(qsubst[j], &samp_table.const_table);
      json_object_object_add(fsubst, fmla->vars[j]->name,
			     json_object_new_string(strdup(cname)));
    }
    json_object_object_add(finfo, "subst", fsubst);
    // Now to get the formula instance
    finst = subst_into_json_fmla(fmlaobj, fmla->vars, qsubst);
    json_object_object_add(finfo, "formula-instance", finst);
    // Now the marginal prob
    prob = query_probability(qinst, &samp_table);
    json_object_object_add(finfo, "probability",
			   json_object_new_double(prob));
    json_object_array_add(answer, finfo);
  }
  // Now clear out the query_instance table for the next query
  reset_query_instance_table(&samp_table.query_instance_table);
  pthread_mutex_unlock(&mutex);
  free_formula(fmla);
  json_object_put(askdecl);
  return xpce_result(envP, answer);
}


static xmlrpc_value *
xpce_mcsat(xmlrpc_env * const envP,
	   xmlrpc_value * const paramArrayP,
	   void * const serverInfo ATTR_UNUSED,
	   void * const channelInfo ATTR_UNUSED) {

  // No need to parse, just run MCSAT and return warnings or errors
  pthread_mutex_lock(&mutex);
  if (lazy_mcsat()) {
    lazy_mc_sat(&samp_table, get_max_samples(), get_sa_probability(),
		get_samp_temperature(), get_rvar_probability(),
		get_max_flips(), get_max_extra_flips(), get_mcsat_timeout());
  } else {
    mc_sat(&samp_table, get_max_samples(), get_sa_probability(),
	   get_samp_temperature(), get_rvar_probability(),
	   get_max_flips(), get_max_extra_flips(), get_mcsat_timeout(),
	   get_burn_in_steps(), get_samp_interval());
  }
  pthread_mutex_unlock(&mutex);
  
  return xpce_result(envP, NULL);
}


static xmlrpc_value *
xpce_reset(xmlrpc_env * const envP,
	   xmlrpc_value * const paramArrayP,
	   void * const serverInfo ATTR_UNUSED,
	   void * const channelInfo ATTR_UNUSED) {
  struct json_object *resetdecl, *resetobj;

  // Reset takes a single argument
  printf("In xpce.reset\n");
  resetdecl = xpce_parse_decl(envP, paramArrayP);
  // should have form {"what": NAME}:
  //    may be empty, NAME is 'all' or 'probabilities'
  if (envP->fault_occurred) {
    mcsat_err("Bad argument: expected {\"what\": NAME}\n");
    json_object_put(resetdecl);
    return xpce_result(envP, NULL);
  }
  resetobj = json_object_object_get(resetdecl, "what");
  if (resetobj == NULL ||
      strcasecmp(json_object_get_string(resetobj), "all") == 0) {
    // Resets the sample tables
    reset_sort_table(&samp_table.sort_table);
    // Need to do more here - like free up space.
    init_samp_table(&samp_table);
    rand_reset();
  } else if (strcasecmp(json_object_get_string(resetobj), "probabilities") == 0) {
    // Simply resets the probabilities of the atom table to -1.0
    output("Resetting probabilities of atoms to -1.0\n");
    int32_t i;
    samp_table.atom_table.num_samples = 0;
    for (i=0; i<samp_table.atom_table.num_vars; i++) {
      samp_table.atom_table.pmodel[i] = -1;
    }
  } else {
    mcsat_err("Bad argument: expected {\"what\": NAME}\n");
  }
  json_object_put(resetdecl);
  return xpce_result(envP, NULL);
}


// static xmlrpc_value *
// xpce_dumptable(xmlrpc_env * const envP,
// 	       xmlrpc_value * const paramArrayP,
// 	       void * const serverInfo ATTR_UNUSED,
// 	       void * const channelInfo ATTR_UNUSED) {

//   const char *table;
//   int32_t tbl;
//   /* Parse our argument array. */
//   xmlrpc_decompose_value(envP, paramArrayP, "(s)", &table);
//   if (envP->fault_occurred)
//     return NULL;
  
//   fprintf(stderr, "Got dumptable %s\n", table);
//   if (strcasecmp(table,"all") == 0) {
//     tbl = ALL;
//   } else if (strcasecmp(table,"sort") == 0) {
//     tbl = SORT;
//   } else if (strcasecmp(table,"predicate") == 0) {
//     tbl = PREDICATE;
//   } else if (strcasecmp(table,"atom") == 0) {
//     tbl = ATOM;
//   } else if (strcasecmp(table,"clause") == 0) {
//     tbl = CLAUSE;
//   } else if (strcasecmp(table,"rule") == 0) {
//     tbl = RULE;
//   } else {
//     return NULL;
//   }
//   // This just prints out at the server end - need to send the string
//   dumptable(tbl, &samp_table);
//   return xmlrpc_build_value(envP, "i", 0);
// }

static xmlrpc_value *
xpce_quit(xmlrpc_env * const envP,
	  xmlrpc_value * const paramArrayP,
	  void * const serverInfo ATTR_UNUSED,
	  void * const channelInfo ATTR_UNUSED) {

  // No need to parse params, just exit
  xmlrpc_server_abyss_terminate(envP, serverToTerminateP);
  return xpce_result(envP, NULL);
}


static xmlrpc_value *
xpce_command(xmlrpc_env * const envP,
	     xmlrpc_value * const paramArrayP,
	     void * const serverInfo ATTR_UNUSED,
	     void * const channelInfo ATTR_UNUSED) {

  const char *cmd;
  char *output;
  xmlrpc_value *value;
  
  /* Parse our argument array. */
  xmlrpc_decompose_value(envP, paramArrayP, "(s)", &cmd);
  if (envP->fault_occurred)
    return NULL;

  pthread_mutex_lock(&mutex);
  fprintf(stderr, "Got cmd %s\n", cmd);
  //in = fmemopen((void *)cmd, strlen (cmd), "r");
  //input_stack_push_stream(in, (char *)cmd);
  input_stack_push_string((char *)cmd);
  // Now set up the output stream
  //out = open_memstream(&output, &size); // Grows as needed
  //set_output_stream(out);
  output_to_string = true;
  read_eval(&samp_table);
  fprintf(stderr, "Processed cmd %s\n", cmd);
  if (error_buffer.size > 0) {
    output = get_string_from_buffer(&error_buffer);
    xmlrpc_env_set_fault(envP, 1, output);
    pthread_mutex_unlock(&mutex);
    return NULL;
  } else {
    output = get_string_from_buffer(&output_buffer);
    if (strlen(output) == 0) {
      fprintf(stderr, "Returning 0 value\n");
      value = xmlrpc_build_value(envP, "i", 0);
    } else {
      //fprintf(stderr, "Returning string value of size %d: %s\n", (int)strlen(output), output);
      value = xmlrpc_build_value(envP, "s", output);
      safe_free(output);
      // Should be safe to free the string after this, here we simply set the index to 0
    }
    fprintf(stderr, "Returning value\n");
    pthread_mutex_unlock(&mutex);
    return value;
  }
}


int main(int const argc, const char ** const argv) {

  // Force output, mcsat_err, and mcsat_warn to save to buffer (see print.c)
  output_to_string = true; 
  rand_reset();
  init_samp_table(&samp_table);
  input_stack_push_file("");
  yylloc.first_line = 1;
  yylloc.first_column = 0;
  yylloc.last_line = 1;
  yylloc.last_column = 0;

  struct xmlrpc_method_info3 const methodInfoCommand
    = {"xpce.command", &xpce_command};
  struct xmlrpc_method_info3 const methodInfoSort
    = {"xpce.sort", &xpce_sort};
  struct xmlrpc_method_info3 const methodInfoPredicate
    = {"xpce.predicate",&xpce_predicate};
  struct xmlrpc_method_info3 const methodInfoConstDecl
    = {"xpce.const",&xpce_constdecl};
  struct xmlrpc_method_info3 const methodInfoAssert
    = {"xpce.assert", &xpce_assert};
  struct xmlrpc_method_info3 const methodInfoAdd
    = {"xpce.add", &xpce_add};
  struct xmlrpc_method_info3 const methodInfoAsk
    = {"xpce.ask", &xpce_ask};
  struct xmlrpc_method_info3 const methodInfoMcsat
    = {"xpce.mcsat", &xpce_mcsat};
//   struct xmlrpc_method_info3 const methodInfoMcsatParams
//     = {"xpce.mcsat_params", &xpce_mcsat_params};
  struct xmlrpc_method_info3 const methodInfoReset
    = {"xpce.reset", &xpce_reset};
//   struct xmlrpc_method_info3 const methodInfoRetract
//     = {"xpce.retract", &xpce_retract};
//   struct xmlrpc_method_info3 const methodInfoDumptable
//     = {"xpce.dumptable",&xpce_dumptable};
  struct xmlrpc_method_info3 const methodInfoQuit
    = {"xpce.quit", &xpce_quit};
  
  xmlrpc_server_abyss_parms serverparm;
  xmlrpc_registry *registryP;
  xmlrpc_env env;
  xmlrpc_server_abyss_t *serverP;
  xmlrpc_server_abyss_sig *oldHandlersP;
  
  if (argc-1 != 1) {
    fprintf(stderr, "You must specify 1 argument:  The TCP port "
	    "number on which the server will accept connections "
	    "for RPCs (8080 is a common choice).  "
	    "You specified %d arguments.\n",  argc-1);
    exit(1);
  }

  // Initialize the error structure
  xmlrpc_env_init(&env);
  
  // Global initialization (needed to use xmlrpc_server_abyss_t objects)
  xmlrpc_server_abyss_global_init(&env);
  
  // Register methods
  registryP = xmlrpc_registry_new(&env);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoCommand);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoSort);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoPredicate);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoConstDecl);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoAssert);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoAdd);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoAsk);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoMcsat);
//   xmlrpc_registry_add_method3(&env, registryP, &methodInfoMcsatParams);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoReset);
//   xmlrpc_registry_add_method3(&env, registryP, &methodInfoRetract);
//   xmlrpc_registry_add_method3(&env, registryP, &methodInfoDumptable);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoQuit);
  
  /* In the modern form of the Abyss API, we supply parameters in memory
     like a normal API.  We select the modern form by setting
     config_file_name to NULL: 
  */
  serverparm.config_file_name = NULL;
  serverparm.registryP        = registryP;
  serverparm.port_number      = atoi(argv[1]);
#ifdef _WIN32
  serverparm.log_file_name    = "C:\\xmlpce_log";
#else
  serverparm.log_file_name    = "/tmp/xmlpce_log";
#endif

  // log_file_name is the last parameter set above
  xmlrpc_server_abyss_create(&env, &serverparm, XMLRPC_APSIZE(log_file_name), &serverP);

  xmlrpc_server_abyss_setup_sig(&env, serverP, &oldHandlersP);

  serverToTerminateP = serverP;
  
  printf("Running XPCE server...\n");

  /* xmlrpc_server_abyss_run_server() returns when terminated */

  xmlrpc_server_abyss_run_server(&env, serverP);
  
  //  xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));
  
  /* xmlrpc_server_abyss() never returns */

  xmlrpc_server_abyss_restore_sig(oldHandlersP);

  xmlrpc_server_abyss_destroy(serverP);

  xmlrpc_server_abyss_global_term();
    
  return 0;
}
