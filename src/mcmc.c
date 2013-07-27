#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "memalloc.h"
#include "prng.h"
#include "int_array_sort.h"
#include "array_hash_map.h"
#include "gcd.h"
#include "utils.h"
#include "print.h"
#include "input.h"
#include "vectors.h"
#include "buffer.h"

#include "weight_learning.h"
#include "samplesat.h"
#include "mcmc.h"

#ifdef _WIN32
#include <windows.h>
// #include <wincrypt.h>
#endif

/* 
 * The next step is to define the main samplesat procedure.  Here we have placed
 * some clauses among the dead clauses, and are trying to satisfy the live ones.
 * The first step is to perform negative_unit propagation.  Then we pick a
 * random assignment to the remaining variables.

 * Scan the clauses, if there is a fixed-satisfied, it goes in sat_clauses.
 * If there is a fixed-unsat, we have a conflict.
 * If there is a sat clause, we place it in watched list.
 * If the sat clause has a fixed-propagation, then we mark the variable
 * as having a fixed truth value, and move the clause to sat_clauses, while
 * link-propagating this truth value.  When scanning, look for unit propagation in
 * unsat clause list.

 * We then randomly flip a variable, and move all the resulting satisfied clauses
 * to the sat/watched list using the scan_unsat_clauses operation.
 * When this is done, if we have no unsat clauses, we stop.
 * Otherwise, either pick a random unfixed variable and flip it,
 * or pick a random unsat clause and a flip either a random variable or the
 * minimal dcost variable and flip it.  Keep flipping until there are no unsat clauses.

 * First scan the dead clauses to move the satisfied ones to the appropriate
 * lists.  Then move all the unsatisfied clauses to the dead list.
 * Then select-and-move the satisfied clauses to the unsat list.
 * Pick a random truth assignment.  Then repeat the scan-propagate loop.
 * Then selectively, either pick an unfixed variable and flip and scan, or
 * pick an unsat clause and, selectively, a variable and flip and scan.
 */

/*
 * Randomly select live clauses from a list:
 * - link = start of the list and *link_ptr == link
 * - all clauses in the list must be satisfied in the current assignment
 * - a clause of weight W is killed with probability exp(-|W|)
 *   (if W == DBL_MAX then it's a hard clause and it's not killed).
 */
static void kill_clause_list(samp_clause_t **link_ptr, samp_clause_t *link,
		clause_table_t *clause_table) {
	samp_clause_t *next;

	while (link != NULL) {
		if (link->weight == DBL_MAX 
				|| link->weight == DBL_MIN // FIXME this case will not happen
				                           // should be in negative_unit_clauses list
				|| choose() < 1 - exp(-fabs(link->weight))) {
			// keep it
			*link_ptr = link;
			link_ptr = &link->link;
			link = link->link;
		} else {
			// move it to the dead_clauses list
			next = link->link;
			push_clause(link, &clause_table->dead_clauses);
			link = next;
		}
	}
	*link_ptr = NULL;
}

/*
 * In MC-SAT, kills some currently satisfied clauses with
 * the probability equal to 1 - exp(-w).
 *
 * The satisfied clauses are either in sat_clauses or in the
 * watched list of each active literal.
 */
static void kill_clauses(samp_table_t *table) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);
	int32_t i, lit;

	//unsat_clauses is empty; need to only kill satisfied clauses
	//kill sat_clauses
	kill_clause_list(&clause_table->sat_clauses, clause_table->sat_clauses,
			clause_table);

	//kill watched clauses
	for (i = 0; i < atom_table->num_vars; i++) {
		lit = pos_lit(i);
		kill_clause_list(&clause_table->watched[lit],
				clause_table->watched[lit], clause_table);
		lit = neg_lit(i);
		kill_clause_list(&clause_table->watched[lit],
				clause_table->watched[lit], clause_table);
	}

	//TEMPERORY
	//print_live_clauses(table);
}

/*
 * Sets all the clause lists in clause_table to NULL, except for
 * the list of all active clauses.
 */
static void empty_clause_lists(samp_table_t *table) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);

	clause_table->sat_clauses = NULL;
	clause_table->unsat_clauses = NULL;
	clause_table->num_unsat_clauses = 0;
	clause_table->negative_or_unit_clauses = NULL;
	clause_table->dead_negative_or_unit_clauses = NULL;
	clause_table->dead_clauses = NULL;
	uint32_t i;
	int32_t num_unfixed = 0;
	for (i = 0; i < atom_table->num_vars; i++) {
		if (!fixed_tval(atom_table->current_assignment[i])) {
			num_unfixed++;
		}
		atom_table->num_unfixed_vars = num_unfixed;
		clause_table->watched[pos_lit(i)] = NULL;
		clause_table->watched[neg_lit(i)] = NULL;
	}
}

/*
 * Pushes each of the hard clauses into either negative_or_unit_clauses
 * if weight is negative or numlits is 1, or into unsat_clauses if weight
 * is non-negative.  Non-hard clauses go into dead_negative_or_unit_clauses
 * or dead_clauses, respectively.
 */
static void init_clause_lists(clause_table_t *clause_table) {
	uint32_t i;
	samp_clause_t *clause;

	for (i = 0; i < clause_table->num_clauses; i++) {
		clause = clause_table->samp_clauses[i];
		if (clause->weight == DBL_MIN || clause->weight == DBL_MAX) {
			if (clause->weight < 0 || clause->numlits == 1) {
				push_negative_or_unit_clause(clause_table, i);
			}
			else {
				push_unsat_clause(clause_table, i);
			}
		}
		else {
			if (clause->weight < 0 || clause->numlits == 1) {
				push_clause(clause_table->samp_clauses[i],
						&(clause_table->dead_negative_or_unit_clauses));
			}
			else {
				push_clause(clause_table->samp_clauses[i],
						&(clause_table->dead_clauses));
			}
		}
	}
}

/*
 * Scan the list of dead clauses:
 * - move all dead clauses that are satisfied by assignment
 *   into an appropriate watched literal list
 */
static void restore_sat_dead_clauses(clause_table_t *clause_table,
		samp_truth_value_t *assignment) {
	int32_t lit, val;
	samp_clause_t *link, *next;
	samp_clause_t **link_ptr;

	link_ptr = &clause_table->dead_clauses;
	link = *link_ptr;
	while (link != NULL) {
		val = eval_clause(assignment, link);
		if (val != -1) {
			cprintf(2, "---> restoring dead clause %p (val = %"PRId32")\n",
					link, val); //BD
			// swap disjunct[0] and disjunct[val]
			lit = link->disjunct[val];
			//link->disjunct[val] = link->disjunct[0];
			//link->disjunct[0] = lit;
			next = link->link;
			push_clause(link, &clause_table->watched[lit]);
			//link->link = clause_table->watched[lit];
			//clause_table->watched[lit] = link;
			link = next;
			assert(assigned_true_lit(assignment, clause_table->watched[lit]->disjunct[val]));
		} else {
			cprintf(2, "---> dead clause %p stays dead (val = %"PRId32")\n",
					link, val); //BD
			*link_ptr = link;
			link_ptr = &(link->link);
			link = link->link;
		}
	}
	*link_ptr = NULL;
}

/*
 * Scan the list of dead clauses that are unit clauses or have negative weight
 * - any clause satisfied by assignment is moved into the list of (not dead)
 *   unit or negative weight clauses.
 */
static void restore_sat_dead_negative_unit_clauses(clause_table_t *clause_table,
		samp_truth_value_t *assignment) {
	bool restore;
	samp_clause_t *link, *next;
	samp_clause_t **link_ptr;

	link_ptr = &clause_table->dead_negative_or_unit_clauses;
	link = *link_ptr;
	while (link != NULL) {
		if (link->weight < 0) {
			restore = (eval_clause(assignment, link) == -1);
		} else { //unit clause
			restore = (eval_clause(assignment, link) != -1);
		}
		if (restore) {
			// push to negative_or_unit_clauses
			next = link->link;
			push_clause(link, &clause_table->negative_or_unit_clauses);
			link = next;
		} else {
			// remains in the current list, move to next
			*link_ptr = link;
			link_ptr = &(link->link);
			link = link->link;
		}
	}
	*link_ptr = NULL;
}

/*
 * Prepare for a new round in mcsat:
 * - select a new set of live clauses
 * - select a random assignment
 * - return -1 if a conflict is detected by unit propagation (in the fixed clauses)
 * - return 0 otherwise
 */
static int32_t reset_sample_sat(samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;

	cprintf(3, "[reset_sample_sat] started ...\n");

	clear_integer_stack(&(table->fixable_stack)); //clear fixable_stack

	/*
	 * move dead clauses that are satisfied into appropriate lists
	 * so that they can be selected as live clauses in this round.
	 */
	restore_sat_dead_negative_unit_clauses(clause_table, assignment);
	restore_sat_dead_clauses(clause_table, assignment);

	/*
	 * kill selected satisfied clauses: select which clauses will
	 * be live in this round.
	 *
	 * This is partial: live unit or negative weight clauses are
	 * selected within negative_unit_propagate
	 */
	kill_clauses(table);

	if (get_verbosity_level() >= 3) {
		printf("[reset_sample_sat] done. %d unsat_clauses\n",
				table->clause_table.num_unsat_clauses);
		print_assignment(table);
		print_clause_table(table);
	}

	return 0;
}

/*
 * Updates the counts for the query formulas
 */
static void update_query_pmodel(samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	query_instance_table_t *qinst_table = &table->query_instance_table;
	samp_query_instance_t *qinst;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t num_queries, i, j, k;
//	int32_t *apmodel;
//	bool fval;

	num_queries = qinst_table->num_queries;
//	apmodel = table->atom_table.pmodel;

	for (i = 0; i < num_queries; i++) {
		// Each query instance is an array of array of literals
		qinst = qinst_table->query_inst[i];
		// Debugging information
		// First loop through the conjuncts
		// for (j = 0; qinst->lit[j] != NULL; j++) {
		//   // Now the disjuncts
		//   for (k = 0; qinst->lit[j][k] != -1; k++) {
		// 	// Print lit and assignment
		// 	if (is_neg(qinst->lit[j][k])) output("~");
		// 	print_atom(atom_table->atom[var_of(qinst->lit[j][k])], table);
		// 	assigned_true_lit(assignment, qinst->lit[j][k]) ? output(": T ") : output(": F ");
		//   }
		// }
		// printf("\n");
		// fflush(stdout);

//		fval = false;
		// First loop through the conjuncts
		for (j = 0; qinst->lit[j] != NULL; j++) {
			// Now the disjuncts
			for (k = 0; qinst->lit[j][k] != -1; k++) {
				// If any literal is true, skip the rest
				if (assigned_true_lit(assignment, qinst->lit[j][k])) {
					goto cont;
				}
			}
			// None was true, kick out
			goto done;
cont: continue;
		}
		// Each conjunct was true, so the assignment is a model - increase the
		// pmodel counter
		if (qinst->lit[j] == NULL) {
			qinst->pmodel++;
		}
done: continue;
	}
}

/*
 * Updates the counts based on the current sample
 */
static void update_pmodel(samp_table_t *table) {
	atom_table_t *atom_table = &table->atom_table;
	samp_truth_value_t *assignment = atom_table->current_assignment;
	int32_t num_vars = table->atom_table.num_vars;
	int32_t *pmodel = table->atom_table.pmodel;
	int32_t i;

	table->atom_table.num_samples++;
	for (i = 0; i < num_vars; i++) {
		if (assigned_true(assignment[i])) {
			if (get_verbosity_level() >= 3 
					&& i > 0) { // FIXME why i > 0?
				printf("Atom %d was assigned true\n", i);
				fflush(stdout);
			}
			pmodel[i]++;
		}
	}

	update_query_pmodel(table);

	// for the covariance matrix
	update_covariance_matrix_statistics(table);

	if (get_dump_samples_path() != NULL) {
		append_assignment_to_file(get_dump_samples_path(), table);
	}
}

/*
 * Indicates the first sample sat of the mc_sat, in which only the
 * hard clauses are considered.
 */
bool hard_only;

/**
 * Top-level MCSAT call
 *
 * Parameters for sample sat:
 * - sa_probability = probability of a simulated annealing step
 * - samp_temperature = temperature for simulated annealing
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
void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
		double samp_temperature, double rvar_probability, uint32_t max_flips,
		uint32_t max_extra_flips, uint32_t timeout,
		uint32_t burn_in_steps, uint32_t samp_interval) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t conflict;
	uint32_t i;
	time_t fintime = 0;
	bool draw_sample; // whether we use the current round of MCMC as a sample

	cprintf(1, "[mc_sat] MC-SAT started ...\n");

	hard_only = true;

	empty_clause_lists(table);
	init_clause_lists(clause_table);

	conflict = first_sample_sat(table, lazy, sa_probability, samp_temperature,
			rvar_probability, max_flips);

	hard_only = false;

	if (conflict == -1) {
		mcsat_err("Found conflict in initialization.\n");
		return;
	}
	//assert(valid_table(table));

	if (timeout != 0) {
		fintime = time(NULL) + timeout;
	}
	for (i = 0; i < burn_in_steps + max_samples * samp_interval; i++) {
		draw_sample = (i >= burn_in_steps && i % samp_interval == 0);
		if (draw_sample) {
			cprintf(1, "\n---- Sample[%"PRIu32"] ---\n",
					(i - burn_in_steps) / samp_interval);
		}

		//assert(valid_table(table));
		conflict = reset_sample_sat(table);

		sample_sat(table, lazy, sa_probability, samp_temperature,
				rvar_probability, max_flips, max_extra_flips);

		/*
		 * If Sample sat did not find a model (within max_flips)
		 * restore the earlier assignment
		 */
		if (conflict == -1 || clause_table->num_unsat_clauses > 0) {
			if (conflict == -1) {
				cprintf(2, "Hit a conflict.\n");
			} else {
				cprintf(2, "Failed to find a model. \
						Consider increasing max_flips and max_tries - see mcsat help.\n");
			}

			// Flip current_assignment (restore the saved assignment)
			atom_table->current_assignment_index ^= 1;
			atom_table->current_assignment = atom_table->assignment[atom_table->current_assignment_index];

			empty_clause_lists(table);
			init_clause_lists(&table->clause_table);
		}

		if (draw_sample) {
			update_pmodel(table);
		}

		if (get_verbosity_level() >= 2 ||
				(get_verbosity_level() >= 1 && draw_sample)) {
			printf("\n[mc_sat] MC-SAT after round %d:\n", i);
			print_assignment(table);
		}
		//assert(valid_table(table));

		if (timeout != 0 && time(NULL) >= fintime) {
			printf("Timeout after %"PRIu32" samples\n", i);
			break;
		}
	}

	dump_query_instance_table(table);
}

