/* The main for mcsat - the read_eval_print_loop is in input.c */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <getopt.h>
#include "samplesat.h"
#include "tables.h"
#include "input.h"
#include "print.h"

#define VERSION "0.1"

// Flags:
//  -v num  -- sets verbosity level
//  -c

samp_table_t samp_table;

static const char *program_name;
static int32_t show_version;
static int32_t show_help;
static int32_t interactive;

enum {
  LAZY_OPTION = CHAR_MAX + 1,
  STRICT_OPTION,
  SEED_OPTION
};

// enum subcommand {
//   UNKNOWN_SUBCOMMAND,
//   VERBOSITY_SUBCOMMAND
// };

static struct option long_options[] = {
  {"help", no_argument, (int *)&show_help, 'h'},
  {"interactive", no_argument, (int *)&interactive, 'i'},
  {"seed", required_argument, 0, SEED_OPTION},
  {"lazy", required_argument, 0, LAZY_OPTION},
  {"strict", required_argument, 0, STRICT_OPTION},
  {"verbosity", required_argument, 0, 'v'},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

// static enum subcommand subcommand_option;

// Print a usage message and exit
static void usage () {
  fprintf(stderr, "Try '%s --help' for more information.\n",
	  program_name);
  exit(1);
}

static void help () {
  printf("\
mcsat computes marginal probabilities based on facts and weighted assertions.\n\n\
Usage: %s [OPTION]... [FILE]...\n\n\
  Interactive If no FILE is provided,\n\
  Otherwise FILEs are read and %s exits without --interactive option\n\
Options:\n\
  -h, -?, --help           print this help and exit\n\
  -i,     --interactive    run interactively\n\
  -s,     --seed=NUM       use given random seed\n\
          --lazy=BOOL      whether to use lazy version (true)\n\
          --strict=BOOL    whether to require declarations for all constants (true)\n\
  -v,     --verbosity=NUM  sets the verbosity level\n\
  -V,     --version        prints the version number and exits\n\
", program_name, program_name);
  exit(0);
}

#define OPTION_STRING "ihs:Vv:"

// static void set_subcommand_option (enum subcommand subcommand) {
//   if (subcommand_option != subcommand) {
//     fprintf(stderr, "You may not specify more than one '-vV?' option\n");
//     exit(1);
//   }
//   subcommand_option = subcommand;
// }

static void decode_options(int argc, char **argv) {
  int32_t i, optchar, option_index;  

  option_index = 0;
  while ((optchar = getopt_long(argc, argv, OPTION_STRING, long_options, &option_index)) != -1) {
    switch (optchar) {
    case '?':
      usage();
    case 0:
      break;
    case 'i':
      interactive = true;
      break;
    case 'h':
      show_help = 1;
      break;
    case 's':
    case SEED_OPTION:
      for(i=0; optarg[i] != '\0'; i++){
	if (! isdigit(optarg[i])) {
	  mcsat_err("Error: seed must be an integer\n");
	  exit(1);
	}
      }
      srand(atoi(optarg));
      break;
    case LAZY_OPTION:
      if ((strcasecmp(optarg, "true") == 0) || (strcasecmp(optarg, "t") == 0) || (strcmp(optarg, "1") == 0)) {
	set_lazy_mcsat(true);
      } else if ((strcasecmp(optarg, "false") == 0) || (strcasecmp(optarg, "f") == 0) || (strcmp(optarg, "0") == 0)) {
	set_lazy_mcsat(false);
      } else {
	mcsat_err("Error: lazy must be true, false, t, f, 0, or 1\n");
	exit(1);
      }
      break;
    case STRICT_OPTION:
      if ((strcasecmp(optarg, "true") == 0) || (strcasecmp(optarg, "t") == 0) || (strcmp(optarg, "1") == 0)) {
	set_strict_constants(true);
      } else if ((strcasecmp(optarg, "false") == 0) || (strcasecmp(optarg, "f") == 0) || (strcmp(optarg, "0") == 0)) {
	set_strict_constants(false);
      } else {
	mcsat_err("Error: strict must be true, false, t, f, 0, or 1\n");
	exit(1);
      }
      break;
    case 'v':
      if (strlen(optarg) > 0) {
	for (i = 0; i < strlen(optarg); i++) {
	  if (!isdigit(optarg[i])) {
	    mcsat_err("Verbosity must be a number\n");
	    exit(1);
	  }
	}
      } else {
	mcsat_err("Verbosity must be a number\n");
	exit(1);
      }
      printf("Setting verbosity to %d\n", atoi(optarg));
      set_verbosity_level(atoi(optarg));
      break;
    default:
      usage();
    }
    if (show_version) {
      output("mcsat %s\nCopyright (C) 2008, SRI International.  All Rights Reserved.\n",
	     VERSION);
      exit(0);
    }
    if (show_help) {
      help();
    }
  }
}

// Main routine for mcsat
int main(int argc, char *argv[]) {
  int32_t i;
  bool file_loaded;

  program_name = argv[0];
  decode_options(argc, argv);
  init_samp_table(&samp_table);
  file_loaded = false;
  for (i = optind; i < argc; i++) {
    if (access(argv[i], R_OK) == 0) {
      printf("Loading %s\n", argv[i]);
      load_mcsat_file(argv[i], &samp_table);
      file_loaded = true;
    } else {
      err(1, "File %s\n", argv[i]);
    }
  }
  if (interactive || !file_loaded) {
    // Now go interactive - empty string is for stdin
    printf("\n\
%s: type 'help;' for help\n\
", program_name);
    read_eval_print_loop("", &samp_table);
    printf("Exiting MCSAT\n");
  }
  return 0;
}
