#ifndef __LAZYSAMPLESAT_H
#define __LAZYSAMPLESAT_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "array_hash_map.h"
#include "symbol_tables.h"
#include "integer_stack.h"
#include "utils.h"

extern void all_lazy_query_instances(samp_query_t *query, samp_table_t *table);

extern void lazy_mc_sat(samp_table_t *table, double sa_probability,
			double samp_temperature, double rvar_probability,
			uint32_t max_flips, uint32_t max_extra_flips,
			uint32_t max_samples);

#endif /* __LAZYSAMPLESAT_H */     
