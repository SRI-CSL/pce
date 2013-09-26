#ifndef __WALKSAT_H
#define __WALKSAT_H

#include "tables.h"

/*
 * Assigns random truth values to unfixed vars and returns number of unfixed
 * vars (num_unfixed_vars).
 */
extern void init_random_assignment(samp_table_t *table);

/* 
 * Put a live clause into sat, unsat, or watched list depending on its
 * evaluation.
 */
extern void insert_live_clause(samp_clause_t *clause, samp_table_t *table);

/*
 * Decompose the live rule instances into clauses and put them
 * into the clause list.
 */
extern void init_live_clauses(samp_table_t *table);

/*
 * Scans the unsat clauses to find those that are sat or to propagate fixed
 * truth values. The propagating clauses are placed on the sat_clauses list,
 * and the propagated literals are placed in fixable_stack so that they will
 * be dealt with by process_fixable_stack later.
 */
extern void scan_live_clauses(samp_table_t *table);

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
 * Computes the cost of flipping an unfixed variable without the actual flip.
 */
extern void cost_flip_unfixed_variable(samp_table_t *table, int32_t var, int32_t *dcost);

/*
 * Chooses a literal from an unsat clause as a candidate to flip next;
 * makes a random choice with rvar_probability, and otherwise a greedy
 * strategy is used (i.e., choose the literal that minimizes the increase
 * of cost).
 *
 * Returns the index of the variable in the clause.
 */
extern int32_t choose_clause_var(samp_table_t *table, samp_clause_t *clause,
		double rvar_probability);

/*
 * Executes a variable flip by first scanning all the previously sat clauses
 * in the watched list, and then moving any previously unsat clauses to the
 * sat/watched list.
 */
int32_t flip_unfixed_variable(samp_table_t *table, int32_t var);

extern void mw_sat(samp_table_t *table, int32_t num_trials, 
		double rvar_probability, int32_t max_flips, uint32_t timeout);

#endif /* __WALKSAT_H */
