#include <stdlib.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "xmlrpc-config.h"  /* information about this build environment */

#define NAME "XPCE Test Client"
#define VERSION "1.0"

static void 
die_if_fault_occurred (xmlrpc_env * const envP) {
    if (envP->fault_occurred) {
        fprintf(stderr, "XML-RPC Fault: %s (%d)\n",
                envP->fault_string, envP->fault_code);
        exit(1);
    }
}



int 
main(int const argc, const char ** const argv ATTR_UNUSED) {

    xmlrpc_env env;
    xmlrpc_value * resultP;
    char *result;
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

    fprintf(stderr, "Adding sort email\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "sort email;");
    die_if_fault_occurred(&env);
    
    fprintf(stderr, "Adding sort task\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "sort task;");
    die_if_fault_occurred(&env);
    
    fprintf(stderr, "Adding sort person\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "sort person;");
    die_if_fault_occurred(&env);
    
    fprintf(stderr, "Adding predicate emailfrom\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)",
		       "predicate emailfrom(email,person) direct;");
    die_if_fault_occurred(&env);
    
    fprintf(stderr, "Adding predicate hastask\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)",
		       "predicate hastask(email,task) indirect;");
    fprintf(stderr, "Adding email constants\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "const em1,em2: email;");
    fprintf(stderr, "Adding task constants\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "const ta1,ta2: task;");
    fprintf(stderr, "Adding person constants\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "const pe1,pe2: person;");
    fprintf(stderr, "Adding assertion\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "assert emailfrom(em1,pe1);");
    fprintf(stderr, "Adding rule\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)",
		       "add [e] emailfrom(em,pe1) implies hastask(em, ta1) 2;");
    fprintf(stderr, "Running mcsat\n");
    xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "mcsat;");
    fprintf(stderr, "Querying\n");
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)",
		       "ask [e, p] emailfrom(e, p) and hastask(e, ta1);");
    xmlrpc_parse_value(&env, resultP, "s", &result);
    printf(result);    
    fprintf(stderr, "Dumping tables\n");
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.command", "(s)", "dumptable;");
    xmlrpc_parse_value(&env, resultP, "s", &result);
    printf(result);
    xmlrpc_DECREF(resultP);
    
    /* Clean up our error-handling environment. */
    xmlrpc_env_clean(&env);
    
    /* Shutdown our XML-RPC client library. */
    xmlrpc_client_cleanup();
    fprintf(stderr, "done\n");

    return 0;
}

