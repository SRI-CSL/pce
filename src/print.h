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

extern void output(const char *fmt, ...);
extern void mcsat_err(const char *fmt, ...);
extern void mcsat_warn(const char *fmt, ...);

extern char *literal_string(samp_literal_t lit, samp_table_t *table);
extern char *atom_string(samp_atom_t *atom, samp_table_t *table);
extern char *var_string(int32_t var, samp_table_t *table);

extern void dump_sort_table (samp_table_t *table);
extern void dump_pred_table (samp_table_t *table);
extern void dump_atom_table (samp_table_t *table);
extern void dump_clause_table (samp_table_t *table);
extern void dump_rule_table (samp_table_t *samp_table);
extern void dump_query_instance_table (samp_table_t *samp_table);

extern void summarize_sort_table (samp_table_t *table);
extern void summarize_pred_table (samp_table_t *table);
extern void summarize_atom_table (samp_table_t *table);
extern void summarize_clause_table (samp_table_t *table);
extern void summarize_rule_table (samp_table_t *samp_table);

extern void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table);
extern void print_atoms(samp_table_t *samp_table);
extern void print_atom(samp_atom_t *atom, samp_table_t *table);
extern void print_clauses(samp_table_t *samp_table);
extern void print_clause_table(samp_table_t *table);
extern void print_rules(rule_table_t *rule_table);
extern void print_state(samp_table_t *table, uint32_t round);
extern void print_literal(samp_literal_t lit, samp_table_t *table);
extern void print_clause(samp_clause_t *clause, samp_table_t *table);
extern void print_query_instance(samp_query_instance_t *qinst, samp_table_t *table,
				 int32_t indent, bool include_prob);

extern void print_rule (samp_rule_t *rule, samp_table_t *table, int indent);
extern void print_rule_substit(samp_rule_t *rule, int32_t *substs, samp_table_t *table);
extern void print_rule_clause (rule_literal_t **lit, var_entry_t **vars,
			       samp_table_t *table);
extern void print_rule_atom (rule_atom_t *ratom, bool neg, var_entry_t **vars,
			     samp_table_t *table, int indent);
extern void print_assignment(samp_table_t *table);
extern void print_atom_now(samp_atom_t *atom, samp_table_t *table);

extern double atom_probability(int32_t atom_index, samp_table_t *table);
extern double query_probability(samp_query_instance_t *qinst,
				samp_table_t *table);

#endif /* __PRINT_H */     
