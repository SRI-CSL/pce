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

    fprintf(stderr, "Making XMLRPC call to server url '%s'\n", serverUrl);
    
    fprintf(stderr, "Generating sort email\n");
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.sort", "(s)", "email");
    die_if_fault_occurred(&env);
    fprintf(stderr, "Generating sort task\n");
    xmlrpc_DECREF(resultP);
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.sort", "(s)", "task");
    die_if_fault_occurred(&env);
    xmlrpc_DECREF(resultP);
    
    fprintf(stderr, "Generating predicate\n");
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.predicate", "(s(ss)b)",
				 "hastask", "email", "task", 0);
    die_if_fault_occurred(&env);
    xmlrpc_DECREF(resultP);
    
    fprintf(stderr, "Generating constants\n");
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.const", "(ss)", "em1", "email");
    die_if_fault_occurred(&env);
    xmlrpc_DECREF(resultP);
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.const", "(ss)", "ta1", "task");
    die_if_fault_occurred(&env);
    xmlrpc_DECREF(resultP);

    fprintf(stderr, "Generating dumptable\n");
    resultP = xmlrpc_client_call(&env, serverUrl, "xpce.dumptable", "(s)", "all");
    die_if_fault_occurred(&env);
    xmlrpc_DECREF(resultP);
    
    /* Clean up our error-handling environment. */
    xmlrpc_env_clean(&env);
    
    /* Shutdown our XML-RPC client library. */
    xmlrpc_client_cleanup();

    return 0;
}

