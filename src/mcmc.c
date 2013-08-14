#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "memalloc.h"
#include "utils.h"
#include "print.h"
#include "input.h"
#include "vectors.h"
#include "buffer.h"
#include "clause_list.h"
#include "samplesat.h"
#include "mcmc.h"
#include "weight_learning.h"
#include "training_data.h"

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
 * - link = start of the list and *ptr == link
 * - all clauses in the list must be satisfied in the current assignment
 * - a clause of weight W is killed with probability exp(-|W|)
 *   (if W == DBL_MAX then it's a hard clause and it's not killed).
 *
 * @list_src: the list from which clauses are killed
 * @list_dst: the list to which the killed clauses are moved
 */
static void kill_clause_list(
		samp_clause_list_t *list_src,
		samp_clause_list_t *list_dst) {
	samp_clause_t *ptr;
	samp_clause_t *clause;

	for (ptr = list_src->head; ptr != list_src->tail;) {
		clause = ptr->link;
		if (clause->weight == DBL_MAX || clause->weight == DBL_MIN
				|| choose() < 1 - exp(-fabs(clause->weight))) {
			/* keep it */
			ptr = next_clause(ptr);
		} else {
			/* move it to the dead_clauses list */
			clause_list_pop(list_src, ptr);
			clause_list_insert_head(clause, list_dst);
		}
	}
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
	kill_clause_list(&clause_table->sat_clauses, &clause_table->dead_clauses);

	//kill watched clauses
	for (i = 0; i < atom_table->num_vars; i++) {
		lit = pos_lit(i);
		kill_clause_list(&clause_table->watched[lit], &clause_table->dead_clauses);
		lit = neg_lit(i);
		kill_clause_list(&clause_table->watched[lit], &clause_table->dead_clauses);
	}

	// all the unsat clauses are already dead
	assert(is_empty_clause_list(&clause_table->unsat_clauses));
}

/*
 * Kill unit and negative clauses
 */
static void kill_negative_unit_clauses(clause_table_t *clause_table) {
	kill_clause_list(&clause_table->negative_or_unit_clauses,
			&clause_table->dead_negative_or_unit_clauses);
}

/*
 * Sets all the clause lists in clause_table to NULL, except for
 * the list of all active clauses.
 */
static void empty_clause_lists(samp_table_t *table) {
	clause_table_t *clause_table = &(table->clause_table);
	atom_table_t *atom_table = &(table->atom_table);

	uint32_t i;
	int32_t num_unfixed = 0;
	for (i = 0; i < atom_table->num_vars; i++) {
		if (unfixed_tval(atom_table->assignment[i])) {
			num_unfixed++;
		}
		atom_table->num_unfixed_vars = num_unfixed;
		init_clause_list(&clause_table->watched[pos_lit(i)]);
		init_clause_list(&clause_table->watched[neg_lit(i)]);
	}

	init_clause_list(&clause_table->sat_clauses);
	init_clause_list(&clause_table->unsat_clauses);
	init_clause_list(&clause_table->live_clauses);
	init_clause_list(&clause_table->negative_or_unit_clauses);
	init_clause_list(&clause_table->dead_clauses);
	init_clause_list(&clause_table->dead_negative_or_unit_clauses);
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
				clause_list_insert_head(clause, &clause_table->negative_or_unit_clauses);
			}
			else {
				/* Temporarily put into live_clause before evaluating them */
				clause_list_insert_head(clause, &clause_table->live_clauses);
			}
		}
		else {
			if (clause->weight < 0 || clause->numlits == 1) {
				clause_list_insert_head(clause, &clause_table->dead_negative_or_unit_clauses);
			}
			else {
				clause_list_insert_head(clause, &clause_table->dead_clauses);
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
	samp_clause_t *ptr;
	samp_clause_t *cls;

	for (ptr = clause_table->dead_clauses.head;
			ptr != clause_table->dead_clauses.tail;) {
		cls = ptr->link;
		val = eval_clause(assignment, cls);
		if (val != -1) {
			cprintf(2, "---> restoring dead clause %p (val = %"PRId32")\n",
					cls, val); //BD
			cls = clause_list_pop(&clause_table->dead_clauses, ptr);
			lit = cls->disjunct[val];
			clause_list_insert_head(cls, &clause_table->watched[lit]);
			assert(assigned_true_lit(assignment, 
						clause_table->watched[lit].head->link->disjunct[val]));
		} else {
			cprintf(2, "---> dead clause %p stays dead (val = %"PRId32")\n",
					cls, val); //BD
			ptr = next_clause(ptr);
		}
	}
}

/*
 * Scan the list of dead clauses that are unit clauses or have negative weight
 * - any clause satisfied by assignment is moved into the list of (not dead)
 *   unit or negative weight clauses.
 */
static void restore_sat_dead_negative_unit_clauses(clause_table_t *clause_table,
		samp_truth_value_t *assignment) {
	bool restore;
	samp_clause_t *ptr;
	samp_clause_t *cls;
	for (ptr = clause_table->dead_negative_or_unit_clauses.head;
			ptr != clause_table->dead_negative_or_unit_clauses.tail;) {
		cls = ptr->link;
		if (cls->weight < 0) { /* negative weight clause */
			restore = (eval_clause(assignment, cls) == -1);
		} else { /* unit clause */
			restore = (eval_clause(assignment, cls) != -1);
		}
		if (restore) {
			/* push to negative_or_unit_clauses */
			cls = clause_list_pop(&clause_table->dead_negative_or_unit_clauses, ptr);
			clause_list_insert_head(cls, &clause_table->negative_or_unit_clauses);
		} else {
			/* remains in the current list, move to next */
			ptr = next_clause(ptr);
		}
	}
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

	cprintf(3, "[reset_sample_sat] started ...\n");

	clear_integer_stack(&(table->fixable_stack)); //clear fixable_stack

	/*
	 * move dead clauses that are satisfied into appropriate lists
	 * so that they can be selected as live clauses in this round.
	 */
	restore_sat_dead_negative_unit_clauses(clause_table, atom_table->assignment);
	restore_sat_dead_clauses(clause_table, atom_table->assignment);

	/*
	 * kill selected satisfied clauses: select which clauses will
	 * be live in this round.
	 */
	kill_negative_unit_clauses(clause_table);
	kill_clauses(table);

	if (get_verbosity_level() >= 2) {
		printf("[reset_sample_sat] done. %d unsat_clauses\n",
				clause_table->unsat_clauses.length);
		print_live_clauses(table);
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
				if (assigned_true_lit(atom_table->assignment, qinst->lit[j][k])) {
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
	int32_t num_vars = table->atom_table.num_vars;
	int32_t *pmodel = table->atom_table.pmodel;
	int32_t i;

	table->atom_table.num_samples++;
	for (i = 0; i < num_vars; i++) {
		if (assigned_true(atom_table->assignment[i])) {
			if (get_verbosity_level() >= 3 && i > 0) { // FIXME why i > 0?
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
 * False before the first sample sat of the mc_sat finishes, in which only the
 * hard clauses are considered. Afterwards, the soft clauses are also
 * considered.
 */
bool soft_clauses_included = false;

/**
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
void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
		double sa_temperature, double rvar_probability, uint32_t max_flips,
		uint32_t max_extra_flips, uint32_t timeout,
		uint32_t burn_in_steps, uint32_t samp_interval) {
	clause_table_t *clause_table = &table->clause_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t conflict;
	uint32_t i;
	time_t fintime = 0;
	bool draw_sample; /* whether we use the current round of MCMC as a sample */

	cprintf(1, "[mc_sat] MC-SAT started ...\n");

	empty_clause_lists(table);
	init_clause_lists(clause_table);

	if (get_verbosity_level() >= 2) {
		printf("[mc_sat] Init clause list done. %"PRIu32" unsat_clauses\n",
				clause_table->unsat_clauses.length);
		//print_live_clauses(table);
		print_clause_table(table);
	}

	/*
	 * FIXME for eager inference, the clauses are instantiated before running
	 * mcsat, but they should be put into the lists following the same criteria
	 * as lazy mcsat.
	 */
	conflict = first_sample_sat(table, lazy, sa_probability, sa_temperature,
			rvar_probability, max_flips);

	soft_clauses_included = true;

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

		cprintf(2, "\n[mc_sat] MC-SAT round %"PRIu32" started:\n", i);

		//assert(valid_table(table));
		conflict = reset_sample_sat(table);

		sample_sat(table, lazy, sa_probability, sa_temperature,
				rvar_probability, max_flips, max_extra_flips);

		/*
		 * If Sample sat did not find a model (within max_flips)
		 * restore the earlier assignment
		 */
		if (conflict == -1 || clause_table->unsat_clauses.length > 0) {
			if (conflict == -1) {
				cprintf(2, "Hit a conflict.\n");
			} else {
				cprintf(2, "Failed to find a model. \
						Consider increasing max_flips and max_tries - see mcsat help.\n");
			}

			// Flip current_assignment (restore the saved assignment)
			switch_assignment_array(atom_table);

			empty_clause_lists(table);
			init_clause_lists(clause_table);
		}

		if (draw_sample) {
			update_pmodel(table);
		}

		cprintf(2, "\n[mc_sat] MC-SAT after round %"PRIu32":\n", i);
		if (get_verbosity_level() >= 2 ||
				(get_verbosity_level() >= 1 && draw_sample)) {
			if (draw_sample) {
				cprintf(1, "\n---- Sample[%"PRIu32"] ---\n",
						(i - burn_in_steps) / samp_interval);
			}
			print_assignment(table);
		}
		//assert(valid_table(table));

		if (timeout != 0 && time(NULL) >= fintime) {
			printf("Timeout after %"PRIu32" samples\n", i);
			break;
		}
	}
}

/*
 * Pushes a newly created clause to the corresponding list. We first decide
 * whether the clause is alive or dead, by running a test based on its weight
 * (choose() < 1 - exp(-w)). If it is alive, we then call push_alive_clause
 * to put it into sat, watched, or unsat list.
 * 
 * In the first round, we only need to find a model for all the HARD clauses.
 * Therefore, when a new clause is activated, if it is a hard clause, we
 * put it into the corresoponding live list, otherwise we put it into the
 * dead lists. In the following rounds, we determine whether to keep the new
 * clause alive by its weight, more specifically, with probability equal to
 * 1 - exp(-w).
 */
void push_newly_activated_clause(int32_t clsidx, samp_table_t *table) {
	clause_table_t *clause_table = &table->clause_table;
	samp_clause_t *clause = clause_table->samp_clauses[clsidx];

	double abs_weight = fabs(clause->weight);
	if (clause->weight == DBL_MAX
			|| (soft_clauses_included && choose() < 1 - exp(-abs_weight))) {
		if (clause->numlits == 1 || clause->weight < 0) {
			insert_negative_unit_clause(clause, table);
			//clause_list_insert_head(clause, &clause_table->negative_or_unit_clauses);
		}
		else {
			insert_live_clause(clause, table);
		}
	}
	else {
		if (clause->numlits == 1 || clause->weight < 0) {
			clause_list_insert_head(clause, &clause_table->dead_negative_or_unit_clauses);
		}
		else {
			clause_list_insert_head(clause, &clause_table->dead_clauses);
		}
	}
}

