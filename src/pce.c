// Needs inlcude dirs for libicl.h, glib.h, e.g.,
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/include \
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1/glib \
// -I /homes/owre/src/oaa2.3.2/lib/x86-linux/glib-2.0/include	\
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <libicl.h>
#include <liboaa.h>
#include "utils.h"
#include "mcsat.h"
#include "yacc.tab.h"
#include "samplesat.h"
#include "memalloc.h"

#define AGENT_NAME "PCE"

int pce_fact_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions);
int setup_oaa_connection(int argc, char *argv[]);

int main(int argc, char *argv[]){
  // Initialize OAA
  oaa_Init(argc, argv);

  samp_table_t table;
  init_samp_table(&table);
  sort_table_t *sort_table = &(table.sort_table);
  const_table_t *const_table = &(table.const_table);
  var_table_t *var_table = &(table.var_table);
  pred_table_t *pred_table = &(table.pred_table);
  //atom_table_t *atom_table = &(table.atom_table);
  //clause_table_t *clause_table = &(table.clause_table);
  /* Create the symbol table */
  stbl_t symbol_table_r;
  stbl_t *symbol_table = &symbol_table_r;
  init_stbl(symbol_table, 0);
  if (!setup_oaa_connection(argc, argv)) {
    printf("Could not set up OAA connections...exiting\n");
    return EXIT_FAILURE;
  }
  oaa_MainLoop(TRUE);
  return EXIT_SUCCESS;
}

/**
 * Setups up the the connection between this agent
 * and the facilitator.
 * @param argc passed in but not used in this implementation
 * @param argv passed in but not used in this implementation
 * @return TRUE if successful
 */
int setup_oaa_connection(int argc, char *argv[]) {
  ICLTerm* pceSolvablesAsList = NULL;

  if (!oaa_SetupCommunication(AGENT_NAME)) {
    printf("Could not connect\n");
    return FALSE;
  }

  // Prepare a list of solvables that this agent is capable of
  // handling.
  pceSolvablesAsList
    = icl_NewTermFromString("[solvable(pce_fact(Source,Formula), \
                                [callback(pce_fact)])]");

  // Register solvables with the Facilitator.
  // The string "parent" represents the Facilitator.
  if (!oaa_Register("parent", AGENT_NAME, pceSolvablesAsList)) {
    printf("Could not register\n");
    return FALSE;
  }

  // Register the callback.
  if (!oaa_RegisterCallback("pce_fact", pce_fact_callback)) {
    printf("Could not register add callback\n");
    return FALSE;
  }

  // Clean up.
  icl_Free(pceSolvablesAsList);

  return TRUE;
}

/**
 * Satisfies <code>add(Addend1, Addendd2, Sum)</code> goals.
 * @param goal the goal
 * @param params parameters
 * @param solutions the solutions list; this callback
 * will add the satisfied goal to this list if the callback is
 * successful.
 * @return TRUE if successful
 */
int pce_fact_callback(ICLTerm* goal, ICLTerm* params, ICLTerm* solutions) {

  // The first addend is the first term
  // in "goal". The "goal" struct
  // is in the form "pce_fact(Source,Formula)".
  ICLTerm *Source = icl_CopyTerm(icl_NthTerm(goal, 1));

  // Repeat for the Formula
  ICLTerm *Formula = icl_CopyTerm(icl_NthTerm(goal, 2));

  // Add the fact
  printf("Adding fact for %s, %s\n", icl_NewStringFromTerm(Source),
	 icl_NewStringFromTerm(Formula));
  
  return TRUE;
}
