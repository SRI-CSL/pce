#ifndef __CNF_H
#define __CNF_H 1

#include "vectors.h"
#include "input.h"

extern pvector_t ask_buffer;

extern void add_cnf(char **frozen, input_formula_t *formula, double weight, 
		char *source, bool add_weights);

extern void ask_cnf(input_formula_t *formula, double threshold, int32_t numresults);

extern void add_cnf_query(input_formula_t *formula);

#endif /* __CNF_H */     
