#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <json/json.h>
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

void print_result(struct json_object *resultJ) {
  struct json_object *err, *warn, *result;
  
  if ((err = json_object_object_get(resultJ, "error")) != NULL) {
    fprintf(stderr, "Error: %s\n", json_object_get_string(err));
  }
  if ((warn = json_object_object_get(resultJ, "warning")) != NULL) {
    fprintf(stderr, "Warning: %s\n", json_object_get_string(warn));
  }
  if ((result = json_object_object_get(resultJ, "result")) != NULL) {
    printf("Result: %s\n", json_object_to_json_string(result));
  }
}

void run_xpce_command(xmlrpc_env *const env,
		      const char *const serverUrl,
		      char *cmd,
		      struct json_object *arg) {
  xmlrpc_value * resultP;
  char *result;
  
  xmlrpc_env_init(env);
  fprintf(stderr, "Invoking %s\n", cmd);
  fflush(stderr);
  resultP = xmlrpc_client_call(env, serverUrl, cmd, "(s)",
			       json_object_to_json_string(arg));
  if (env->fault_occurred) {
    fprintf(stderr, "Error: %s\n", env->fault_string);
  } else {
    xmlrpc_decompose_value(env, resultP, "(s)", &result);
    print_result(json_tokener_parse(result));
    xmlrpc_DECREF(resultP);
  }
}
    

int main(int const argc, const char ** const argv ATTR_UNUSED) {
  xmlrpc_env env;
  char *serverUrl;
  struct json_object *cmd;

  struct xmlrpc_clientparms clientParms;
  struct xmlrpc_curl_xportparms curlParms;

  //xargs *xmlrpc_array_new(envP);
  
  if (argc-1 != 1) {
    fprintf(stderr, "You must specify 1 argument:  The URL "
	    "http://localhost:8080/RPC2 is a common choice."
	    "You specified %d arguments.\n",  argc-1);
    exit(1);
  }

  serverUrl = (char *) argv[1];
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

  // sort entity;
  cmd = json_tokener_parse("{\"name\": \"entity\"}");
  run_xpce_command(&env, serverUrl, "xpce.sort", cmd);
  // subsort email entity;
  cmd = json_tokener_parse("{\"name\": \"email\", \"super\": \"entity\"}");
  run_xpce_command(&env, serverUrl, "xpce.sort", cmd);
  // subsort task entity;
  cmd = json_tokener_parse("{\"name\": \"task\", \"super\": \"entity\"}");
  run_xpce_command(&env, serverUrl, "xpce.sort", cmd);
  // subsort person entity;
  cmd = json_tokener_parse("{\"name\": \"person\", \"super\": \"entity\"}");
  run_xpce_command(&env, serverUrl, "xpce.sort", cmd);
  // predicate emailfrom(email,person) direct;
  cmd = json_tokener_parse("{\"predicate\": \"emailfrom\", \"arguments\": [\"email\", \"person\"], \"observable\": true}");
  run_xpce_command(&env, serverUrl, "xpce.predicate", cmd);
  // predicate hastask(email,task) indirect;
  cmd = json_tokener_parse("{\"predicate\": \"hastask\", \"arguments\": [\"email\", \"task\"], \"observable\": false}");
  run_xpce_command(&env, serverUrl, "xpce.predicate", cmd);
  // const em1,em2: email;
  cmd = json_tokener_parse("{\"names\": [\"em1\", \"em2\"], \"sort\": \"email\"}");
  run_xpce_command(&env, serverUrl, "xpce.const", cmd);
  // const ta1,ta2: task;
  cmd = json_tokener_parse("{\"names\": [\"ta1\", \"ta2\"], \"sort\": \"task\"}");
  run_xpce_command(&env, serverUrl, "xpce.const", cmd);
  // const pe1,pe2: person;
  cmd = json_tokener_parse("{\"names\": [\"pe1\", \"pe2\"], \"sort\": \"person\"}");
  run_xpce_command(&env, serverUrl, "xpce.const", cmd);
  // assert emailfrom(em1,pe1);
  cmd = json_tokener_parse("{\"fact\": {\"predicate\": \"emailfrom\", \"arguments\": [\"em1\", \"pe1\"]}}");
  run_xpce_command(&env, serverUrl, "xpce.assert", cmd);
  // Should get an error
  cmd = json_tokener_parse("{\"formula\": \"(married($X, $Y) => likes($X, $Y))\", \"weight\": 2}");
  run_xpce_command(&env, serverUrl, "xpce.add", cmd);
  // add [em] emailfrom(em,pe1) implies hastask(em, ta1) 2;
  cmd = json_tokener_parse("{\"formula\": {\"implies\": [{\"atom\": {\"predicate\": \"emailfrom\", \"arguments\": [{\"var\": \"em\"}, {\"const\": \"pe1\"}]}}, {\"atom\": {\"predicate\": \"hastask\", \"arguments\": [{\"var\": \"em\"}, {\"const\": \"ta1\"}]}}]}, \"weight\": 2}");
  run_xpce_command(&env, serverUrl, "xpce.add", cmd);
  // mcsat;
  run_xpce_command(&env, serverUrl, "xpce.mcsat", NULL);
  // ask [e, p] emailfrom(e, p) and hastask(e, ta1);
  cmd = json_tokener_parse("{\"formula\": {\"and\": [{\"atom\": {\"predicate\": \"emailfrom\", \"arguments\": [{\"var\": \"e\"}, {\"var\": \"p\"}]}}, {\"atom\": {\"predicate\": \"hastask\", \"arguments\": [{\"var\": \"e\"}, {\"const\": \"ta1\"}]}}]}, \"threshold\": 0.2}");
  run_xpce_command(&env, serverUrl, "xpce.ask", cmd);
  
  run_pce_command(&env, serverUrl, "dumptable;");
    
  /* Clean up our error-handling environment. */
  xmlrpc_env_clean(&env);
    
  /* Shutdown our XML-RPC client library. */
  xmlrpc_client_cleanup();
  xmlrpc_client_teardown_global_const();
  fprintf(stderr, "done\n");

  return 0;
}
