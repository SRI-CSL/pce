#ifndef __MCMC_H
#define __MCMC_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

#define MCSAT_CONFLICT 21

extern void update_pmodel(samp_table_t *table);

extern void empty_clause_lists(samp_table_t *table);

extern void init_clause_lists(clause_table_t *clause_table);

int32_t reset_sample_sat(samp_table_t *table);

void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
	    double samp_temperature, double rvar_probability,
	    uint32_t max_flips, uint32_t max_extra_flips, uint32_t timeout,
	    uint32_t burn_in_steps, uint32_t samp_interval);

#endif /* __MCMC_H */

