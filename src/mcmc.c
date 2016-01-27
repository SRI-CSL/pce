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
#include "walksat.h"
#include "mcmc.h"
#include "weight_learning.h"

#ifdef _WIN32
#include <windows.h>
// #include <wincrypt.h>
#endif

/*
 * Defined in tables.h for lack of a better place:
 */
#if USE_PTHREADS
#include <pthread.h>

/* Borrowed from the man page, so let's rethink this: */
struct thread_info {         /* Used as argument to thread_start() */
  pthread_t thread_id;       /* ID returned by pthread_create() */
  int       thread_num;      /* Application-defined thread # */
  samp_table_t *samp_table;  /* Copy of the master samp_table_t object */
};

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
 * In MC-SAT, kills some currently satisfied rule instances with
 * the probability equal to 1 - exp(-w).
 */
static void kill_rule_instances(samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	int32_t i;

	for (i = 0; i < rule_inst_table->num_rule_insts; i++) {
		if (!assigned_true(rule_inst_table->assignment[i]))
			continue;
		rule_inst_t *rinst = rule_inst_table->rule_insts[i];
		if (rinst->weight == DBL_MAX
				|| choose() < 1 - exp(-rinst->weight)) {
		} else {
			rule_inst_table->assignment[i] = v_up_false;
		}
	}
}

/*
 * Set the hard rule instances live and soft rule instances dead.
 */
static void init_rule_instances(rule_inst_table_t *rule_inst_table) {
	uint32_t i;
	for (i = 0; i < rule_inst_table->num_rule_insts; i++) {
		rule_inst_t *rinst = rule_inst_table->rule_insts[i];
		if (rinst->weight == DBL_MAX) {
			rule_inst_table->assignment[i] = v_up_true;
		} else {
			rule_inst_table->assignment[i] = v_up_false;
		}
	}
}

/*
 * Scan the dead rule instances, restore all dead rule instances that are
 * satisfied by assignment.
 */
static void restore_sat_dead_rule_instances(rule_inst_table_t *rule_inst_table,
		samp_truth_value_t *assignment) {
	int32_t i;
	rule_inst_t *rinst;
	for (i = 0; i < rule_inst_table->num_rule_insts; i++) {
		//if (rule_inst_table->live[i]) continue;
		rinst = rule_inst_table->rule_insts[i];
		if (eval_rule_inst(assignment, rinst) == -1) {
			rule_inst_table->assignment[i] = v_up_true;
		} else {
			rule_inst_table->assignment[i] = v_up_false;
		}
	}
}

/*
 * Prepare for a new round in mcsat: select a new set of live rule instances
 */
static int32_t reset_sample_sat(samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	atom_table_t *atom_table = &table->atom_table;

	cprintf(3, "[reset_sample_sat] started ...\n");

	if (get_verbosity_level() >= 4) {
		printf("[reset_sample_sat] before restoring. %d unsat_clauses\n",
				rule_inst_table->unsat_clauses.length);
		print_rule_instances(table);
	}

	restore_sat_dead_rule_instances(rule_inst_table, atom_table->assignment);

	//if (get_verbosity_level() >= 4) {
	//	printf("[reset_sample_sat] restored. %d unsat_clauses\n",
	//			rule_inst_table->unsat_clauses.length);
	//	print_rule_instances(table);
	//}

	kill_rule_instances(table);

	if (get_verbosity_level() >= 2) {
		printf("[reset_sample_sat] killed, done. %d unsat_clauses\n",
				rule_inst_table->unsat_clauses.length);
		print_rule_instances(table);
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

  ///* for the covariance matrix */
  update_covariance_matrix_statistics(table);

  if (get_dump_samples_path() != NULL) {
    append_assignment_to_file(get_dump_samples_path(), table);
  }
}

/*
 * Add a small perburbation to the current assignment by flipping
 * one atom, and then run walksat on only the hard rules to get a
 * consistent model (and close neighboor) of the current model.
 */
static int32_t perturb_assignment(samp_table_t *table, bool lazy, 
		double rvar_probability, uint32_t max_flips, uint32_t max_extra_flips) {

	atom_table_t *atom_table = &table->atom_table;
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;

	int32_t var = choose_unfixed_variable(atom_table);
	atom_table->assignment[var] = negate_tval(atom_table->assignment[var]);

	if (get_verbosity_level() >= 3) {
		cprintf(3, "[perturb_assignment] var = ");
		print_atom_now(atom_table->atom[var], table);
		cprintf(3, "\n");
	}

	init_rule_instances(rule_inst_table);
	copy_assignment_array(atom_table);
	int32_t conflict = sample_sat(table, lazy, 0.0, 0.0, rvar_probability,
			max_flips, max_extra_flips, false);

	/*
	 * If Sample sat did not find a model (within max_flips) restore the
	 * earlier assignment.
	 */
	if (conflict == -1 || rule_inst_table->unsat_clauses.length > 0) {
		if (conflict == -1) {
			cprintf(2, "Hit a conflict.\n");
		} else {
			cprintf(2, "Failed to find a model. Consider increasing"
					"max_flips and max_tries - see mcsat help.\n");
		}

		restore_assignment_array(atom_table);
	}

	cprintf(2, "\n[mc_sat] MC-SAT after perturbation:\n");
	if (get_verbosity_level() >= 2) {
		print_assignment(table);
	}

	return 0;
}

/*
 * Perhaps the easiest path to parallelization lies in multithreading
 * this function by cloning the (master copy of the) table and
 * allowing multiple threads to operate, each on their own copy.  The
 * trick is that we will need to clone the table and its slots,
 * following all of the pointers.  Once that is done, we should be
 * able to start multiple threads that (we hope) won't require
 * mutexes.
 *
 * Then, after joining, we can merge the tables (I make it sound so
 * easy, don't I!) back into the master copy.
 */


void mc_sat_internal(samp_table_t *table_in, bool lazy, uint32_t max_samples, double sa_probability,
                     double sa_temperature, double rvar_probability, uint32_t max_flips,
                     uint32_t max_extra_flips, uint32_t timeout,
                     uint32_t burn_in_steps, uint32_t samp_interval) {

  /* Deep Copy test - first let's just check that the copy functions
   * themselves don't puke: */
  samp_table_t *table = table_in;

  rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
  atom_table_t *atom_table = &table->atom_table;
  int32_t conflict;
  uint32_t i;
  time_t fintime = 0;
  /* whether we use the current round of MCMC as a sample */
  bool draw_sample; 

  if (timeout != 0) {
    fintime = time(NULL) + timeout;
  }

  cprintf(1, "[mc_sat] MC-SAT started ...\n");

  init_rule_instances(rule_inst_table);

  if (get_verbosity_level() >= 2) {
    printf("[mc_sat] Init rule instances done. %"PRIu32" unsat_clauses\n",
	   rule_inst_table->unsat_clauses.length);
    print_rule_instances(table);
  }

  conflict = first_sample_sat(table, lazy, sa_probability, sa_temperature,
			      rvar_probability, max_flips);
  if (conflict == -1) {
    mcsat_err("Found conflict in initialization.\n");
    return;
  }
  //assert(valid_table(table));

  for (i = 0; i < burn_in_steps + max_samples * samp_interval; i++) {

    if (timeout != 0 && time(NULL) >= fintime) {
      printf("Timeout after %"PRIu32" samples\n", i);
      break;
    }

    rule_inst_table->soft_rules_included = true;
    draw_sample = (i >= burn_in_steps && i % samp_interval == 0);
    cprintf(2, "\n[mc_sat] MC-SAT round %"PRIu32" started:\n", i);

    //assert(valid_table(table));
    conflict = reset_sample_sat(table);

    copy_assignment_array(atom_table);
    conflict = sample_sat(table, lazy, sa_probability, sa_temperature,
			  rvar_probability, max_flips, max_extra_flips, true);

    /*
     * If Sample sat did not find a model (within max_flips)
     * restore the earlier assignment
     */
    if (conflict == -1 || rule_inst_table->unsat_clauses.length > 0) {
      if (conflict == -1) {
	cprintf(2, "Hit a conflict.\n");
      } else {
	cprintf(2, "Failed to find a model. Consider increasing"
		"max_flips and max_tries - see mcsat help.\n");
      }

      // Flip current_assignment (restore the saved assignment)
      restore_assignment_array(atom_table);
      i--;
      continue;
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

    /* add a perturbation on the current model, resulting a new model with
     * approximately similar probability, to jump out of the an isolated
     * region of feasible models */
    rule_inst_table->soft_rules_included = false;

    conflict = perturb_assignment(table, lazy, rvar_probability, max_flips,
				  max_extra_flips);
  }
}

static void *mc_sat_thread(void *arg) {
  struct thread_info *tinfo = (struct thread_info *) arg;
  mc_sat_internal(tinfo->samp_table, lazy_mcsat(), get_max_samples(),
                  get_sa_probability(), get_sa_temperature(),
                  get_rvar_probability(), get_max_flips(),
                  get_max_extra_flips(), get_mcsat_timeout(),
                  get_burn_in_steps(), get_samp_interval());
  return NULL;
}


void mc_sat(samp_table_t *table, bool lazy, uint32_t max_samples, double sa_probability,
            double sa_temperature, double rvar_probability, uint32_t max_flips,
            uint32_t max_extra_flips, uint32_t timeout,
            uint32_t burn_in_steps, uint32_t samp_interval) {

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* Start threading - ultimately, we want to migrate
   * this into mcmc.c to create a threaded version of
   * the mc_sat function, which would be accessible to
   * any command that explicitly or implicitly performs
   * mcsat. */

  int nthreads = get_mcsat_thread_count();
  if (nthreads == 0 || !USE_PTHREADS) {
    mc_sat_internal(table, lazy_mcsat(), get_max_samples(),
                    get_sa_probability(), get_sa_temperature(),
                    get_rvar_probability(), get_max_flips(),
                    get_max_extra_flips(), get_mcsat_timeout(),
                    get_burn_in_steps(), get_samp_interval());
  } else {
    /* Maybe push this to another static function: */
#if USE_PTHREADS                  
    int i, s;
    void *res;
    pthread_attr_t attr;
    struct thread_info *tinfo;
    samp_table_t **copy;

    /* These should be thrown into a pool to be re-used! */
    tinfo = (struct thread_info*) safe_malloc(nthreads * sizeof(struct thread_info));
    copy = (samp_table_t **) safe_malloc(nthreads * sizeof(samp_table_t*));

    s = pthread_attr_init(&attr);
    if (s != 0) perror("pthread_attr_init");

    for (i = 0; i < nthreads; i++) {
      copy[i] = clone_samp_table(table);
      tinfo[i].thread_num = i+1;
      tinfo[i].samp_table = copy[i];
#if 0
      printf("mcsat thread %d: %llx\n",
             tinfo[i].thread_num, (long long unsigned int) tinfo[i].samp_table);
      fflush(stdout);
#endif
      s = pthread_create(&tinfo[i].thread_id, &attr, &mc_sat_thread, &(tinfo[i]));
      if (s != 0) perror("pthread_create");
    }

    printf("Created %d mcmc threads\n", nthreads);
    fflush(stdout);

    for (i = 0; i < nthreads; i++) {
      s = pthread_join(tinfo[i].thread_id, &res);
      if (s != 0) perror("pthread_join");
      merge_atom_tables( &(table->atom_table), &(copy[i]->atom_table) );
      merge_query_instance_tables( &(table->query_instance_table), &(copy[i]->query_instance_table) );
    }

    s = pthread_attr_destroy(&attr);
    free(tinfo);
    /* DANGER - THIS IS A MEMORY LEAK!  Until "deep
       freedom" can be implemented, this code will leak
       memory like a chump. */

    /* free(copy) */
#endif
  }

}




/*
 * Pushes a newly created clause to the corresponding list. We first decide
 * whether the clause is alive or dead, by running a test based on its weight
 * (choose() < 1 - exp(-w)). If it is alive, we then call push_alive_clause to
 * put it into sat, watched, or unsat list based on the evaluation.
 * 
 * In the first sample SAT, we only need to find a model for all the HARD
 * clauses.  Therefore, when a new clause is activated, if it is a hard clause,
 * we put it into the corresoponding live list, otherwise we put it into the
 * dead lists. In the following rounds of sample SAT, we determine whether to
 * keep the new clause alive by its weight, more specifically, with probability
 * equal to 1 - exp(-w).
 *
 * We use rule_inst_table->soft_rules_included to determine whether the soft
 * rules are being considered (true for the sample SAT phase of the second and
 * following rounds, but false for the first round and the perturb + WalkSAT
 * phase of the following rounds.
 */
void push_newly_activated_rule_instance(int32_t ridx, samp_table_t *table) {
	rule_inst_table_t *rule_inst_table = &table->rule_inst_table;
	rule_inst_t *rinst = rule_inst_table->rule_insts[ridx];

	int32_t i;
	if (rinst->weight == DBL_MAX
			|| (rule_inst_table->soft_rules_included 
				&& choose() < 1 - exp(-rinst->weight))) {
		rule_inst_table->assignment[ridx] = v_up_true;
		for (i = 0; i < rinst->num_clauses; i++) {
			clause_list_insert_tail(rinst->conjunct[i], &rule_inst_table->live_clauses);
		}
	}
	else {
		rule_inst_table->assignment[ridx] = v_up_false;
	}
}

