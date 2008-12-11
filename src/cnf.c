//#include <inttypes.h>
//#include <float.h>
#include <string.h>
#include "memalloc.h"
#include "utils.h"
//#include "parser.h"
#include "yacc.tab.h"
//#include "print.h"
#include "input.h"
//#include "samplesat.h"

extern samp_table_t samp_table;

static int32_t vars_len(var_entry_t **vars) {
  int32_t vlen;

  if (vars == NULL) {
    return 0;
  } else {
    for (vlen = 0; vars[vlen] != NULL; vlen++) {}
    return vlen;
  }
}

static rule_literal_t *atom_to_rule_literal(input_atom_t *iatom,
					    var_entry_t **vars, bool neg) {
  pred_table_t *pred_table = &samp_table.pred_table;
  sort_table_t *sort_table = &samp_table.sort_table;
  const_table_t *const_table = &samp_table.const_table;
  int32_t i, j, num_args, pred_val, pred_idx, const_idx, *psig, vsig, subsig, vlen;
  char *pred, *var, *cname;
  samp_atom_t *atom;
  rule_literal_t *lit;
  
  pred = iatom->pred;
  // Get number of args
  for (num_args = 0; iatom->args[num_args] != NULL; num_args++) {}
  // check if we've seen the predicate before and complain if not.
  pred_val = pred_index(pred, pred_table);
  if (pred_val == -1) {
    printf("Predicate %s not declared", pred);
    return NULL;
  }
  pred_idx = pred_val_to_index(pred_val);
  if (pred_arity(pred_idx, pred_table) != num_args) {
    printf("Wrong number of args");
    return NULL;
  }
  atom = (samp_atom_t *) safe_malloc((num_args + 1) * sizeof(samp_atom_t));
  atom->pred = pred_idx;
  psig = pred_signature(pred_idx, pred_table);
  vlen = vars_len(vars);
  for (i = 0; i < num_args; i++) {
    for (j = 0; j < vlen; j++) {
      if (strcmp(vars[j]->name, iatom->args[i]) == 0) {
	break;
      }
    }
    if (j < vlen) {
      // Variables are negated indices into the var array, which will match
      // the names_buffer.  Need to add one in order not to hit zero.
      // Check the signature, and adjust to the least common supersort
      vsig = vars[j]->sort_index;
      if (vsig == -1) {
	vars[j]->sort_index = psig[i];
      } else {
	subsig = greatest_common_subsort(vsig, psig[i], sort_table);
	if (subsig != vsig) {
	  if (subsig == -1) {
	    printf("Variable %s used with incompatible sorts %s and %s",
		   var, sort_table->entries[vsig].name,
		   sort_table->entries[psig[i]].name);
	    return NULL;
	}
	vars[j]->sort_index = subsig;
	}
      }
      // Variables are negated indices into the var array, which will match
      // the names_buffer.  Need to add one in order not to hit zero.
      atom->args[i] = -(j + 1);
    } else {
      // Must be a constant
      cname = iatom->args[i];
      // See if constant is known
      const_idx = stbl_find(&const_table->const_name_index, cname);
      if (const_idx == -1) {
	if (!strict_constants()) {
	  // Add the constant - note that the sort is determined by the
	  // predicate, and may be wrong if psig[i] has supersorts -
	  // warn in such a case.
	  add_const_internal(cname, psig[i], &samp_table);
	  if (sort_table->entries[psig[i]].supersorts != NULL) {
	    printf("Warning: constant %s assigned to subsort %s\n",
		   cname, sort_table->entries[psig[i]].name);
	  }
	  const_idx = stbl_find(&const_table->const_name_index, cname);
	} else {
	  printf("Constant %s not previously declared\n", cname);
	  return NULL;
	}
      } else {
	sort_entry_t *sort_entry = &samp_table.sort_table.entries[psig[i]];
	bool foundit = false;
	for (j = 0; j < sort_entry->cardinality; j++) {
	  if (sort_entry->constants[j] == const_idx) {
	    foundit = true;
	    break;
	  }
	}
	if (!foundit) {
	  printf("Sort error: constant %s of sort %s\n does not match pred %s arg %d sort %s\n",
		    cname, const_sort_name(const_idx,&samp_table), pred, i+1, sort_entry->name);
	  return NULL;
	}
      }
      atom->args[i] = const_idx;
    }
  }
  //print_atom(atom, &samp_table);
  //fflush(stdout);
  lit = (rule_literal_t *) safe_malloc(sizeof(rule_literal_t));
  lit->neg = neg;
  lit->atom = atom;
  return lit;
}

static rule_literal_t *** cnf_literal(input_atom_t *atom, var_entry_t **vars,
				      bool neg) {
  rule_literal_t *lit;
  rule_literal_t **conj_lit;
  rule_literal_t ***cnf_lit;
  
  lit = atom_to_rule_literal(atom, vars, neg);
  if (lit == NULL) {
    return NULL;
  }
  conj_lit = (rule_literal_t **) safe_malloc(2 * sizeof(rule_literal_t *));
  cnf_lit = (rule_literal_t ***) safe_malloc(2 * sizeof(rule_literal_t **));
  cnf_lit[0] = conj_lit;
  cnf_lit[1] = NULL;
  conj_lit[0] = lit;
  conj_lit[1] = NULL;
  return cnf_lit;
}

// Given two CNFs - essentially lists of clauses, which are lists of
// literals.  We construct the product.  Here the CNFs are represented by
// (NULL-terminated) arrays of arrays of literals.
//   ((a b) (c d)) X ((x y z) (w)) == ((a b x y z) (a b w) (c d x y z) (c d w))
// For now, no simplification is done: so
// ((a b) (c)) X ((a ~c)) == ((a b a ~c) (c a ~c)), which could simplify to
//  ((a b ~c))

static rule_literal_t ***cnf_product(rule_literal_t ***c1,
				     rule_literal_t ***c2) {
  int32_t i, j, ii, jj, len1, len2, cnflen1, cnflen2, idx;
  rule_literal_t ***productcnf;
  rule_literal_t **conjunct;

  if (c1 == NULL || c2 == NULL) {
    return NULL;
  }
  for (cnflen1 = 0; c1[cnflen1] != 0; cnflen1++) {}
  for (cnflen2 = 0; c1[cnflen2] != 0; cnflen2++) {}
  productcnf = (rule_literal_t ***)
    safe_malloc((cnflen1 * cnflen2 + 1) * sizeof(rule_literal_t **));
  idx = 0;
  for (i = 0; c1[i] != NULL; i++) {
    for (len1 = 0; c1[i][len1] != NULL; len1++) {}
    for (j = 0; c2[j] != NULL; j++) {
      for (len2 = 0; c2[j][len2] != NULL; len2++) {}
      conjunct = (rule_literal_t **)
	safe_malloc((len1 + len2 + 1) * sizeof(rule_literal_t *));
      for (ii = 0; ii < len1; ii++) {
	conjunct[ii] = c1[i][ii];
      }
      for (jj = 0; jj < len2; jj++) {
	conjunct[len1+jj] = c2[j][jj];
      }
      conjunct[len1+len2] = NULL;
      productcnf[idx++] = conjunct;
    }
  }
  productcnf[idx] = NULL;
  for (i = 0; c1[i] != NULL; i++) {
    safe_free(c1[i]);
  }
  safe_free(c1);
  for (j = 0; c2[j] != NULL; j++) {
    safe_free(c2[j]);
  }
  safe_free(c2);
  return productcnf;
}

// Given two CNFs, return the union.
// ((a b) (c d)) U ((x y z) (w)) == ((a b) (c d) (x y z) (w))
// Again, no simplification involved.

static rule_literal_t ***cnf_union(rule_literal_t ***c1,
				   rule_literal_t ***c2) {
  int32_t i, cnflen1, cnflen2;
  rule_literal_t ***unioncnf;
  
  if (c1 == NULL || c2 == NULL) {
    return NULL;
  }
  for (cnflen1 = 0; c1[cnflen1] != 0; cnflen1++) {}
  for (cnflen2 = 0; c1[cnflen2] != 0; cnflen2++) {}
  unioncnf = (rule_literal_t ***)
    safe_malloc((cnflen1 + cnflen2 + 1) * sizeof(rule_literal_t **));
  for (i = 0; c1[i] != NULL; i++) {
    unioncnf[i] = c1[i];
  }
  for (i = 0; c2[i] != NULL; i++) {
    unioncnf[cnflen1+i] = c2[i];
  }
  unioncnf[cnflen1+i] = NULL;
  safe_free(c1);
  safe_free(c2);
  return unioncnf;
}

rule_literal_t ***cnf_neg(input_fmla_t *fmla, var_entry_t **vars);

rule_literal_t ***cnf_pos(input_fmla_t *fmla, var_entry_t **vars) {
  int32_t op;
  input_atom_t *atom;
  input_comp_fmla_t *cfmla;

  if (fmla->kind == ATOM) {
    atom = fmla->ufmla->atom;
    return cnf_literal(atom, vars, false);
  } else {
    cfmla = fmla->ufmla->cfmla;
    op = cfmla->op;
    if (op == NOT) {
      return cnf_neg(cfmla->arg1, vars);
    } else if (op == OR) {
      return cnf_product(cnf_pos(cfmla->arg1, vars),
			 cnf_pos(cfmla->arg2, vars));
    } else if (op == IMPLIES) {
      return cnf_product(cnf_neg(cfmla->arg1, vars),
			 cnf_pos(cfmla->arg2, vars));
    } else if (op == AND) {
      return cnf_union(cnf_pos(cfmla->arg1, vars),
		       cnf_pos(cfmla->arg2, vars));
    } else if (op == IFF) {
      // CNF(A iff B) = CNF(A /\ B) X CNF(not(A) /\ not(B))
      return cnf_product(cnf_union(cnf_pos(cfmla->arg1, vars),
				   cnf_pos(cfmla->arg2, vars)),
			 cnf_union(cnf_neg(cfmla->arg1, vars),
				   cnf_neg(cfmla->arg2, vars)));
    } else {
      printf("cnfpos error: op %d unexpected\n", op);
      return NULL;
    }
  }
}

rule_literal_t ***cnf_neg(input_fmla_t *fmla, var_entry_t **vars) {
  int32_t op;
  input_atom_t *atom;
  input_comp_fmla_t *cfmla;
  
  if (fmla->kind == ATOM) {
    atom = fmla->ufmla->atom;
    return cnf_literal(atom, vars, true);
  } else {    
    cfmla = fmla->ufmla->cfmla;
    op = cfmla->op;
    if (op == NOT) {
      return cnf_pos(cfmla->arg1, vars);
    } else if (op == OR) {
      return cnf_union(cnf_neg(cfmla->arg1, vars),
		       cnf_neg(cfmla->arg2, vars));
    } else if (op == IMPLIES) {
      return cnf_union(cnf_pos(cfmla->arg1, vars),
		       cnf_neg(cfmla->arg2, vars));
    } else if (op == AND) {
      return cnf_product(cnf_neg(cfmla->arg1, vars),
			 cnf_neg(cfmla->arg2, vars));
    } else if (op == IFF) {
      // CNF(not(A iff B)) = CNF(A \/ B) U CNF(not(A) \/ not(B))
      return cnf_union(cnf_product(cnf_pos(cfmla->arg1, vars),
				   cnf_pos(cfmla->arg2, vars)),
		       cnf_product(cnf_neg(cfmla->arg1, vars),
				   cnf_neg(cfmla->arg2, vars)));
    } else {
      printf("cnfpos error: op %d unexpected\n", op);
      return NULL;
    }
  }
}

var_entry_t **copy_variables(var_entry_t **vars) {
  var_entry_t **cvars;
  int32_t i, numvars;

  for (numvars = 0; vars[numvars] != NULL; numvars++) {}
  cvars = (var_entry_t **) safe_malloc(numvars * sizeof(var_entry_t *));
  for (i = 0; i < numvars; i++) {
    cvars[i] = (var_entry_t *) safe_malloc(sizeof(var_entry_t));
    cvars[i]->name = str_copy(vars[i]->name);
    cvars[i]->sort_index = vars[i]->sort_index;
  }
  return cvars;
}

// Sets the variables in the clause, removing those not mentioned
void set_fmla_clause_variables(samp_rule_t *clause, var_entry_t **vars) {
  pred_table_t *pred_table = &samp_table.pred_table;
  samp_atom_t *atom;
  var_entry_t **cvars;
  int32_t i, j, numvars, arity, vidx, delta, *mapping;
  bool usedvar, unusedvar;
  
  // First copy the vars, then tag all with negated signatures
  // assumes all sigs are nonnegative.
  for (numvars = 0; vars[numvars] != NULL; numvars++) {}
  cvars = copy_variables(vars);
  for (i = 0; i < numvars; i++) {
    assert(cvars[i]->sort_index >= 0);
    cvars[i]->sort_index = -(cvars[i]->sort_index + 1);
  }
  // Now go through the literals, untagging the found variables
  for (i = 0; i < clause->num_lits; i++) {
    atom = clause->literals[i]->atom;
    arity = pred_arity(atom->pred, pred_table);
    // Loop through the args of the atom
    for (j = 0; j < arity; j++) {
      if (atom->args[j] < 0) {
	// We have a variable - vidx is the index into vars
	vidx = -(atom->args[j] + 1);
	if (cvars[vidx]->sort_index < 0) {
	  cvars[vidx]->sort_index = -(cvars[vidx]->sort_index + 1);
	}
      }
    }
  }
  usedvar = false;
  unusedvar = false;
  for (i = 0; i < numvars; i++) {
    if (cvars[i]->sort_index >= 0) {
      usedvar = true;
    } else {
      unusedvar = true;
    }
  }
  if (usedvar) {
    delta = 0;
    if (unusedvar) {
      // Some variable was not used - create an int array mapping
      // old var index to new, remove unused vars from cvars, and
      // reset the literals using the mapping.
      mapping = (int32_t *) safe_malloc(numvars * sizeof(int32_t));
      j = 0;
      for (i = 0; i < numvars; i++) {
	if (cvars[i]->sort_index < 0) {
	  delta++;
	  mapping[i] = -1;
	} else {
	  cvars[i - delta] = cvars[i];
	  mapping[i] = i - delta;
	}
      }
      safe_realloc(cvars, (numvars-delta) * sizeof(var_entry_t *));
      for (i = 0; i < clause->num_lits; i++) {
	atom = clause->literals[i]->atom;
	arity = pred_arity(atom->pred, pred_table);
	// Loop through the args of the atom
	for (j = 0; j < arity; j++) {
	  if (atom->args[j] < 0) {
	    // We have a variable - vidx is the index into vars
	    vidx = -(atom->args[j] + 1);
	    atom->args[j] = -(mapping[vidx] + 1);
	  }
	}
      }
    }
    clause->num_vars = numvars-delta;
    clause->vars = cvars;
  } else {
    clause->num_vars = 0;
    clause->vars = NULL;
    free_var_entries(cvars);
  }
}
      
      

// Generates the CNF form of a formula, and updates the clause table (if no
// variables) or the rule table.

void add_cnf(input_formula_t *formula, double weight) {
  rule_table_t *rule_table = &(samp_table.rule_table);
  int32_t i, j, current_rule, num_lits, num_vars, atom_idx;
  rule_literal_t ***lits, *lit;
  samp_rule_t *clause;

  // Returns the literals, and sets the sort of the variables
  lits = cnf_pos(formula->fmla, formula->vars);
  
  if (formula->vars == NULL) {
    // No variables, just assert the clauses
    
    for (i = 0; lits[i] != NULL; i++) {
      for (num_lits = 0; lits[i][num_lits] != NULL; num_lits++) {}
      clause_buffer_resize(num_lits);
      for (j = 0; j < num_lits; j++) {
	lit = lits[i][j];
	atom_idx = add_internal_atom(&samp_table, lit->atom);
	clause_buffer.data[j] = lit->neg ? neg_lit(atom_idx) : pos_lit(atom_idx);
      }
      add_internal_clause(&samp_table, clause_buffer.data, num_lits, weight);
    }
  } else {
    for (num_vars = 0; formula->vars[num_vars] != NULL; num_vars++) {}
    for (i = 0; i < num_vars; i++) {
      // Check that all variables have been referenced
      if (formula->vars[i]->sort_index == -1) {
	printf("cnf error: variable %s not used in formula\n",
	       formula->vars[i]->name);
	safe_free(lits);
	return;
      }
    }
    // Now create the samp_rule_t, and add it to the rule_table
    for (i = 0; lits[i] != NULL; i++) {
      clause = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
      for (num_lits = 0; lits[i][num_lits] != NULL; num_lits++) {}
      clause->num_lits = num_lits;
      clause->literals = lits[i];
      //set_clause_variables(clause);
      clause->num_vars = num_vars;
      set_fmla_clause_variables(clause, formula->vars);
      clause->weight = weight;
      rule_table_resize(rule_table);
      current_rule = rule_table->num_rules;      
      rule_table->samp_rules[current_rule] = clause;
      rule_table->num_rules++;
      all_rule_instances(current_rule, &samp_table);
    }
    safe_free(lits);
  }
}


// Generates the CNF form of a formula, and updates the query and
// query_instance tables.

// void ask_cnf(input_formula_t *formula, double weight) {
//   query_table_t *query_table = &(samp_table.query_table);
//   query_instance_table_t *query_instance_table = &(samp_table.query_instance_table);
//   int32_t i, j, current_rule, num_lits, num_vars, atom_idx;
//   rule_literal_t ***lits, *lit;
//   samp_rule_t *clause;

//   // Returns the literals, and sets the sort of the variables
//   lits = cnf_pos(formula->fmla, formula->vars);
  
//   if (formula->vars == NULL) {
//     // No variables, just add to query_instance tables
    
//     for (i = 0; lits[i] != NULL; i++) {
//       for (num_lits = 0; lits[i][num_lits] != NULL; num_lits++) {}
//       clause_buffer_resize(num_lits);
//       for (j = 0; j < num_lits; j++) {
// 	lit = lits[i][j];
// 	atom_idx = add_internal_atom(&samp_table, lit->atom);
// 	clause_buffer.data[j] = lit->neg ? neg_lit(atom_idx) : pos_lit(atom_idx);
//       }
//       add_to_query_instance_table(&samp_table, clause_buffer.data, num_lits);
//     }
//   } else {
//     for (num_vars = 0; formula->vars[num_vars] != NULL; num_vars++) {}
//     for (i = 0; i < num_vars; i++) {
//       // Check that all variables have been referenced
//       if (formula->vars[i]->sort_index == -1) {
// 	printf("cnf error: variable %s not used in formula\n",
// 	       formula->vars[i]->name);
// 	safe_free(lits);
// 	return;
//       }
//     }
//     // Now create the samp_rule_t, and add it to the rule_table
//     for (i = 0; lits[i] != NULL; i++) {
//       clause = (samp_rule_t *) safe_malloc(sizeof(samp_rule_t));
//       for (num_lits = 0; lits[i][num_lits] != NULL; num_lits++) {}
//       clause->num_lits = num_lits;
//       clause->literals = lits[i];
//       //set_clause_variables(clause);
//       clause->num_vars = num_vars;
//       clause->vars = formula->vars;
//       clause->weight = weight;
//       rule_table_resize(rule_table);
//       current_rule = rule_table->num_rules;      
//       rule_table->samp_rules[current_rule] = clause;
//       rule_table->num_rules++;
//       if (!lazy_mcsat) {
// 	all_rule_instances(current_rule, &samp_table);
//       }
//     }
//     safe_free(lits);
//   }
// }
