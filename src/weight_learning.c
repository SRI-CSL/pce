/*
 * weight_learning.c
 *
 *  Created on: Jul 29, 2011
 *      Author: papai
 */
#include <string.h>
#include <math.h>
#include "input.h"
#include "print.h"
#include "weight_learning.h"
#include "memalloc.h"
#include "tables.h"
#include "lbfgs.h"
#include "cnf.h"
#include "prng.h"
#include "mcmc.h"

static weighted_formula_t* last_weighted_formula = NULL;
static weighted_formula_t* first_weighted_formula = NULL;
static covariance_matrix_t* covariance_matrix = NULL;
static pseudo_log_likelihood_stats_t pll_stats;
static training_data_t *training_data;

static int32_t num_weighted_formulas;

// true if we use training data
static bool training_data_available;

// true if at least for one formula a subjective probability is provided
static bool subjective_probabilities_available;

// Computes the probability of a FOL based on the probabilities of the ground query instances
// belonging to it.
extern double get_probability_of_quantified_formula(pvector_t ask_buffer,
		input_formula_t *fmla, samp_table_t *table) {
	//samp_query_instance_t *qinst;
	int32_t i;

	double probability = 0.0;

	double num_instances = (double) ask_buffer.size;
	for (i = 0; i < ask_buffer.size; i++) {
	//	qinst = (samp_query_instance_t *) ask_buffer.data[i];
		probability += query_probability(
				(samp_query_instance_t *) ask_buffer.data[i], table);
	}
	probability = probability / num_instances;
	return probability;
}

// Called in each iteration of the weight learning algorithm.
// Computes the probability of every formula the weight of which is
// about to be learnt and stores the computed values in the
// weighted formula structure.
extern void set_empirical_expectation_of_weighted_formulae(
		weighted_formula_t* first_weighted_formula, samp_table_t *table) {
	ground_query_t *ground_query;
	weighted_formula_t* weighted_formula = first_weighted_formula;
	//	dump_atom_table(table);
	while (weighted_formula != NULL) {
		weighted_formula->sampled_expected_value = 0.0;
		ground_query = weighted_formula->qinst_list;
		while (ground_query != NULL) {
			double prob = query_probability(ground_query->qinst, table);
			weighted_formula->sampled_expected_value += prob;
			ground_query = ground_query->next;
			//			cprintf(0, "prob: %f\n", prob);
		}
		weighted_formula->sampled_expected_value
				/= ((double) weighted_formula->num_groundings);
		//		print_weighted_formula(weighted_formula, table);
		//		cprintf(0, "empirical EF: %f\tfrom %f groundings\n", weighted_formula->sampled_expected_value, (double)weighted_formula->n_groundings);
		weighted_formula = weighted_formula->next;
	}

}

// Associates a ground clause with the weighted formula.
extern void add_ground_clause_to_weighted_formula(
		weighted_formula_t* weighted_formula, samp_clause_t* clause) {
	if (weighted_formula->ground_clause_list == NULL) {
		weighted_formula->ground_clause_list = (ground_clause_t *) safe_malloc(
				sizeof(ground_clause_t));
		weighted_formula->ground_clause_list->clause = clause;
		weighted_formula->num_groundings++;
		weighted_formula->ground_clause_list->next = NULL;
		return;
	}

	ground_clause_t* ground_clause = weighted_formula->ground_clause_list;

	while (ground_clause->next != NULL) {
		ground_clause = ground_clause->next;
	}
	ground_clause->next = (ground_clause_t *) safe_malloc(
			sizeof(ground_clause_t));
	ground_clause->next->clause = clause;
	weighted_formula->num_groundings++;
	ground_clause->next->next = NULL;
}

// Associates a ground query clause with the weighted formula.
extern void add_query_instance_to_weighted_formula(
		weighted_formula_t* weighted_formula, samp_query_instance_t* qinst) {
	if (weighted_formula->qinst_list == NULL) {
		weighted_formula->qinst_list = (ground_query_t *) safe_malloc(
				sizeof(ground_query_t));
		weighted_formula->qinst_list->qinst = qinst;
		weighted_formula->qinst_list->next = NULL;
		return;
	}

	ground_query_t* ground_query = weighted_formula->qinst_list;

	while (ground_query->next != NULL) {
		ground_query = ground_query->next;
	}
	ground_query->next = (ground_query_t *) safe_malloc(sizeof(ground_query_t));
	ground_query->next->qinst = qinst;
	ground_query->next->next = NULL;

}

extern void print_weighted_formula(weighted_formula_t *weighted_formula,
		samp_table_t* table) {
	output("W:%f\tPr:%f\tSPr:%f\tDPr:%f\t", weighted_formula->weight,
			weighted_formula->sampled_expected_value,
			weighted_formula->subjective_probability,
			weighted_formula->data_expected_value);
	if (weighted_formula->is_ground) {
		print_clause(weighted_formula->clausified_formula.clause, table);
	} else {
		rule_literal_t *lit;
		int32_t j;
		samp_rule_t* rule = weighted_formula->clausified_formula.rule;

		if (rule->num_vars > 0) {
			for (j = 0; j < rule->num_vars; j++) {
				j == 0 ? output(" (") : output(", ");
				output("%s", rule->vars[j]->name);
			}
			output(")");
		}
		for (j = 0; j < rule->num_lits; j++) {
			j == 0 ? output("   ") : output(" | ");
			lit = rule->literals[j];
			if (lit->neg)
				output("~");
			print_rule_atom(lit->atom, lit->neg, rule->vars, table, 0);
		}
	}
	output("\n");
}

// Clears the query instances list, resets the empirical expected values.
// Called in each iteration of the weight learning algorithm.
extern void reset_weighted_formula(weighted_formula_t *weighted_formula) {
	ground_query_t* ground_query = weighted_formula->qinst_list;
	ground_query_t* next_ground_query;
	while (ground_query != NULL) {
		next_ground_query = ground_query->next;
		safe_free(ground_query);
		ground_query = next_ground_query;
	}
	weighted_formula->sampled_expected_value = -1.0;
	weighted_formula->qinst_list = NULL;
}

extern void free_weighted_formula(weighted_formula_t *weighted_formula) {
	int i;
	int vlen;
	ground_clause_t* next_ground_clause;

	free_formula(weighted_formula->formula);

	if (weighted_formula->frozen != NULL) {
		for (vlen = 0; weighted_formula->frozen[vlen] != NULL; vlen++) {
			safe_free(weighted_formula->frozen[vlen]);
		}
	}
	safe_free(weighted_formula->frozen);
	safe_free(weighted_formula->source);

	ground_clause_t* ground_clause = weighted_formula->ground_clause_list;
	while (ground_clause != NULL) {
		next_ground_clause = ground_clause->next;
		safe_free(ground_clause);
		ground_clause = next_ground_clause;
	}
	ground_query_t* ground_query = weighted_formula->qinst_list;
	ground_query_t* next_ground_query;
	while (ground_query != NULL) {
		next_ground_query = ground_query->next;
		safe_free(ground_query);
		ground_query = next_ground_query;
	}
	for (i = 0; i < weighted_formula->num_training_sets; ++i) {
		safe_free(weighted_formula->feature_counts[i].count_var_zero);
		safe_free(weighted_formula->feature_counts[i].count_var_one);
	}
	safe_free(weighted_formula->feature_counts);
	//safe_free(weighted_formula->clausified_formula);
	safe_free(weighted_formula);
}

// From the input formula creates a weighted formula.
extern void add_weighted_formula(samp_table_t* table, input_add_fdecl_t* decl) {
	input_formula_t* formula = deep_copy_formula(decl->formula);
	weighted_formula_t* weighted_formula = (weighted_formula_t*) safe_malloc(
			sizeof(weighted_formula_t));
	weighted_formula->formula = formula;
	weighted_formula->ground_clause_list = NULL;
	weighted_formula->weight = decl->weight;
	//	weighted_formula->weight = 1.0;
	if (decl->frozen == NULL) {
		weighted_formula->frozen = NULL;
	} else {
		int vlen;
		for (vlen = 0; decl->frozen[vlen] != NULL; vlen++) {
		}
		weighted_formula->frozen = (char **) safe_malloc(
				(vlen + 1) * sizeof(char *));
		int i;
		for (i = 0; i < vlen; i++) {
			int len = strlen(decl->frozen[i]);
			weighted_formula->frozen[i] = (char*) safe_malloc(
					(len + 1) * sizeof(char));
			strcpy(weighted_formula->frozen[i], decl->frozen[i]);
		}
		weighted_formula->frozen[vlen] = NULL;
	}
	if (decl->source == NULL) {
		weighted_formula->source = NULL;
	} else {
		int len = strlen(decl->source);
		weighted_formula->source
				= (char*) safe_malloc((len + 1) * sizeof(char));
		strcpy(weighted_formula->source, decl->source);
	}

	//TODO: do parsing in a different way
	weighted_formula->subjective_probability = decl->weight;
	weighted_formula->next = NULL;

	if ((first_weighted_formula == NULL) && (last_weighted_formula == NULL)) {
		first_weighted_formula = last_weighted_formula = weighted_formula;
	} else {
		last_weighted_formula->next = weighted_formula;
		last_weighted_formula = weighted_formula;
	}

	int32_t rule_cnt = table->rule_table.num_rules;
	int32_t clause_cnt = table->clause_table.num_clauses;

	// To monitor which clauses were added
	// TODO: implement this in a decent way
	double clause_weights[clause_cnt];
	int32_t i;
	for (i = 0; i < clause_cnt; i++) {
		clause_weights[i] = table->clause_table.samp_clauses[i]->weight;
	}
	cprintf(2, "Clausifying and adding formula for LEARN\n");
	add_cnf(decl->frozen, decl->formula, decl->weight, decl->source, true);

	if (rule_cnt != table->rule_table.num_rules) {
		// rule was not ground
		weighted_formula->is_ground = false;
		weighted_formula->clausified_formula.rule
				= table->rule_table.samp_rules[table->rule_table.num_rules - 1];
	} else {
		weighted_formula->is_ground = true;
		weighted_formula->clausified_formula.clause
				= table->clause_table.samp_clauses[table->clause_table.num_clauses
						- 1];
	}

	for (i = 0; i < table->clause_table.num_clauses; i++) {
		samp_clause_t* clause = table->clause_table.samp_clauses[i];
		if (i >= clause_cnt) { // clause has just been added  to the clause table, it must belong to the formula to be trained
			add_ground_clause_to_weighted_formula(weighted_formula, clause);
		} else {
			if (clause->weight != clause_weights[i]) {
				// the clause's weight has changed, it must belong to the formula to be trained
				add_ground_clause_to_weighted_formula(weighted_formula, clause);
			}
		}

	}

}

// Create queries based on the weighted formulas we want to learn
// associate the created ground queries with the weighted formulas
extern void initialize_weighted_formulas(samp_table_t* table) {
	int i, j;
	weighted_formula_t* weighted_formula = first_weighted_formula;
	query_table_t* query_table = &table->query_table;
	query_instance_table_t* query_instance_table = &table->query_instance_table;
	while (weighted_formula != NULL) {
		reset_weighted_formula(weighted_formula);
		//print_weighted_formula(weighted_formula, table);
		add_cnf_query(weighted_formula->formula);
		weighted_formula->num_groundings = 0;
		// query instances belonging to the query
		for (i = 0; i < query_instance_table->num_queries; ++i) {
			samp_query_instance_t* qinst = query_instance_table->query_inst[i];
			for (j = 0; j < qinst->query_indices.size; ++j) {
				if (qinst->query_indices.data[j] == query_table->num_queries
						- 1) {
					add_query_instance_to_weighted_formula(weighted_formula,
							qinst);
					weighted_formula->num_groundings++;
				}
			}
		}
		weighted_formula->sampled_expected_value = -1.0;
		weighted_formula = weighted_formula->next;
	}
}


// The gradient ascent algorithm.
extern void gradient_ascent(training_data_t *data, samp_table_t* table) {
	double error = 0.0;
	int it = 0;
	int i, j;
	query_table_t* query_table = &table->query_table;
	weighted_formula_t* weighted_formula;
	training_data = data;

	training_data_available = (training_data != NULL);

	set_subjective_probabilities_available();
	set_num_weighted_formulas();

	if (training_data_available) {
		add_training_data(training_data);
	}
	if (subjective_probabilities_available) {
		init_covariance_matrix();
	}
	if (training_data_available  && USE_PLL) {
		initialize_pll_stats(training_data, &pll_stats, &table->clause_table);
	}

	double gradient[num_weighted_formulas];
	int K = 50;
	double last_K_expected_values[num_weighted_formulas][K];

	// used for the stopping criteria
	double avg_last_K_expected_values[num_weighted_formulas];

	for (i = 0; i < K; ++i) {
		for (j = 0; j < num_weighted_formulas; ++j) {
			last_K_expected_values[j][i] = 0.0;
		}
		avg_last_K_expected_values[i] = 0.0;
	}

	do {
		error = 0.0;
		initialize_weighted_formulas(table);
		if (subjective_probabilities_available) {
			reset_covariance_matrix();
		}
		//					printf("number of queries: %d\n", query_table->num_queries);
		//					printf("number of query instances: %d\n",
		//							query_instance_table->num_queries);

		mc_sat(table, false, get_max_samples(), get_sa_probability(),
				get_sa_temperature(), get_rvar_probability(),
				get_max_flips(), get_max_extra_flips(), get_mcsat_timeout(),
				get_burn_in_steps(), get_samp_interval());
		set_empirical_expectation_of_weighted_formulae(first_weighted_formula,
				table);

		if (subjective_probabilities_available) {
			compute_covariance_matrix();
		}
		if (training_data_available && USE_PLL) {
			compute_pseudo_log_likelihood_statistics(training_data, &pll_stats);
		}
		compute_gradient(gradient);

		// normalize gradient if its length is greater than 1.0
		double g_length = 0.0;
		int k;
		for (weighted_formula = first_weighted_formula, k=0; weighted_formula
				!= NULL; weighted_formula = weighted_formula->next,k++) {
			g_length = gradient[k] * gradient[k];
		}
		g_length = sqrt(g_length);
	    if (g_length > 1.0)
	    {
			for (weighted_formula = first_weighted_formula, k=0; weighted_formula
					!= NULL; weighted_formula = weighted_formula->next,k++) {
				gradient[k] /= g_length;
			}
	    }

		if (it % 100 == 0) {
			output("it=%d\n", it);
		}

		for (i = 0, weighted_formula = first_weighted_formula; weighted_formula
				!= NULL; ++i, weighted_formula = weighted_formula->next) {
			// updated the average of the last expected values in the circular buffer
			last_K_expected_values[i][it % K]
					= weighted_formula->sampled_expected_value;

			double diff = weighted_formula->subjective_probability
					- weighted_formula->sampled_expected_value;

			diff = gradient[i];
			double delta_weight = WEIGHT_LEARNING_RATE / (1.0 + log(
					(double) (it + 1))) * (diff
			//- 0.01* weighted_formula->weight
					);
			weighted_formula->weight = weighted_formula->weight + delta_weight;
			// update the weight of all the ground clauses belonging to the formula
			ground_clause_t *ground_clause =
					weighted_formula->ground_clause_list;
			while (ground_clause != NULL) {
				if (ground_clause->clause->weight != 0) // if not hard clause change its weight
				{
					ground_clause->clause->weight += delta_weight;
					//TODO: change the value if we get 0.0
				}
				ground_clause = ground_clause->next;
			}
			// Set the rule's weight if it is not a ground clause
			if (!weighted_formula->is_ground) {
				weighted_formula->clausified_formula.rule->weight
						= weighted_formula->weight;
			}
			//double weight_diff = previous_weights[k] -  weighted_formula->sampled_expected_value;
			// double new_diff = diff
			//- 0.01 * weighted_formula->weight
			;
			// error += new_diff > 0 ? new_diff : -new_diff;
			//					previous_weights[k] =  weighted_formula->sampled_expected_value;;
			//			reset_weighted_formula(weighted_formula);
			if (it % 100 == 0) {
				double avg = 0.0;
				for (j = 0; j < K; ++j) {
					avg += last_K_expected_values[i][j];
				}
				avg /= K;
				double expectation_diff = avg - avg_last_K_expected_values[i];
				error += expectation_diff > 0 ? expectation_diff
						: -expectation_diff;
				avg_last_K_expected_values[i] = avg;
				print_weighted_formula(weighted_formula, table);
			}
		}

		reset_query_instance_table(&table->query_instance_table);
		for (i = 0; i < query_table->num_queries; ++i) {
			free_samp_query(query_table->query[i]);
		}
		query_table->num_queries = 0;
		if (it % 100 == 0) {
			output("error=%f\n", error);
		}

		if (it % 100 != 0) {
			error = 100000000000.0;
		}

		it++;

	} while (error > STOPPING_ERROR && it < MAX_IT);
	output("\n\nAfter weight learning:\n");
	output("error=%f\tit=%d\n", error, it);

	//	dump_clause_table(table);
	//	dump_rule_table(table);

	weighted_formula = first_weighted_formula;
	while (weighted_formula != NULL) {
		print_weighted_formula(weighted_formula, table);
		free_weighted_formula(weighted_formula);
		weighted_formula = weighted_formula->next;
	}
	if (subjective_probabilities_available) {
		compute_covariance_matrix();
		print_covariance_matrix();
		free_covariance_matrix();
	}
	if (training_data_available && USE_PLL) {
		free_pll_stats_fields(&pll_stats);
	}
}

// Callback function for the L-BFGS library.
// Called when L-BFGS needs the value of the objective function (PLL + log of the prior)
// and the gradient.
static lbfgsfloatval_t lbfgs_evaluate(void *instance, const lbfgsfloatval_t *x,
		lbfgsfloatval_t *g, const int n, const lbfgsfloatval_t step) {
	int i;
	lbfgsfloatval_t fx;
	double gradient[n];
	samp_table_t *table = (samp_table_t*) instance;
	query_table_t* query_table = &table->query_table;
	weighted_formula_t *weighted_formula;

	// Update weights of the weighted formulas and the associated rules / clauses
	for (i = 0, weighted_formula = first_weighted_formula; weighted_formula
			!= NULL; ++i, weighted_formula = weighted_formula->next) {
		set_formula_weight_lbfgs(weighted_formula, x[i]);
	}

	initialize_weighted_formulas(table);
	if (subjective_probabilities_available) {
		reset_covariance_matrix();
	}

	mc_sat(table, false, get_max_samples(), get_sa_probability(),
			get_sa_temperature(), get_rvar_probability(), get_max_flips(),
			get_max_extra_flips(), get_mcsat_timeout(),
			get_burn_in_steps(), get_samp_interval());
	set_empirical_expectation_of_weighted_formulae(first_weighted_formula,
			table);

	if (subjective_probabilities_available) {
		compute_covariance_matrix();
	}

	if (training_data_available && USE_PLL) {
		compute_pseudo_log_likelihood_statistics(training_data, &pll_stats);
	}

	compute_gradient(gradient);

	// set gradient for lbfgs, since it wants to
	// minimize we have to negate our objective function
	// and its gradients
	for (i = 0; i < n; ++i) {
		g[i] = -gradient[i];
	}

	reset_query_instance_table(&table->query_instance_table);
	for (i = 0; i < query_table->num_queries; ++i) {
		free_samp_query(query_table->query[i]);
	}
	query_table->num_queries = 0;

	fx = 0.0;
	if (subjective_probabilities_available) {
		fx += objective_expert();
	}
	fx += objective_data();

	return -fx;
}

static int lbfgs_progress(void *instance, const lbfgsfloatval_t *x,
		const lbfgsfloatval_t *g, const lbfgsfloatval_t fx,
		const lbfgsfloatval_t xnorm, const lbfgsfloatval_t gnorm,
		const lbfgsfloatval_t step, int n, int k, int ls) {
	samp_table_t *table = (samp_table_t*) instance;
	weighted_formula_t *weighted_formula;
	printf("Iteration %d:\n", k);
	for (weighted_formula = first_weighted_formula; weighted_formula != NULL; weighted_formula
			= weighted_formula->next) {
		print_weighted_formula(weighted_formula, table);
	}
	return 0;
}

void random_walk_step(lbfgsfloatval_t *x, int n) {
	int i;
	double max_step_size = MAX_LBFGS_RANDOM_STEP_SIZE;

	for (i = 0; i < n; ++i) {
		x[i] += max_step_size * (drand() - 0.5);
	}
}

extern void weight_training_lbfgs(training_data_t *data, samp_table_t* table) {
	int32_t i, ret = 0;
	lbfgsfloatval_t fx, best_fx;
	lbfgsfloatval_t *x, *best_x;
	lbfgs_parameter_t param;
	weighted_formula_t* weighted_formula;
	bool solution_found = false;
	// Number of allowed random_walks if stuck in local optimum
	int32_t attempts = 0;

	training_data = data;
	training_data_available = (training_data != NULL);

	set_subjective_probabilities_available();
	set_num_weighted_formulas();
	if (training_data_available) {
		add_training_data(training_data);
	}

	if (subjective_probabilities_available) {
		init_covariance_matrix();
	}

	if (training_data_available  && USE_PLL) {
		initialize_pll_stats(training_data, &pll_stats, &table->clause_table);
	}

	x = lbfgs_malloc(num_weighted_formulas);
	best_x = lbfgs_malloc(num_weighted_formulas);
	if (x == NULL) {
		printf("ERROR: Failed to allocate a memory block for variables.\n");
		// handle error
	}

	for (i = 0, weighted_formula = first_weighted_formula; weighted_formula
			!= NULL; ++i, weighted_formula = weighted_formula->next) {
		x[i] = weighted_formula->weight;
	}

	do {
		/* Initialize the parameters for the L-BFGS optimization. */
		lbfgs_parameter_init(&param);

		//param.min_step = 1e-20;
		//param.ftol = 1e-4;
		/*param.linesearch = LBFGS_LINESEARCH_BACKTRACKING;*/

		/*
		 Start the L-BFGS optimization; this will invoke the callback functions
		 evaluate() and progress() when necessary.
		 */
		ret = lbfgs(num_weighted_formulas, x, &fx, lbfgs_evaluate,
				lbfgs_progress, (void*) table, &param);

		/* Report the result. */
		printf("L-BFGS optimization terminated with status code = %d\n", ret);

		if (attempts == 0) {
			best_fx = fx;
			for (i = 0; i < num_weighted_formulas; ++i) {
				best_x[i] = x[i];
			}
		} else if (fx > best_fx) { // we are minimizing the negative log-likelihood
			best_fx = fx;
			for (i = 0; i < num_weighted_formulas; ++i) {
				best_x[i] = x[i];
			}
		}
		printf("\nObjective function value: %f\n\n", fx);

		switch (ret) {
		case LBFGSERR_ROUNDING_ERROR:
			printf(
					"LBFGSERR_ROUNDING_ERROR, A rounding error occurred; alternatively, no line-search step satisfies the sufficient decrease and curvature conditions.\n");
			solution_found = false;
			// algorithm got stuck, do random walk
			random_walk_step(x, num_weighted_formulas);
			break;
		case LBFGSERR_MINIMUMSTEP:
			printf(
					"LBFGSERR_MINIMUMSTEP, The line-search step became smaller than lbfgs_parameter_t::min_step.\n");
			solution_found = false;
			// algorithm got stuck, do random walk
			random_walk_step(x, num_weighted_formulas);
			break;
		case LBFGSERR_MAXIMUMSTEP:
			printf(
					"LBFGSERR_MAXIMUMSTEP, The line-search step became larger than lbfgs_parameter_t::max_step.\n");
			solution_found = true;
			break;
		default:
			solution_found = true;
			break;
		}
	} while (!solution_found && attempts++ < MAX_LBFGS_RANDOM_WALK_ATTEMPTS);

	printf("Best objective function value: %f\n", best_fx);
	for (i = 0, weighted_formula = first_weighted_formula; weighted_formula
			!= NULL; weighted_formula = weighted_formula->next, ++i) {
		set_formula_weight_lbfgs(weighted_formula, best_x[i]);
		print_weighted_formula(weighted_formula, table);
	}
	lbfgs_free(x);
	lbfgs_free(best_x);
	if (training_data_available && USE_PLL) {
		free_pll_stats_fields(&pll_stats);
	}
}

void init_covariance_matrix() {
	int i;
	weighted_formula_t* weighted_formula;
	if (covariance_matrix != NULL) {
		safe_free(covariance_matrix);
	}
	covariance_matrix = (covariance_matrix_t*) safe_malloc(
			sizeof(covariance_matrix_t));

	// initialize the size of the matrix
	covariance_matrix->N = 0;
	for (weighted_formula = first_weighted_formula; weighted_formula != NULL; weighted_formula
			= weighted_formula-> next) {
		covariance_matrix->N++;
	}
	covariance_matrix->size = covariance_matrix->N * (covariance_matrix->N + 1)
			/ 2;

	covariance_matrix->features = (weighted_formula_t***) safe_malloc(
			sizeof(weighted_formula_t**) * covariance_matrix->size);

	covariance_matrix->exy = (double*) safe_malloc(
			sizeof(double) * covariance_matrix->size);

	covariance_matrix->ex = (double*) safe_malloc(
			sizeof(double) * covariance_matrix->size);

	covariance_matrix->ey = (double*) safe_malloc(
			sizeof(double) * covariance_matrix->size);

	covariance_matrix->cov = (double*) safe_malloc(
			sizeof(double) * covariance_matrix->size);

	reset_covariance_matrix();
	i = 0;
	for (i = 0; i < covariance_matrix->size; ++i) {
		covariance_matrix->features[i] = (weighted_formula_t**) safe_malloc(
				sizeof(weighted_formula_t*) * 2);
	}

	weighted_formula_t *weighted_formula_ptr1, *weighted_formula_ptr2;
	i = 0;
	for (weighted_formula_ptr1 = first_weighted_formula; weighted_formula_ptr1
			!= NULL; weighted_formula_ptr1 = weighted_formula_ptr1-> next) {
		for (weighted_formula_ptr2 = weighted_formula_ptr1; weighted_formula_ptr2
				!= NULL; weighted_formula_ptr2 = weighted_formula_ptr2-> next) {
			covariance_matrix->features[i][0] = weighted_formula_ptr2;
			covariance_matrix->features[i][1] = weighted_formula_ptr1;
			i++;
		}
	}
	assert(i == covariance_matrix->size);
}

extern void update_covariance_matrix_statistics(samp_table_t *table) {
	atom_table_t *atom_table;
	samp_query_instance_t *qinst;
	int32_t i, j, k, l;
	//bool fval;

	// We only need the covariance matrix when do weight learning with subjective probabilities
	if (!subjective_probabilities_available)
		return;

	if (covariance_matrix == NULL)
		return;

	atom_table = &table->atom_table;

	double normalized_true_groundings[covariance_matrix->N];
	l = 0;
	weighted_formula_t* weighted_formula;
	for (weighted_formula = first_weighted_formula; weighted_formula != NULL; weighted_formula
			= weighted_formula-> next, l++) {
		normalized_true_groundings[l] = 0.0;
		ground_query_t *ground_query;
		for (ground_query = weighted_formula->qinst_list; ground_query != NULL; ground_query
				= ground_query->next) {
			// Each query instance is an array of array of literals
			qinst = ground_query->qinst;
			//fval = false;
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
				normalized_true_groundings[l]++;
			}
			done: continue;
		}
		normalized_true_groundings[l] /= weighted_formula->num_groundings;
	}

	k = 0;
	for (i = 0; i < covariance_matrix->N; ++i) {
		for (j = i; j < covariance_matrix->N; ++j) {
			covariance_matrix->exy[k] += normalized_true_groundings[j]
					* normalized_true_groundings[i];
			covariance_matrix->ex[k] += normalized_true_groundings[j];
			covariance_matrix->ey[k] += normalized_true_groundings[i];
			k++;
		}
	}
	covariance_matrix->n_samples++;
}

// Normalize exy, and compute cov values
void compute_covariance_matrix() {
	int i;
	for (i = 0; i < covariance_matrix->size; ++i) {
		covariance_matrix->exy[i] /= (double) covariance_matrix->n_samples;
		covariance_matrix->ex[i] /= (double) covariance_matrix->n_samples;
		covariance_matrix->ey[i] /= (double) covariance_matrix->n_samples;
		covariance_matrix->cov[i] = covariance_matrix->exy[i]
				- covariance_matrix->ex[i] * covariance_matrix->ey[i];
	}
}

void free_covariance_matrix() {
	int i;
	for (i = 0; i < covariance_matrix->size; ++i) {
		safe_free(covariance_matrix->features[i]);
	}
	safe_free(covariance_matrix->features);
	safe_free(covariance_matrix->exy);
	safe_free(covariance_matrix->ex);
	safe_free(covariance_matrix->ey);
	safe_free(covariance_matrix->cov);
	safe_free(covariance_matrix);
}

void print_covariance_matrix() {
	int i, j, k = 0;

	output("\nEXY:\n");
	for (i = 0; i < covariance_matrix->N; ++i) {
		for (j = 0; j < covariance_matrix->N; ++j) {
			if (j >= i) {
				output("%f", covariance_matrix->exy[k]);
				k++;
			} else {
				output("X");
			}
			output(" ");
		}
		output("\n");
	}

	output("\nEY:\n");
	k = 0;
	for (i = 0; i < covariance_matrix->N; ++i) {
		for (j = 0; j < covariance_matrix->N; ++j) {
			if (j >= i) {
				output("%f", covariance_matrix->ey[k]);
				k++;
			} else {
				output("X");
			}
			output(" ");
		}
		output("\n");
	}

	output("\nEX:\n");
	k = 0;
	for (i = 0; i < covariance_matrix->N; ++i) {
		for (j = 0; j < covariance_matrix->N; ++j) {
			if (j >= i) {
				output("%f", covariance_matrix->ex[k]);
				k++;
			} else {
				output("X");
			}
			output(" ");
		}
		output("\n");
	}

	output("\nCOV:\n");
	k = 0;
	for (i = 0; i < covariance_matrix->N; ++i) {
		for (j = 0; j < covariance_matrix->N; ++j) {
			if (j >= i) {
				output("%f", covariance_matrix->cov[k]);
				k++;
			} else {
				output("X");
			}
			output(" ");
		}
		output("\n");
	}
	fflush(stdout);

}

input_formula_t* deep_copy_formula(input_formula_t* formula) {
	if (formula == NULL)
		return NULL;
	input_formula_t* copied_f = (input_formula_t*) safe_malloc(
			sizeof(input_fmla_t));
	if (formula->vars == NULL) {
		copied_f->vars = NULL;
	} else {
		int vlen;
		for (vlen = 0; formula->vars[vlen] != NULL; vlen++) {
		}
		copied_f->vars = (var_entry_t **) safe_malloc(
				(vlen + 1) * sizeof(var_entry_t *));
		int i;
		for (i = 0; i < vlen; i++) {
			copied_f->vars[i]
					= (var_entry_t *) safe_malloc(sizeof(var_entry_t));
			int len = strlen(formula->vars[i]->name);
			copied_f->vars[i]->name = (char*) safe_malloc(
					(len + 1) * sizeof(char));
			strcpy(copied_f->vars[i]->name, formula->vars[i]->name);
			copied_f->vars[i]->sort_index = formula->vars[i]->sort_index;
		}
		copied_f->vars[vlen] = NULL;
	}
	copied_f->fmla = deep_copy_fmla(formula->fmla);
	return copied_f;
}

input_fmla_t* deep_copy_fmla(input_fmla_t* fmla) {
	if (fmla == NULL)
		return NULL;
	input_fmla_t* copied_f = (input_fmla_t*) safe_malloc(sizeof(input_fmla_t));

	copied_f->atomic = fmla->atomic;

	copied_f->ufmla = (input_ufmla_t *) safe_malloc(sizeof(input_ufmla_t));
	if (copied_f->atomic) {

		copied_f->ufmla->atom = (input_atom_t *) safe_malloc(
				sizeof(input_atom_t));

		if (fmla->ufmla->atom->args == NULL) {
			copied_f->ufmla->atom->args = NULL;
		} else {
			int vlen;
			for (vlen = 0; fmla->ufmla->atom->args[vlen] != NULL; vlen++) {
			}
			copied_f->ufmla->atom->args = (char **) safe_malloc(
					(vlen + 1) * sizeof(char *));
			int i;
			for (i = 0; i < vlen; i++) {
				int len = strlen(fmla->ufmla->atom->args[i]);
				copied_f->ufmla->atom->args[i] = (char*) safe_malloc(
						(len + 1) * sizeof(char));
				strcpy(copied_f->ufmla->atom->args[i],
						fmla->ufmla->atom->args[i]);
			}
			copied_f->ufmla->atom->args[vlen] = NULL;
		}

		copied_f->ufmla->atom->builtinop = fmla->ufmla->atom->builtinop;
		int len = strlen(fmla->ufmla->atom->pred);
		copied_f->ufmla->atom->pred = (char*) safe_malloc(
				(len + 1) * sizeof(char));
		strcpy(copied_f->ufmla->atom->pred, fmla->ufmla->atom->pred);
	} else {

		copied_f->ufmla->cfmla = (input_comp_fmla_t *) safe_malloc(
				sizeof(input_comp_fmla_t));
		copied_f->ufmla->cfmla->op = fmla->ufmla->cfmla->op;
		copied_f->ufmla->cfmla->arg1 = deep_copy_fmla(fmla->ufmla->cfmla->arg1);
		copied_f->ufmla->cfmla->arg2 = deep_copy_fmla(fmla->ufmla->cfmla->arg2);
	}

	return copied_f;
}

void reset_covariance_matrix() {
	int i;

	for (i = 0; i < covariance_matrix->size; ++i) {
		covariance_matrix->exy[i] = 0.0;
		covariance_matrix->ex[i] = 0.0;
		covariance_matrix->ey[i] = 0.0;
		covariance_matrix->cov[i] = -1.0;
	}
	covariance_matrix->n_samples = 0;
}

// It uses the available weighted_formulas and the computed covariance matrix to determine the gradient.
// The result is returned in the gradient double array.
// dmu/dtheta * dO/dmu
void compute_gradient(double* gradient) {
	int i, j, k;

	double mean_diffs[num_weighted_formulas];
	weighted_formula_t *weighted_formula = first_weighted_formula;

	for (i = 0; i < num_weighted_formulas; ++i) {
		gradient[i] = 0.0;
	}

	if (subjective_probabilities_available) {
		for (i = 0; i < num_weighted_formulas; ++i, weighted_formula
				= weighted_formula->next) {
			if (weighted_formula->subjective_probability == NOT_AVAILABLE) { // don't care about the subjective probability since it won't contribute to the gradient
				mean_diffs[i] = 0.0;
			} else {
				mean_diffs[i] = weighted_formula->subjective_probability
						- weighted_formula->sampled_expected_value;
			}
		}

		assert(weighted_formula == NULL);
		k = 0;
		for (i = 0; i < num_weighted_formulas; ++i) {
			for (j = i; j < num_weighted_formulas; ++j) {
				// g_i += M_ij * d_j
				gradient[i] += covariance_matrix->cov[k] * mean_diffs[j];

				if (i != j) {
					// g_j += M_ij * d_i, since M_ij = M_ji
					gradient[j] += covariance_matrix->cov[k] * mean_diffs[i];
				}
				k++;
			}
		}

		// add the expert's opinion's weight
		for (i = 0; i < num_weighted_formulas; ++i) {
			gradient[i] *= EXPERTS_WEIGHT;
		}
	}
	if (training_data_available) {
		if (USE_PLL)
		{
				// pseudo log-likelihood's gradient
				double pll_grad[num_weighted_formulas];
				k = 0;
				for (weighted_formula = first_weighted_formula; weighted_formula != NULL; weighted_formula
						= weighted_formula->next, ++k) {
					pll_grad[k] = 0.0;
					for (i = 0; i < pll_stats.training_sets; ++i) {
						for (j = 0; j < pll_stats.num_vars; ++j) {
							pll_grad[k]
									+= (weighted_formula->feature_counts[i].total_count
											- pll_stats.pseudo_likelihoods_var_zero[i][j]
													* weighted_formula->feature_counts[i].count_var_zero[j]
											- pll_stats.pseudo_likelihoods_var_one[i][j]
													* weighted_formula->feature_counts[i].count_var_one[j]);
						}
					}
					printf(
							"W:%f\tPr:%f\tSPr:%f\tDPr:%f\tPLLG:%f\tSG:%f\n",
							weighted_formula->weight,
							weighted_formula->sampled_expected_value,
							weighted_formula->subjective_probability,
							weighted_formula->data_expected_value,
							pll_grad[k] / (weighted_formula->num_groundings
									* pll_stats.training_sets), gradient[k]);
					fflush(stdout);
					gradient[k] += pll_grad[k] / (weighted_formula->num_groundings
							* pll_stats.training_sets);

				}
		} else {
		//	 log-likelihood's gradient
		k = 0;
		for (weighted_formula = first_weighted_formula; weighted_formula
				!= NULL; weighted_formula = weighted_formula->next, ++k) {
			gradient[k] += training_data->num_data_sets * weighted_formula->num_groundings
					 * (weighted_formula->data_expected_value
					- weighted_formula->sampled_expected_value);
			// normalize with number of groundings
			gradient[k] /= weighted_formula->num_groundings;

			printf("W:%f\tPr:%f\tSPr:%f\tDPr:%f\tG:%f\n",
					weighted_formula->weight,
					weighted_formula->sampled_expected_value,
					weighted_formula->subjective_probability,
					weighted_formula->data_expected_value, gradient[k]);
		}
		printf("\n");
		}
	}

//	// regularization
//	weighted_formula = first_weighted_formula;
//	for (i = 0; i < num_weighted_formulas; ++i) {
//		gradient[i] -= weighted_formula->weight * 0.1;
//		weighted_formula = weighted_formula->next;
//	}


}

void add_training_data(training_data_t *training_data) {
	int i, j, k;
	weighted_formula_t *weighted_formula;
	ground_clause_t *ground_clause;
	samp_clause_t *clause;
	samp_bvar_t var; // index of the atom
	bool positive;
	int ground_clause_cnt;

	if (training_data == NULL) {
		for (weighted_formula = first_weighted_formula; weighted_formula
				!= NULL; weighted_formula = weighted_formula-> next) {
			weighted_formula->data_expected_value = NOT_AVAILABLE;
		}
		return;
	}

	// compute the expected feature counts from the training data
	for (weighted_formula = first_weighted_formula; weighted_formula != NULL; weighted_formula
			= weighted_formula-> next) {
		weighted_formula->num_training_sets = training_data->num_data_sets;
		weighted_formula->feature_counts = (feature_counts_t*) safe_malloc(
				training_data->num_data_sets * sizeof(feature_counts_t));
		weighted_formula->data_expected_value = 0.0;
		ground_clause_cnt = 0;

		for (i = 0; i < training_data->num_data_sets; ++i) {
			weighted_formula->feature_counts[i].count_var_zero
					= (int32_t*) safe_malloc(
							training_data->num_evidence_atoms * sizeof(int32_t));
			weighted_formula->feature_counts[i].count_var_one
					= (int32_t*) safe_malloc(
							training_data->num_evidence_atoms * sizeof(int32_t));

			weighted_formula->feature_counts[i].total_count = 0;

			for (k = 0; k < training_data->num_evidence_atoms; ++k) {
				evidence_truth_value_t orig_truth_value =
						training_data->evidence_atoms[i][k].truth_value;

				weighted_formula->feature_counts[i].count_var_zero[k] = 0;
				training_data->evidence_atoms[i][k].truth_value = ev_false;
				for (ground_clause = weighted_formula->ground_clause_list; ground_clause
						!= NULL; ground_clause = ground_clause->next) {
					clause = ground_clause->clause;
					for (j = 0; j < clause->numlits; ++j) {
						positive = is_pos(clause->disjunct[j]);
						var = var_of(clause->disjunct[j]);
						if (positive ? training_data->evidence_atoms[i][var].truth_value
								== ev_true
								: training_data->evidence_atoms[i][var].truth_value
										== ev_false) {
							// found a true grounding
							weighted_formula->feature_counts[i].count_var_zero[k]++;
							break;
						}
					}
				}

				weighted_formula->feature_counts[i].count_var_one[k] = 0;
				training_data->evidence_atoms[i][k].truth_value = ev_true;
				for (ground_clause = weighted_formula->ground_clause_list; ground_clause
						!= NULL; ground_clause = ground_clause->next) {
					clause = ground_clause->clause;
					for (j = 0; j < clause->numlits; ++j) {
						positive = is_pos(clause->disjunct[j]);
						var = var_of(clause->disjunct[j]);
						if (positive ? training_data->evidence_atoms[i][var].truth_value
								== ev_true
								: training_data->evidence_atoms[i][var].truth_value
										== ev_false) {
							// found a true grounding
							weighted_formula->feature_counts[i].count_var_one[k]++;
							break;
						}
					}
				}
				training_data->evidence_atoms[i][k].truth_value
						= orig_truth_value;
			}

			for (ground_clause = weighted_formula->ground_clause_list; ground_clause
					!= NULL; ground_clause = ground_clause->next) {
				clause = ground_clause->clause;
				for (j = 0; j < clause->numlits; ++j) {
					positive = is_pos(clause->disjunct[j]);
					var = var_of(clause->disjunct[j]);
					if (positive ? training_data->evidence_atoms[i][var].truth_value
							== ev_true
							: training_data->evidence_atoms[i][var].truth_value
									== ev_false) {
						// found a true grounding
						weighted_formula->feature_counts[i].total_count++;
						break;
					}
				}
				ground_clause_cnt++;
			}
		}

		for (i = 0; i < training_data->num_data_sets; ++i) {
			weighted_formula->data_expected_value
					+= weighted_formula->feature_counts[i].total_count;
		}
		weighted_formula->data_expected_value /= (double) ground_clause_cnt;

	}
}

void initialize_pll_stats(training_data_t* training_data,
		pseudo_log_likelihood_stats_t *pll_stats, rule_inst_table_t *rule_inst_table) {
	int32_t i, j, k;
	rule_inst_t *rinst;
	samp_clause_t *clause;
	samp_bvar_t var;

	pll_stats->training_sets = training_data->num_data_sets;
	pll_stats->num_vars = training_data->num_evidence_atoms;

	int32_t counters[pll_stats->num_vars];

	for (i = 0; i < pll_stats->num_vars; ++i) {
		counters[i] = 0;
	}

	for (j = 0; j < clause_table->num_clauses; ++j) {
		clause = clause_table->samp_clauses[j];
		for (k = 0; k < clause->numlits; ++k) {
			var = var_of(clause->disjunct[k]);
			counters[var]++;
		}
	}

	pll_stats->clause_cnts = (int32_t*) safe_malloc(
			pll_stats->num_vars * sizeof(int32_t));
	pll_stats->clauses = (samp_clause_t***) safe_malloc(
			pll_stats->num_vars * sizeof(samp_clause_t**));
	for (i = 0; i < pll_stats->num_vars; ++i) {
		pll_stats->clauses[i] = (samp_clause_t**) safe_malloc(
				counters[i] * sizeof(samp_clause_t*));
		pll_stats->clause_cnts[i] = counters[i];
		counters[i] = 0;
	}

	for (j = 0; j < clause_table->num_clauses; ++j) {
		clause = clause_table->samp_clauses[j];
		for (k = 0; k < clause->numlits; ++k) {
			var = var_of(clause->disjunct[k]);
			pll_stats->clauses[var][counters[var]++] = clause;
		}
	}

	pll_stats->pseudo_likelihoods = (double**) safe_malloc(
			pll_stats->training_sets * sizeof(double*));
	pll_stats->pseudo_likelihoods_var_zero = (double**) safe_malloc(
			pll_stats->training_sets * sizeof(double*));
	pll_stats->pseudo_likelihoods_var_one = (double**) safe_malloc(
			pll_stats->training_sets * sizeof(double*));

	for (i = 0; i < pll_stats->training_sets; ++i) {
		pll_stats->pseudo_likelihoods[i] = (double*) safe_malloc(
				pll_stats->num_vars * sizeof(double));
		pll_stats->pseudo_likelihoods_var_zero[i] = (double*) safe_malloc(
				pll_stats->num_vars * sizeof(double));
		pll_stats->pseudo_likelihoods_var_one[i] = (double*) safe_malloc(
				pll_stats->num_vars * sizeof(double));
	}

}

void free_pll_stats_fields(pseudo_log_likelihood_stats_t *pll_stats) {
	int i;

	for (i = 0; i < pll_stats->num_vars; ++i) {
		safe_free(pll_stats->clauses[i]);
	}
	safe_free(pll_stats->clauses);
	safe_free(pll_stats->clause_cnts);

	for (i = 0; i < pll_stats->training_sets; ++i) {
		safe_free(pll_stats->pseudo_likelihoods[i]);
		safe_free(pll_stats->pseudo_likelihoods_var_zero[i]);
		safe_free(pll_stats->pseudo_likelihoods_var_one[i]);
	}
	safe_free(pll_stats->pseudo_likelihoods);
	safe_free(pll_stats->pseudo_likelihoods_var_zero);
	safe_free(pll_stats->pseudo_likelihoods_var_one);

}

// compute the objective function for the subjective probability matching part
double objective_expert() {
	weighted_formula_t *weighted_formula;
	double fx = 0.0;

	// compute the objective function, truncated Gaussian
	for (weighted_formula = first_weighted_formula; weighted_formula != NULL; weighted_formula
			= weighted_formula->next) {
		// updated the average of the last expected values in the circular buffer
		double diff = 0.0;
		if (weighted_formula->subjective_probability != NOT_AVAILABLE) {
			diff = weighted_formula->subjective_probability
					- weighted_formula->sampled_expected_value;
		}
		fx += (diff * diff);

//		// regularization
//		fx -= 0.05 * (weighted_formula->weight * weighted_formula->weight);
	}
	return EXPERTS_WEIGHT * fx;
}

// compute the objective function for the training data
double objective_data() {
	// compute pseudo-log-likelihood
	double pll = 0.0;
	int i, j;
	for (i = 0; i < pll_stats.training_sets; ++i) {
		for (j = 0; j < pll_stats.num_vars; ++j) {
			pll += log(pll_stats.pseudo_likelihoods[i][j]);
		}
	}

	return pll;
}

// P(X_l| MB(X_l)), P(X_l = 0 | MB(X_l)) and P(X_l = 1 | MB(X_l)) values for every data set
void compute_pseudo_log_likelihood_statistics(training_data_t* training_data,
		pseudo_log_likelihood_stats_t* pll_stats) {
	int i, j, k, l;
	samp_clause_t *clause;
	double pos_weights, neg_weights;
	double normalization;
	bool positive;
	samp_bvar_t var;

	for (i = 0; i < pll_stats->training_sets; ++i) {
		for (j = 0; j < pll_stats->num_vars; ++j) {
			pll_stats->pseudo_likelihoods[i][j] = 0.0;
			pll_stats->pseudo_likelihoods_var_zero[i][j] = 0.0;
			pll_stats->pseudo_likelihoods_var_one[i][j] = 0.0;

			pos_weights = 0.0;
			neg_weights = 0.0;

			evidence_truth_value_t orig_truth_value =
					training_data->evidence_atoms[i][j].truth_value;

			training_data->evidence_atoms[i][j].truth_value = ev_false;
			for (k = 0; k < pll_stats->clause_cnts[j]; ++k) {
				clause = pll_stats->clauses[j][k];

				for (l = 0; l < clause->numlits; ++l) {
					positive = is_pos(clause->disjunct[l]);
					var = var_of(clause->disjunct[l]);
					if (positive ? training_data->evidence_atoms[i][var].truth_value
							== ev_true
							: training_data->evidence_atoms[i][var].truth_value
									== ev_false) {
						// clause is satisfied
						neg_weights += clause->weight;
						break;
					}
				}
			}

			training_data->evidence_atoms[i][j].truth_value = ev_true;
			for (k = 0; k < pll_stats->clause_cnts[j]; ++k) {
				clause = pll_stats->clauses[j][k];

				for (l = 0; l < clause->numlits; ++l) {
					positive = is_pos(clause->disjunct[l]);
					var = var_of(clause->disjunct[l]);
					if (positive ? training_data->evidence_atoms[i][var].truth_value
							== ev_true
							: training_data->evidence_atoms[i][var].truth_value
									== ev_false) {
						// clause is satisfied
						pos_weights += clause->weight;
						break;
					}
				}
			}

			training_data->evidence_atoms[i][j].truth_value = orig_truth_value;

			normalization = exp(pos_weights) + exp(neg_weights);
			pll_stats->pseudo_likelihoods_var_zero[i][j] = exp(neg_weights)
					/ normalization;
			pll_stats->pseudo_likelihoods_var_one[i][j] = exp(pos_weights)
					/ normalization;

			if (orig_truth_value == ev_true) {
				pll_stats->pseudo_likelihoods[i][j]
						= pll_stats->pseudo_likelihoods_var_one[i][j];
			} else {
				pll_stats->pseudo_likelihoods[i][j]
						= pll_stats->pseudo_likelihoods_var_zero[i][j];
			}
		}

	}
}

void set_formula_weight(weighted_formula_t *weighted_formula, double x) {
	double delta_weight = x - weighted_formula->weight;
	weighted_formula->weight = x;

	// update the weight of all the ground clauses belonging to the formula
	ground_clause_t *ground_clause = weighted_formula->ground_clause_list;
	while (ground_clause != NULL) {
		if (ground_clause->clause->weight != 0) // if not hard clause change its weight
		{
			ground_clause->clause->weight += delta_weight;
		}
		ground_clause = ground_clause->next;
	}
	// Set the rule's weight if it is not a ground clause
	if (!weighted_formula->is_ground) {
		weighted_formula->clausified_formula.rule->weight
				= weighted_formula->weight;
	}
}

void set_formula_weight_lbfgs(weighted_formula_t *weighted_formula,
		lbfgsfloatval_t x) {
	double delta_weight = x - weighted_formula->weight;
	weighted_formula->weight = x;

	// update the weight of all the ground clauses belonging to the formula
	ground_clause_t *ground_clause = weighted_formula->ground_clause_list;
	while (ground_clause != NULL) {
		if (ground_clause->clause->weight != 0) // if not hard clause change its weight
		{
			ground_clause->clause->weight += delta_weight;
		}
		ground_clause = ground_clause->next;
	}
	// Set the rule's weight if it is not a ground clause
	if (!weighted_formula->is_ground) {
		weighted_formula->clausified_formula.rule->weight
				= weighted_formula->weight;
	}
}

void set_subjective_probabilities_available() {
	weighted_formula_t *weighted_formula = first_weighted_formula;
	subjective_probabilities_available = false;
	while (weighted_formula != NULL) {
		if (weighted_formula->subjective_probability != NOT_AVAILABLE) {
			subjective_probabilities_available = true;
			return;
		}
		weighted_formula = weighted_formula->next;
	}
}

void set_num_weighted_formulas() {
	weighted_formula_t *weighted_formula = first_weighted_formula;
	num_weighted_formulas = 0;
	while (weighted_formula != NULL) {
		num_weighted_formulas++;
		weighted_formula = weighted_formula->next;
	}
}
