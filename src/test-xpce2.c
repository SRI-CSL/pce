#include <stdlib.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "xmlrpc-config.h"  /* information about this build environment */

#define NAME "XPCE Test Client"
#define VERSION "1.0"

static void die_if_fault_occurred (xmlrpc_env * const envP) {
    if (envP->fault_occurred) {
        fprintf(stderr, "XML-RPC Fault: %s (%d)\n",
                envP->fault_string, envP->fault_code);
        exit(1);
    }
}

void run_pce_command (xmlrpc_env * const env, const char * const serverUrl, char *cmd) {
  xmlrpc_value * resultP;
  char *result;
  
  xmlrpc_env_init(env);
  fprintf(stderr, "Invoking %s\n", cmd);
  fflush(stderr);
  resultP = xmlrpc_client_call(env, serverUrl, "xpce.command", "(s)", cmd);
  if (env->fault_occurred) {
    fprintf(stderr, "Error: %s\n", env->fault_string);
  } else {
    xmlrpc_parse_value(env, resultP, "s", &result);
    printf(result);    
    xmlrpc_DECREF(resultP);
  }
}

int main(int const argc, const char ** const argv ATTR_UNUSED) {

    xmlrpc_env env;
    int32_t i;
    const char * const serverUrl = "http://localhost:8080/RPC2";
    //xargs *xmlrpc_array_new(envP);

    if (argc-1 > 0) {
      fprintf(stderr, "This program has no arguments\n");
      exit(1);
    }

    /* Initialize our error-handling environment. */
    xmlrpc_env_init(&env);

    /* Start up our XML-RPC client library. */
    xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0);
    die_if_fault_occurred(&env);

    run_pce_command(&env, serverUrl, "const em3,em4: email;");
    run_pce_command(&env, serverUrl, "assert emailfrom(em4,pe2);");
    run_pce_command(&env, serverUrl,
		    "add [em] emailfrom(em,pe2) implies hastask(em, ta2) 4;");
    for (i = 0; i < 100; i++) {
      run_pce_command(&env, serverUrl, "mcsat;");
    }
    run_pce_command(&env, serverUrl, "ask [e, p] emailfrom(e, p) and hastask(e, ta2);");
    run_pce_command(&env, serverUrl, "dumptable;");
    
    /* Clean up our error-handling environment. */
    xmlrpc_env_clean(&env);
    
    /* Shutdown our XML-RPC client library. */
    xmlrpc_client_cleanup();
    fprintf(stderr, "done\n");

    return 0;
}

