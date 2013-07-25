/*
 * weight_learning.h
 *
 *  Created on: Jul 29, 2011
 *      Author: papai
 */

#ifndef WEIGHT_LEARNING_H_
#define WEIGHT_LEARNING_H_

#include "input.h"
#include "vectors.h"
#include "tables.h"
#include "training_data.h"
#include "lbfgs.h"


// this should be specified by the user, set it to 1 for L-BFGS, to 0 for gradient
// ascent, to use L-BFGS you should also set USE_PLL to 1
#define LBFGS_MODE 0

// When L-BFGS returns with an error we do a random walk (simulated annealing)
#define MAX_LBFGS_RANDOM_STEP_SIZE 0.5
#define MAX_LBFGS_RANDOM_WALK_ATTEMPTS 30

// for gradient ascent
#define WEIGHT_LEARNING_RATE 0.1
#define STOPPING_ERROR 0.001
#define MAX_IT 10000
// use pseudo-loglikelihood, note that L-BFGS should always use PLL
#define USE_PLL 0


// measures the relevance of the expert's subjective probabilities in the objective function
// and in the gradient
#define EXPERTS_WEIGHT 1000.0

// used when the subjective probability or the data expected value is not provided (has to be double and
// outside of the [0,1] interval
#define NOT_AVAILABLE -1.0

// for pseudo-likelihood computation
typedef struct feature_counts_s
{
	int32_t total_count;		// ni(x), number of all groundings
	int32_t *count_var_zero;	// ni(x[X_l=0]), number of groundings when variable is 1
	int32_t *count_var_one;		// ni(x[X_l=1]), number of groundings when variable is 0

} feature_counts_t;

// stores the statistics needed to do pseudo likelihood computation
typedef struct pseudo_log_likelihood_stats_s
{
	int32_t training_sets;

	// number of ground atoms
	int32_t num_vars;

	// the number of clauses belonging to each variable
	int32_t *clause_cnts;

	// the pointers to clauses belonging to each variable
	// needed to find the Markov blanket of the variable
	samp_clause_t ***clauses;

	// pseudo likelihood of each variable in each training set
	double **pseudo_likelihoods;

	// pseudo likelihood of each variable in each training set
	// when the variable is set to 0
	double **pseudo_likelihoods_var_zero;

	// pseudo likelihood of each variable in each training set
	// when the variable is set to 1
	double **pseudo_likelihoods_var_one;
} pseudo_log_likelihood_stats_t;

typedef struct covariance_matrix_s {
	// size N x N matrix
	int N;

	// the number of elements in the array: N(N+1)/2
	int size;

	// contains the array of all pairs exactly once, i.e., has N(N+1)/2 elements
	// it stores the upper triangle matrix from row by row from the top
	struct weighted_formula_s*** features;

	// number of times statistics were updated
	int32_t n_samples;

	// the corresponding E[X*Y] and the E[X] and E[Y] values which should agree with the ones in weighted_formula_t
	double *exy;
	double *ex;
	double *ey;

	// the covariances
	double *cov;

} covariance_matrix_t;

// contains a non-ground rule or ground clause
typedef union clausified_formula_s {
	struct samp_rule_s* rule;
	struct samp_clause_s* clause;
} clausified_formula_t;

// the ground clauses belonging to the weighted FOL formula
typedef struct ground_clause_s {
	struct ground_clause_s *next;
	samp_clause_t* clause;
} ground_clause_t;

// the ground queries belonging to the weighted FOL formula
typedef struct ground_query_s {
	struct ground_query_s *next;
	samp_query_instance_t* qinst;
} ground_query_t;

typedef struct weighted_formula_s {
	// the formula in input format
	input_formula_t *formula;
	char **frozen;
	char *source;
	double weight;

	// expected value coming from the data set
	double data_expected_value;

	// subjective probability coming from the expert / NOT_AVAILABLE when
	// the subjective probability is not provided
	double subjective_probability;

	// computed in each iteration from the result of MC-SAT
	double sampled_expected_value;

	// number of all groundings of the formula
	int32_t num_groundings;

	// true if the formula is ground, then clausified formula is a ground clause
	// not a rule
	bool is_ground;
	clausified_formula_t clausified_formula;

	// the list of ground clauses belonging to the formula
	ground_clause_t *ground_clause_list;

	// the list of ground queries belonging to the formula
	ground_query_t *qinst_list;

	int32_t num_training_sets;
	// the size of the array is stored in num_training_sets, needed to compute PLL
	feature_counts_t *feature_counts;

	// pointer to the next link in the linked list
	struct weighted_formula_s *next;
} weighted_formula_t;

extern double get_probability_of_quantified_formula(pvector_t ask_buffer,
		input_formula_t *fmla, samp_table_t *table);
extern void set_empirical_expectation_of_weighted_formulae(
		weighted_formula_t* first_weighted_formula, samp_table_t *table);
extern void add_ground_clause_to_weighted_formula(
		weighted_formula_t* weighted_formula, samp_clause_t* clause);
extern void add_query_instance_to_weighted_formula(
		weighted_formula_t* weighted_formula, samp_query_instance_t* qinst);
extern void reset_weighted_formula(weighted_formula_t *weighted_formula);
extern void free_weighted_formula(weighted_formula_t *weighted_formula);
extern void print_weighted_formula(weighted_formula_t *weighted_formula,
		samp_table_t* table);

extern void initialize_weighted_formulas(samp_table_t* table);
extern void gradient_ascent(training_data_t *training_data, samp_table_t* table);

extern void weight_training_lbfgs(training_data_t *training_data, samp_table_t* table);

extern void add_weighted_formula(samp_table_t* table, input_add_fdecl_t* decl);

void init_covariance_matrix();
void compute_covariance_matrix();
void free_covariance_matrix();
void print_covariance_matrix();
extern void update_covariance_matrix_statistics(samp_table_t *table);
void reset_covariance_matrix();

void compute_gradient(double *gradient);

void add_training_data(training_data_t *training_data);

void initialize_pll_stats(training_data_t* training_data, pseudo_log_likelihood_stats_t *pll_stats, clause_table_t *clause_table);
void free_pll_stats_fields(pseudo_log_likelihood_stats_t *pll_stats);
void compute_pseudo_log_likelihood_statistics(training_data_t* training_data, pseudo_log_likelihood_stats_t* pll_stats);

double objective_expert();
double objective_data();

void set_formula_weight(weighted_formula_t *weighted_formula, double x);
void set_formula_weight_lbfgs(weighted_formula_t *weighted_formula, lbfgsfloatval_t x);

void set_subjective_probabilities_available();
void set_num_weighted_formulas();

input_formula_t* deep_copy_formula(input_formula_t* formula);
input_fmla_t* deep_copy_fmla(input_fmla_t* fmla);


#endif /* WEIGHT_LEARNING_H_ */
