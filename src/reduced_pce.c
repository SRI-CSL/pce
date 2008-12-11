/* Needs inlcude dirs for libicl.h, glib.h, e.g.,
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/include \
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1/glib \
// -I /homes/owre/src/oaa2.3.2/lib/x86-linux/glib-2.0/include \
// -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1

// And for linking, needs this (doesn't work on 64-bit at the moment,
//   unless you recompile OAA)
// -L /homes/owre/src/oaa2.3.2/lib/x86-linux -loaa2
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <libicl.h>
#include <liboaa.h>
#include "memsize.h"

#define AGENT_NAME "reduced-pce"

static bool keep_looping;
static time_t pce_timeout = 60;
static time_t idle_timer = 0;
static int counter = 5;


/*
 * Idle callback: do nothing
 */
int pce_idle_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  time_t curtime;

  curtime = time(&curtime);
  if (difftime(curtime, idle_timer) > pce_timeout) {
    printf("Memory used so far: %.2f MB\n", mem_size()/(1024 * 1024));
    fflush(stdout);
    time(&idle_timer);

    counter --;
    keep_looping = counter > 0;
  }

  return true;
}



/*
 * Callback for pce_fact   
 * This takes a Source (i.e., learner id), and a Formula, and adds the
 * formula as a fact.  The Source is currently ignored.  The Formula must be
 * an atom.  The predicate must have been previously declared (generally in
 * pce-init.pce), and its sorts are used to declare any new constants.
 */
int pce_fact_callback(ICLTerm *goal, ICLTerm *params, ICLTerm *solutions) {
  // do nothing and return. This should never be executed anyway
  return true;
}




/**
 * Setups up the the connection between this agent
 * and the facilitator.
 * @param argc passed in but not used in this implementation
 * @param argv passed in but not used in this implementation
 * @return true if successful
 */
int setup_oaa_connection(int argc, char *argv[]) {
  ICLTerm *pceSolvables;
  bool code;

  printf("Setting up OAA connection\n");
  if (!oaa_SetupCommunication(AGENT_NAME)) {
    printf("Could not connect\n");
    return false;
  }

  // Prepare a list of solvables that this agent is capable of
  // handling.
  printf("Setting up solvables\n");
  pceSolvables = icl_NewList(NULL); 
  icl_AddToList(pceSolvables,
		icl_NewTermFromString("solvable(pce_fact(Source,Formula),[callback(pce_fact)])"),
		true);

  code = false;

  // Register solvables with the Facilitator.
  // The string "parent" represents the Facilitator.
  printf("Registering solvables\n");
  if (!oaa_Register("parent", AGENT_NAME, pceSolvables)) {
    printf("Could not register\n");
    goto done;
  }

  // Register the callbacks.
  if (!oaa_RegisterCallback("pce_fact", pce_fact_callback)) {
    printf("Could not register pce_fact callback\n");
    goto done;
  }

  if (!oaa_RegisterCallback("app_idle", pce_idle_callback)) {
    printf("Could not register idle callback\n");
    goto done;
  }

  // This seems to be ignored
  oaa_SetTimeout(pce_timeout);

  
  code = true;

 done:
  // Cleanup
  printf("Freeing solvables\n");
  icl_Free(pceSolvables);

  return code;
}


int main(int argc, char *argv[]){
  ICLTerm *Event, *Params;

  // Initialize OAA
  printf("Initializing OAA\n");
  oaa_Init(argc, argv);
  printf("Memory used so far: %.2f MB\n", mem_size()/(1024 * 1024));
  fflush(stdout);

  if (!setup_oaa_connection(argc, argv)) {
    printf("Could not set up OAA connections...exiting\n");
    return EXIT_FAILURE;
  }

  printf("Memory used so far: %.2f MB\n", mem_size()/(1024 * 1024));
  fflush(stdout);

  printf("Entering MainLoop\n");

  //  oaa_MainLoop(true); This expands to the code below (more or less)

  keep_looping = true;
  while (keep_looping) {
    oaa_GetEvent(&Event, &Params, 0);
    CHECK_LEAKS();
    oaa_ProcessEvent(Event, Params);
    CHECK_LEAKS();
    icl_Free(Event);
    icl_Free(Params);
  }

  oaa_Disconnect(NULL, NULL);

  return EXIT_SUCCESS;
}
