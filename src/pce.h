
#ifndef __PCE_H
#define __PCE_H

#include <libicl.h>

#define INIT_NAMES_BUFFER_SIZE 8
#define INIT_RULES_BUFFER_SIZE 8
#define INIT_PCE_QUEUE_SIZE 16
#define INIT_SUBSCRIPTIONS_SIZE 8
#define INIT_QM_BUFFER_SIZE 8
#define INIT_PCE_MODEL_SIZE 8

typedef struct pce_model_s {
  uint32_t size;
  uint32_t capacity;
  int32_t *atom;
} pce_model_t;

typedef struct names_buffer_s {
  uint32_t size;
  uint32_t capacity;
  char **name;
} names_buffer_t;

typedef struct rule_vars_buffer_s {
  uint32_t size;
  uint32_t capacity;
  var_entry_t **vars;
} rule_vars_buffer_t;

typedef struct rules_buffer_s {
  uint32_t size;
  uint32_t capacity;
  samp_rule_t **rules;
} rules_buffer_t;

typedef struct subscription_s {
  int32_t lid; // Learner ID - index into learner names
  int32_t query_index;
  // who_t who; // Not used - one of 'all' or 'notme'
  // condition_t condition; //Not used - one of 'in_to_out' or 'out_to_in'
  char *id; // normally the subscription table index converted to a string
  ICLTerm *formula;
} subscription_t;

typedef struct subscription_buffer_s {
  uint32_t size;
  uint32_t capacity;
  subscription_t **subscription;
} subscription_buffer_t;

typedef struct qm_entry_s {
  // query caches the query - NULL means the predicate is unknown to QM
  ICLTerm *query;
} qm_entry_t;

typedef struct qm_buffer_s {
  uint32_t size;
  uint32_t capacity;
  // One per (direct) predicate
  qm_entry_t **entry;
} qm_buffer_t;
#endif
