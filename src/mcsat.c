/* The main for mcsat - the read_eval_print_loop is in input.c */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "samplesat.h"
#include "tables.h"
#include "input.h"
#include "print.h"

int main(int argc, char *argv[]) {
  samp_table_t table;
  char *curdir;
  int32_t c, i;

  while ((c = getopt(argc, argv, "v:")) != -1)
    switch (c) {
    case 'v':
      if (strlen(optarg) > 0) {
	for (i = 0; i < strlen(optarg); i++) {
	  if (!isdigit(optarg[i])) {
	    printf("Verbosity must be a number\n");
	    return 1;
	  }
	}
      } else {
	printf("Verbosity must be a number\n");
	return 1;
      }
      printf("Setting verbosity to %d\n", atoi(optarg));
      set_verbosity_level(atoi(optarg));
      break;
    case '?':
      if (optopt == 'c') {
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      } else if (isprint (optopt)) {
	fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      } else {
	fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
      }
      return 1;
    default:
      abort();
    }
  init_samp_table(&table);
  for (i = optind; i < argc; i++) {
    if (access(argv[i], R_OK) == 0) {
      printf("Loading %s\n", argv[i]);
      load_mcsat_file(argv[i], &table);
    } else {
      err(1, "File %s\n", argv[i]);
    }
  }
  // Now go interactive
  read_eval_print_loop(stdin, &table);
  printf("Exiting MCSAT\n");
  return 0;
}
