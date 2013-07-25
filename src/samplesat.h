#ifndef __SAMPLESAT_H
#define __SAMPLESAT_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

extern void first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability, uint32_t max_flips);

extern void sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double samp_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips);

/* Put the new clauses into the corresponding list based on their values */
extern void push_newly_activated_clause(int32_t clsidx, samp_table_t *table);

#endif /* __SAMPLESAT_H */

