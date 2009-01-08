#ifndef __CNF_H
#define __CNF_H 1

extern void add_cnf(input_formula_t *formula, double weight);

extern void ask_cnf(input_formula_t *formula, int32_t num_samples, double threshold);

#endif /* __CNF_H */     
