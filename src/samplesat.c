#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "tables.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "utils.h"
#include "gcd.h"
#include "print.h"
#include "vectors.h"
#include "buffer.h"
#include "clause_list.h"
#include "yacc.tab.h"
#include "ground.h"
#include "samplesat.h"
#include "walksat.h"

/*
 * Initializes the variables for the first run of sample SAT. Only hard
 * clauses are considered. Randomizes all active atoms and their neighboors.
 *
 * TODO In the first run of sample SAT, only hard clauses are considered;
 * Want to generalize it to work for the following initialization of
 * sample SAT.
 */

/*
 * The initialization phase for the mcmcsat step. First place all the clauses
 * in the unsat list, and then use scan_live_clauses to move them into
 * sat_clauses or the watched (sat) lists.
 *
 * Like init_first_sample_sat, but takes an existing state and sets it up for a
 * second round of sampling
 *
 * @randomize: whether we start from a randomized assignment or the current
 * assignment
 */
static int32_t init_sample_sat(samp_table_t *table, bool randomize) {

	init_live_clauses(table);
	valid_table(table);

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] before unit propagation ...\n");
		print_assignment(table);
	}

	scan_live_clauses(table);
	valid_table(table);

	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] after unit propagation:\n");
		print_assignment(table);
	}

	if (randomize) {
		init_random_assignment(table);
		valid_table(table);
	}

	scan_live_clauses(table);
	valid_table(table);
	
	if (get_verbosity_level() >= 4) {
		printf("\n[init_first_sample_sat] after randomization:\n");
		print_assignment(table);
		print_live_clauses(table);
	}

	return 0;
}

/*
 * A step of sample sat, in which the value of a random variable is flipped
 *
 * @lazy: true if inference is lazy
 * @sa_probability: the probability of choosing a simulated annealing step,
 * otherwise choose a walksat step
 * @sa_temperature: (fixed) temperature of simulated annealing
 * @rvar_probability: probability of selecting a random variable in a walksat
 * step, otherwise select a variable that minimizes dcost
 */
static int32_t sample_sat_body(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability) {
	rule_inst_table_t *rule_inst_table = &(table->rule_inst_table);
	atom_table_t *atom_table = &(table->atom_table);
	int32_t dcost;
	int32_t var;
	int32_t conflict = 0;

	valid_table(table);

	double choice = choose();
	cprintf(3, "\n[sample_sat_body] num_unsat = %d, choice = % .4f, sa_probability = % .4f\n",
			rule_inst_table->unsat_clauses.length, choice, sa_probability);

	assert(rule_inst_table->unsat_clauses.length >= 0);
	/* Simulated annlealing vs. walksat */
	if (rule_inst_table->unsat_clauses.length == 0 || choice < sa_probability) {
		/* Simulated annealing step */
		cprintf(3, "[sample_sat_body] simulated annealing\n");

		if (!lazy) {
			var = choose_unfixed_variable(atom_table);
		} else {
			var = choose_random_atom(table);
		}

		if (var == -1 || !unfixed_tval(atom_table->assignment[var])) {
			return 0;
		}
		
		cost_flip_unfixed_variable(table, var, &dcost);
		cprintf(3, "[sample_sat_body] simulated annealing num_unsat = %d, var = %d, dcost = %d\n",
				rule_inst_table->unsat_clauses.length, var, dcost);
		if (dcost <= 0) {
			conflict = flip_unfixed_variable(table, var);
		} else {
			choice = choose();
			if (choice < exp(-dcost / sa_temperature)) {
				conflict = flip_unfixed_variable(table, var);
			}
		}
	} else {
		/* Walksat step */
		cprintf(3, "[sample_sat_body] WalkSAT\n");

		/* Choose a random unsat clause */
		samp_clause_t *unsat_clause = choose_random_clause(&rule_inst_table->unsat_clauses);

		/* Choose an atom from the clause, with two strategies: random and greedy */
		var = var_of(unsat_clause->disjunct[choose_clause_var(table, unsat_clause,
				rvar_probability)]);

		if (get_verbosity_level() >= 3) {
			printf("[sample_sat_body] WalkSAT num_unsat = %d, var = %d, dcost = %d\n",
					rule_inst_table->unsat_clauses.length, var, dcost);
			printf("from unsat clause ");
			print_clause(unsat_clause, table);
			printf("\n");
		}
		conflict = flip_unfixed_variable(table, var);
	}
	return conflict;
}

/* 
 * After successfully get a model in samplesat, we remove the fix flag used by
 * unit propagation and only keep the truth values of the atoms.
 */
void unfix_assignment_array(atom_table_t *atom_table) {
	atom_table->num_unfixed_vars = 0;
	int32_t i;
	for (i = 0; i < atom_table->num_vars; i++) {
		atom_table->assignment[i] = unfix_tval(atom_table->assignment[i]);
		if (unfixed_tval(atom_table->assignment[i])) {
			atom_table->num_unfixed_vars++;
		}
	}
}

/*
 * A SAT solver for only the hard formulas
 */
int32_t first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability, uint32_t max_flips) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t conflict;

	conflict = init_sample_sat(table, true);

	if (conflict == -1)
		return -1;

	uint32_t num_flips = max_flips;
	while (rule_inst_table->unsat_clauses.length > 0 && num_flips > 0) {
		if (sample_sat_body(table, lazy, sa_probability, sa_temperature,
					rvar_probability) == -1)
			return -1;
		num_flips--;
	}
	
	unfix_assignment_array(atom_table);

	if (rule_inst_table->unsat_clauses.length > 0) {
		printf("num of unsat clauses = %d\n", rule_inst_table->unsat_clauses.length);
		mcsat_err("Initialization failed to find a model; increase max_flips\n");
		return -1;
	}

	if (get_verbosity_level() >= 1) {
		printf("\n[first_sample_sat] initial assignment:\n");
		print_assignment(table);
	}

	return 0;
}

/*
 * Next, need to write the main samplesat routine. How should hard clauses be
 * represented, with weight MAX_DOUBLE? The parameters include clause_set, DB,
 * KB, maxflips, p_anneal, anneal_temp, p_walk.
 *
 * The assignment will map each atom to undef, T, F, FixedT, FixedF, DB_T, or
 * DB_F. The latter assignments are fixed by the closed world assumption on the
 * DB. The other fixed assignments are those obtained from unit propagation on
 * the selected clauses.
 *
 * The samplesat routine starts with a random assignment to the non-fixed
 * variables and a selection of alive clauses. The cost is the number of unsat
 * clauses. It then either (with prob p_anneal) picks a variable and flips it
 * (if it reduces cost) or with probability (based on the cost diff).
 * Otherwise, it does a walksat step. The tricky question is what it means to
 * activate a clause or atom.
 *
 * Code for random selection with filtering is in smtcore.c
 * (select_random_bvar)
 *
 * First step is to write a unit-propagator. The propagation is done repeatedly
 * to handle recent additions.
 */
int32_t sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips, bool randomize) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t conflict;

	cprintf(2, "[sample_sat] started ...\n");

	conflict = init_sample_sat(table, randomize);

	uint32_t num_flips = max_flips;
	while (num_flips > 0 && conflict == 0) {
		if (rule_inst_table->unsat_clauses.length == 0) {
			if (max_extra_flips > 0) {
				max_extra_flips--;
			} else {
				break;
			}
		}
		conflict = sample_sat_body(table, lazy, sa_probability, sa_temperature,
				rvar_probability);
		//assert(valid_table(table));
		num_flips--;
	}

	/* If we successfully get a model, try to walk around to neighboring models;
	 * Note that if the model is isolated, it will remain unchanged. */
	if (conflict != -1 && rule_inst_table->unsat_clauses.length == 0) {
		num_flips = genrand_uint(max_extra_flips);
		while (num_flips > 0 && conflict == 0) {
			/* low temperature means stable */
			conflict = sample_sat_body(table, lazy, 1, 0.01, rvar_probability);
			//assert(valid_table(table));
			num_flips--;
		}
	}
	
	unfix_assignment_array(atom_table);

	return conflict;
}

