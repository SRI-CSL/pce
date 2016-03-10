#ifndef __PARAMS_H 
#define __PARAMS_H 1

/*
 * MCSAT and weight learning parameters: Keep all of these parameters
 * in a struct that (ultimately) can be passed around as an argument,
 * or even as a slot in samp_tables.
 */

#define DEFAULT_MAX_SAMPLES 100
#define DEFAULT_SA_PROBABILITY .5
#define DEFAULT_SA_TEMPERATURE 5.0
#define DEFAULT_RVAR_PROBABILITY .05
#define DEFAULT_MAX_FLIPS 100
#define DEFAULT_MAX_EXTRA_FLIPS 5
#define DEFAULT_MCSAT_TIMEOUT 0
#define DEFAULT_BURN_IN_STEPS 100
#define DEFAULT_SAMP_INTERVAL 1
#define DEFAULT_GIBBS_STEPS 3
#define DEFAULT_NUM_TRIALS 100
#define DEFAULT_MWSAT_TIMEOUT 0
#define DEFAULT_MCSAT_THREAD_COUNT 0

#define DEFAULT_WEIGHTLEARN_LBFGS_MODE 1
#define DEFAULT_WEIGHTLEARN_MAX_ITER 1000
#define DEFAULT_WEIGHTLEARN_MIN_ERROR 0.001
#define DEFAULT_WEIGHTLEARN_RATE 0.1
#define DEFAULT_WEIGHTLEARN_REPORTING 100


typedef struct mcsat_params_s {
/*
 * Encapsulation: We create a struct to hold these things as a first
 * step toward threadsafe multi-mcsat...
 */
  int32_t max_samples;
  double sa_probability;
  double sa_temperature;
  double rvar_probability;
  int32_t max_flips;
  int32_t max_extra_flips;
  int32_t mcsat_timeout;
  int32_t burn_in_steps;
  int32_t samp_interval;
  int32_t gibbs_steps;
  int32_t num_trials;
  int32_t mwsat_timeout;
  int32_t mcsat_thread_count;

  int32_t weightlearn_lbfgs_mode;
  int32_t weightlearn_max_iter;
  double weightlearn_min_error;
  double weightlearn_rate;
  int32_t weightlearn_reporting;
  
  bool strict_consts;
  bool lazy;
  char *dump_samples_path;

  bool print_exp_p;

} mcsat_params_t;


#endif /* __TABLES_H */
