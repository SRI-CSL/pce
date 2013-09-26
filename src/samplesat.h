#ifndef __SAMPLESAT_H
#define __SAMPLESAT_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

/* 
 * First run of sample sat, in which only hard clauses are considered.
 */
extern int32_t first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability, uint32_t max_flips);

/*
 * Second and following rounds of sample SAT, in which soft rules are also
 * considered.
 */
extern int32_t sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips, bool randomize);

#endif /* __SAMPLESAT_H */

