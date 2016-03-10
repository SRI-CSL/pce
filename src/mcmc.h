#ifndef __MCMC_H
#define __MCMC_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

/*
 * Top-level MCSAT call
 *
 * Parameters for sample sat:
 * - sa_probability = probability of a simulated annealing step
 * - sa_temperature = temperature for simulated annealing
 * - rvar_probability = probability used by a Walksat step:
 *   a Walksat step selects an unsat clause and flips one of its variables
 *   - with probability rvar_probability, that variable is chosen randomly
 *   - with probability 1(-rvar_probability), that variable is the one that
 *     results in minimal increase of the number of unsat clauses.
 * - max_flips = bound on the number of sample_sat steps
 * - max_extra_flips = number of additional (simulated annealing) steps to perform
 *   after a satisfying assignment is found
 *
 * Parameters for mc_sat:
 * - max_samples = number of samples generated
 * - timeout = maximum running time for MCSAT
 * - burn_in_steps = number of burn-in steps
 * - samp_interval = sampling interval
 */
extern void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
	    double sa_temperature, double rvar_probability,
	    uint32_t max_flips, uint32_t max_extra_flips, uint32_t timeout,
	    uint32_t burn_in_steps, uint32_t samp_interval);

/* 
 * Handles the newly activated rule instances. Put them into the corresponding
 * list based on their weights and evalutions.
 */
extern void push_newly_activated_rule_instance(int32_t ridx, samp_table_t *table);

#endif /* __MCMC_H */

