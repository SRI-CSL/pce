/* The main for mcsat - the read_eval_print_loop is in input.c */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <getopt.h>
#include "samplesat.h"
#include "tables.h"
#include "input.h"
#include "print.h"
#include "mcsat.h"
#include "SFMT.h"
#ifdef _WIN32
#include <io.h>
#endif

// What kind of versioning system should we have?

#define VERSION "0.1.2"

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
	SEED_OPTION,
	DUMP_SAMPLES_OPTION,
	MAX_SAMPLES_OPTION,
	SA_PROBABILITY_OPTION,
	SA_TEMPERATURE_OPTION,
	RVAR_PROBABILITY_OPTION,
	MAX_FLIPS_OPTION,
	MAX_EXTRA_FLIPS_OPTION,
	TIMEOUT_OPTION,
	BURN_IN_STEPS_OPTION,
	SAMP_INTERVAL_OPTION,
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
	{"dumpsamples", required_argument, 0, DUMP_SAMPLES_OPTION},
	{"strict", required_argument, 0, STRICT_OPTION},
	{"prexp", no_argument, 0, 'e'},
	{"verbosity", required_argument, 0, 'v'},
	{"version", no_argument, &show_version, 1},
	{"max_samples", required_argument, 0, MAX_SAMPLES_OPTION},
	{"sa_probability", required_argument, 0, SA_PROBABILITY_OPTION},
	{"sa_temperature", required_argument, 0, SA_TEMPERATURE_OPTION},
	{"rvar_probability", required_argument, 0, RVAR_PROBABILITY_OPTION},
	{"max_flips", required_argument, 0, MAX_FLIPS_OPTION},
	{"max_extra_flips", required_argument, 0, MAX_EXTRA_FLIPS_OPTION},
	{"timeout", required_argument, 0, TIMEOUT_OPTION},
	{"burn_in_steps", required_argument, 0, BURN_IN_STEPS_OPTION},
	{"samp_interval", required_argument, 0, SAMP_INTERVAL_OPTION},
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
  -h, -?, --help                   print this help and exit\n\
  -i,     --interactive            run interactively\n\
  -s,     --seed=NUM               use given random seed\n\
          --lazy=BOOL              whether to use lazy version (true)\n\
          --dumpsamples=BOOL       whether to dump samples\n\
          --strict=BOOL            whether to require declarations for all constants (true)\n\
  -e,     --prexp                  whether to print using 'e' notation\n\
  -v,     --verbosity=NUM          sets the verbosity level\n\
  -V,     --version                prints the version number and exits\n\
          --max_samples=NUM        number of samples\n\
          --sa_probability=NUM     probability of selecting a SA step\n\
          --sa_temperature=NUM     temperature of SA\n\
          --rvar_probability=NUM   probability of random strategy in WalkSAT\n\
          --max_flips=NUM          maximum number of flips in SampleSAT\n\
          --max_extra_flips=NUM    maximum number of extra flips after finding a model\n\
          --timeout=NUM            MCSAT will terminate if exceeds timeout\n\
          --burn_in_steps=NUM      number of burn_in steps of MCSAT\n\
          --samp_interval=NUM      number of SampleSAT between two samples\n\
", program_name, program_name);
  exit(0);
}

#define OPTION_STRING "ihs:eVv:"

// static void set_subcommand_option (enum subcommand subcommand) {
//   if (subcommand_option != subcommand) {
//     fprintf(stderr, "You may not specify more than one '-vV?' option\n");
//     exit(1);
//   }
//   subcommand_option = subcommand;
// }

static void decode_options(int argc, char **argv) {
	int32_t i, optchar, option_index;
	char *endptr;
	double val;

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
		case SEED_OPTION: {
			for(i=0; optarg[i] != '\0'; i++){
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: seed must be an integer\n");
					exit(1);
				}
			}
			// srand(atoi(optarg));
			set_pce_rand_seed(atoi(optarg));
			break;
		}
		case MAX_SAMPLES_OPTION: {
			for(i=0; optarg[i] != '\0'; i++) {
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: max_samples must be an integer\n");
					exit(1);
				}
			}
			set_max_samples(atoi(optarg));
			break;
		}
		case SA_PROBABILITY_OPTION: {
			errno = 0;
			val = strtod(optarg, &endptr);
			if ((errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL))
					|| (errno != 0 && val == 0) || endptr == optarg || *endptr != '\0'
					|| val < 0.0 || val > 1.0) {
				mcsat_err("Error: sa_probability must be a float between 0.0 and 1.0");
				exit(1);
			}
			set_sa_probability(val);
			break;
		}
		case SA_TEMPERATURE_OPTION: {
			errno = 0;
			val = strtod(optarg, &endptr);
			if ((errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL))
					|| (errno != 0 && val == 0) || endptr == optarg || *endptr != '\0'
					|| val < 0.0) {
				mcsat_err("Error: sa_temperature must be a float >= 0.0");
				exit(1);
			}
			set_sa_temperature(val);
			break;
		}
		case RVAR_PROBABILITY_OPTION: {
			errno = 0;
			val = strtod(optarg, &endptr);
			if ((errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL))
					|| (errno != 0 && val == 0) || endptr == optarg || *endptr != '\0'
					|| val < 0.0 || val > 1.0) {
				mcsat_err("Error: rvar_probability must be a float between 0.0 and 1.0");
				exit(1);
			}
			set_rvar_probability(val);
			break;
		}
		case MAX_FLIPS_OPTION: {
			for(i=0; optarg[i] != '\0'; i++) {
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: max_flips must be an integer\n");
					exit(1);
				}
			}
			set_max_flips(atoi(optarg));
			break;
		}
		case MAX_EXTRA_FLIPS_OPTION: {
			for(i=0; optarg[i] != '\0'; i++) {
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: max_extra_flips must be an integer\n");
					exit(1);
				}
			}
			set_max_extra_flips(atoi(optarg));
			break;
		}
		case TIMEOUT_OPTION: {
			for(i=0; optarg[i] != '\0'; i++) {
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: timeout must be an integer\n");
					exit(1);
				}
			}
			set_mcsat_timeout(atoi(optarg));
			break;
		}
		case BURN_IN_STEPS_OPTION:
			for (i=0; optarg[i] != '\0'; i++) {
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: burn_in_steps must be a non-negative integer\n");
					exit(1);
				}
			}
			set_burn_in_steps(atoi(optarg));
			break;
		case SAMP_INTERVAL_OPTION:
			for (i=0; optarg[i] != '\0'; i++) {
				if (! isdigit(optarg[i])) {
					mcsat_err("Error: samp_interval must be a positive integer\n");
					exit(1);
				}
			}
			set_samp_interval(atoi(optarg));
			break;
		case DUMP_SAMPLES_OPTION:
			set_dump_samples_path(optarg);
			break;
		case LAZY_OPTION:
			if ((strcasecmp(optarg, "true") == 0) 
					|| (strcasecmp(optarg, "t") == 0) 
					|| (strcmp(optarg, "1") == 0)) {
				set_lazy_mcsat(true);
			} else if ((strcasecmp(optarg, "false") == 0) 
					|| (strcasecmp(optarg, "f") == 0) 
					|| (strcmp(optarg, "0") == 0)) {
				set_lazy_mcsat(false);
			} else {
				mcsat_err("Error: lazy must be true, false, t, f, 0, or 1\n");
				exit(1);
			}
			break;
		case STRICT_OPTION:
			if ((strcasecmp(optarg, "true") == 0) 
					|| (strcasecmp(optarg, "t") == 0) 
					|| (strcmp(optarg, "1") == 0)) {
				set_strict_constants(true);
			} else if ((strcasecmp(optarg, "false") == 0) 
					|| (strcasecmp(optarg, "f") == 0) 
					|| (strcmp(optarg, "0") == 0)) {
				set_strict_constants(false);
			} else {
				mcsat_err("Error: strict must be true, false, t, f, 0, or 1\n");
				exit(1);
			}
			break;
		case 'e':
			set_print_exp_p(true);
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
	rand_reset(); // May be reset in options
	decode_options(argc, argv);
	init_samp_table(&samp_table);
	file_loaded = false;
	for (i = optind; i < argc; i++) {
		if (access(argv[i], R_OK) == 0) {
			printf("Loading %s\n", argv[i]);
			load_mcsat_file(argv[i], &samp_table);
			file_loaded = true;
		} else {
		  /* Strange problem.  I created this file:
		     'examples/threads/cancer1-2threads.pce'
		     permissions good, and fell through to here: */
			//err(1, "File %s\n", argv[i]);
			fprintf(stderr, "%s: No access to file %s\n", program_name, argv[i]);
			perror("main (access)");
			exit(1);
		}
	}
	if (interactive || !file_loaded) {
		// Now go interactive - empty string is for stdin
		printf("\n%s: type 'help;' for help\n", program_name);
		read_eval_print_loop("", &samp_table);
		printf("Exiting MCSAT\n");
	}
	return 0;
}
