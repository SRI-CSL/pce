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
/*
 * [eager only] Chooses a random unfixed atom in a simmulated annealing
 * step in sample SAT.
 */
extern int32_t choose_unfixed_variable(atom_table_t *atom_table);

/* 
 * [lazy only] Lazy version of choose_unfixed_variable. In lazy inference, not
 * all the atoms are in memory, so we need to randomly pick from both active
 * and inactive atoms.
 */
extern int32_t choose_random_atom(samp_table_t *table);

/* 
 * Put a live clause into sat, unsat, or watched list depending on its
 * evaluation.
 */
extern void insert_live_clause(samp_clause_t *clause, samp_table_t *table);

#endif /* __SAMPLESAT_H */

