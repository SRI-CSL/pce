#ifndef __CNF_H
#define __CNF_H 1
#include "vectors.h"

extern pvector_t ask_buffer;

extern void add_cnf(char **fixed, input_formula_t *formula, double weight, char *source, bool add_weights);

extern void ask_cnf(input_formula_t *formula, double threshold, int32_t numresults);

#endif /* __CNF_H */     
