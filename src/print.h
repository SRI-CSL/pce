#ifndef __PRINT_H
#define __PRINT_H 1

/*
 * - A verbosity_level controls the amount of output produced
 *   it's set to 1 by default. Can be changed via set_verbosity_level
 * - cprintf is a replacement for printf:
 *   cprintf(level, <format>, ...) does nothing if level > verbosity_level
 *   otherwise, it's the same as printf(<format>, ...)
 */
extern void set_verbosity_level(int32_t level);
extern int32_t get_verbosity_level();
extern void cprintf(int32_t level, const char *fmt, ...);

extern void dump_sort_table (samp_table_t *table);
extern void dump_pred_table (samp_table_t *table);
extern void dump_atom_table (samp_table_t *table);
extern void dump_clause_table (samp_table_t *table);
extern void dump_rule_table (samp_table_t *samp_table);

#endif /* __PRINT_H */     
