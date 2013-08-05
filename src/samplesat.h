#ifndef __SAMPLESAT_H
#define __SAMPLESAT_H 1

#include <stdint.h>
#include <stdbool.h>

#include "tables.h"

/* First run of sample sat, where only hard clauses are considered */
extern int32_t first_sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability, uint32_t max_flips);

/* Later run of sample sat, where soft clauses are also considered */
extern void sample_sat(samp_table_t *table, bool lazy, double sa_probability,
		double sa_temperature, double rvar_probability,
		uint32_t max_flips, uint32_t max_extra_flips);

/* Put a live clause into sat, unsat, or watched list */
extern void insert_live_clause(samp_clause_t *clause, samp_table_t *table);

/* Process a newly created live negative_or_unit_clause */
extern void insert_negative_unit_clause(samp_clause_t *clause, samp_table_t *table);

#endif /* __SAMPLESAT_H */

