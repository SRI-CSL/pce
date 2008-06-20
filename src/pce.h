
#ifndef __PCE_H
#define __PCE_H

#define INIT_NAMES_BUFFER_SIZE 8
#define INIT_RULES_BUFFER_SIZE 8
#define INIT_PCE_QUEUE_SIZE 16

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

#endif
