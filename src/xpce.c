#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <string.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

#include "xmlrpc-config.h"  /* information about this build environment */
#include "memalloc.h"
#include "utils.h"
#include "print.h"
#include "input.h"
#include "mcsat.h"
#include "yacc.tab.h"

static xmlrpc_value *
xpce_sort(xmlrpc_env * const envP,
	  xmlrpc_value * const paramArrayP,
	  void * const serverInfo ATTR_UNUSED,
	  void * const channelInfo ATTR_UNUSED) {

  const char *sort;
  /* Parse our argument array. */
  fprintf(stderr, "Parsing array\n");
  xmlrpc_decompose_value(envP, paramArrayP, "(s)", &sort);
  if (envP->fault_occurred)
    return NULL;

  fprintf(stderr, "Got sort %s\n", sort);
  //xmlrpc_read_string(envP, xsort, &sort);
  add_sort(&samp_table.sort_table, sort);
  fprintf(stderr, "Added sort %s\n", sort);
  return xmlrpc_build_value(envP, "i", 0);
  //return NULL;
}

static xmlrpc_value *
xpce_predicate(xmlrpc_env * const envP,
	       xmlrpc_value * const paramArrayP,
	       void * const serverInfo ATTR_UNUSED,
	       void * const channelInfo ATTR_UNUSED) {

  xmlrpc_value *xsorts, *xsort;
  const char *pred;
  const char **sorts;
  const bool *directp;
  int32_t i, sort_size;
  /* Parse our argument array. */
  xmlrpc_decompose_value(envP, paramArrayP, "(sAb)",
			 &pred, &xsorts, &directp);
  if (envP->fault_occurred)
    return NULL;
  
  fprintf(stderr, "Got pred %s\n", pred);
  //xmlrpc_read_string(envP, xpred, &pred);
  sort_size = xmlrpc_array_size(envP, xsorts);
  fprintf(stderr, "Sort size is %d\n", sort_size);
  sorts = safe_malloc((sort_size + 1) * sizeof(char *));
  for (i = 0; i < sort_size; i++) {
    fprintf(stderr, "Reading %d sort\n", i);
    xmlrpc_array_read_item(envP, xsorts, i, &xsort);
    fprintf(stderr, "Adding to sorts[%d]\n", i);
    xmlrpc_read_string(envP, xsort, &sorts[i]);
  }
  sorts[i] = NULL;
  fprintf(stderr, "Adding predicate\n");
  add_predicate(pred, sorts, directp, &samp_table);
  safe_free(sorts);
  return xmlrpc_build_value(envP, "i", 0);
}


static xmlrpc_value *
xpce_const(xmlrpc_env * const envP,
	   xmlrpc_value * const paramArrayP,
	   void * const serverInfo ATTR_UNUSED,
	   void * const channelInfo ATTR_UNUSED) {

  char *cnst, *sort;

  /* Parse our argument array. */
  xmlrpc_decompose_value(envP, paramArrayP, "(ss)", &cnst, &sort);
  if (envP->fault_occurred)
    return NULL;
  
  fprintf(stderr, "Got const %s of sort %s\n", cnst, sort);
  add_constant(cnst, sort, &samp_table);
  return xmlrpc_build_value(envP, "i", 0);
}


static xmlrpc_value *
xpce_assert(xmlrpc_env * const envP,
	    xmlrpc_value * const paramArrayP,
	    void * const serverInfo ATTR_UNUSED,
	    void * const channelInfo ATTR_UNUSED) {

  xmlrpc_value *xsorts, *xsort;
  const char *pred;
  const char **sorts;
  const bool *directp;
  int32_t i, sort_size;
  /* Parse our argument array. */
  xmlrpc_decompose_value(envP, paramArrayP, "(sAb)",
			 &pred, &xsorts, &directp);
  if (envP->fault_occurred)
    return NULL;
  
  fprintf(stderr, "Got pred %s\n", pred);
  //xmlrpc_read_string(envP, xpred, &pred);
  sort_size = xmlrpc_array_size(envP, xsorts);
  fprintf(stderr, "Sort size is %d\n", sort_size);
  sorts = safe_malloc((sort_size + 1) * sizeof(char *));
  for (i = 0; i < sort_size; i++) {
    fprintf(stderr, "Reading %d sort\n", i);
    xmlrpc_array_read_item(envP, xsorts, i, &xsort);
    fprintf(stderr, "Adding to sorts[%d]\n", i);
    xmlrpc_read_string(envP, xsort, &sorts[i]);
  }
  sorts[i] = NULL;
  fprintf(stderr, "Adding predicate\n");
  add_predicate(pred, sorts, directp, &samp_table);
  safe_free(sorts);
  return xmlrpc_build_value(envP, "i", 0);
}


static xmlrpc_value *
xpce_dumptable(xmlrpc_env * const envP,
	       xmlrpc_value * const paramArrayP,
	       void * const serverInfo ATTR_UNUSED,
	       void * const channelInfo ATTR_UNUSED) {

  const char *table;
  int32_t tbl;
  /* Parse our argument array. */
  xmlrpc_decompose_value(envP, paramArrayP, "(s)", &table);
  if (envP->fault_occurred)
    return NULL;
  
  fprintf(stderr, "Got dumptable %s\n", table);
  if (strcasecmp(table,"all") == 0) {
    tbl = ALL;
  } else if (strcasecmp(table,"sort") == 0) {
    tbl = SORT;
  } else if (strcasecmp(table,"predicate") == 0) {
    tbl = PREDICATE;
  } else if (strcasecmp(table,"atom") == 0) {
    tbl = ATOM;
  } else if (strcasecmp(table,"clause") == 0) {
    tbl = CLAUSE;
  } else if (strcasecmp(table,"rule") == 0) {
    tbl = RULE;
  } else {
    return NULL;
  }
  // This just prints out at the server end - need to send the string
  dumptable(tbl, &samp_table);
  return xmlrpc_build_value(envP, "i", 0);
}


int main(int const argc, const char ** const argv) {
  
  init_samp_table(&samp_table);

  struct xmlrpc_method_info3 const methodInfoSort
    = {"xpce.sort", &xpce_sort};
  struct xmlrpc_method_info3 const methodInfoPredicate
    = {"xpce.predicate",&xpce_predicate};
  struct xmlrpc_method_info3 const methodInfoConst
    = {"xpce.const",&xpce_const};
  struct xmlrpc_method_info3 const methodInfoDumptable
    = {"xpce.dumptable",&xpce_dumptable};
  
  xmlrpc_server_abyss_parms serverparm;
  xmlrpc_registry * registryP;
  xmlrpc_env env;
  
  if (argc-1 != 1) {
    fprintf(stderr, "You must specify 1 argument:  The TCP port "
	    "number on which the server will accept connections "
	    "for RPCs (8080 is a common choice).  "
	    "You specified %d arguments.\n",  argc-1);
    exit(1);
  }

  // Initialize the error structure
  xmlrpc_env_init(&env);
  
  registryP = xmlrpc_registry_new(&env);
  
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoSort);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoPredicate);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoConst);
  xmlrpc_registry_add_method3(&env, registryP, &methodInfoDumptable);
  
  /* In the modern form of the Abyss API, we supply parameters in memory
     like a normal API.  We select the modern form by setting
     config_file_name to NULL: 
  */
  serverparm.config_file_name = NULL;
  serverparm.registryP        = registryP;
  serverparm.port_number      = atoi(argv[1]);
  serverparm.log_file_name    = "/tmp/xmlpce_log";

  printf("Running XPCE server...\n");

  xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));
  
  /* xmlrpc_server_abyss() never returns */
  
  return 0;
}
