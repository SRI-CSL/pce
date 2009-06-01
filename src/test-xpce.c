#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <xmlrpc.h>
#include <xmlrpc_client.h>

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
  int32_t i;
  xmlrpc_env env;
  const char * const serverUrl = "http://localhost:8080/RPC2";

  struct xmlrpc_clientparms clientParms;
  struct xmlrpc_curl_xportparms curlParms;

  //xargs *xmlrpc_array_new(envP);

  if (argc-1 > 0) {
    fprintf(stderr, "This program has no arguments\n");
    exit(1);
  }

  /* Initialize our error-handling environment. */
  xmlrpc_env_init(&env);
  xmlrpc_client_setup_global_const(&env);

  curlParms.network_interface = NULL;
  curlParms.no_ssl_verifypeer = false;
  curlParms.no_ssl_verifyhost = false;
  curlParms.user_agent        = "XPCE Test Client/1.0";
 
  clientParms.transport          = "curl";
  clientParms.transportparmsP    = &curlParms;
  clientParms.transportparm_size = XMLRPC_CXPSIZE(user_agent);
 
  /* Start up our XML-RPC client library. */
  xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, &clientParms,
		      XMLRPC_CPSIZE(transportparm_size));
  die_if_fault_occurred(&env);

  run_pce_command(&env, serverUrl, "sort entity;");
  run_pce_command(&env, serverUrl, "subsort email entity;");
  run_pce_command(&env, serverUrl, "subsort task entity;");
  run_pce_command(&env, serverUrl, "subsort person entity;");
  run_pce_command(&env, serverUrl, "predicate emailfrom(email,person) direct;");
  run_pce_command(&env, serverUrl, "predicate hastask(email,task) indirect;");
  run_pce_command(&env, serverUrl, "const em1,em2: email;");
  run_pce_command(&env, serverUrl, "const ta1,ta2: task;");
  run_pce_command(&env, serverUrl, "const pe1,pe2: person;");
  run_pce_command(&env, serverUrl, "assert emailfrom(em1,pe1);");
  run_pce_command(&env, serverUrl,
		  "add [em] emailfrom(em,pe1) implies hastask(em, ta1) 2;");
  //for (i = 0; i < 100; i++) {
  run_pce_command(&env, serverUrl, "mcsat;");
  //}
  run_pce_command(&env, serverUrl, "ask [e, p] emailfrom(e, p) and hastask(e, ta1);");
  run_pce_command(&env, serverUrl, "dumptable;");
    
  /* Clean up our error-handling environment. */
  xmlrpc_env_clean(&env);
    
  /* Shutdown our XML-RPC client library. */
  xmlrpc_client_cleanup();
  xmlrpc_client_teardown_global_const();
  fprintf(stderr, "done\n");

  return 0;
}

