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

/* General output util functions */
extern void cprintf(int32_t level, const char *fmt, ...);
extern void output(const char *fmt, ...);
extern void mcsat_err(const char *fmt, ...);
extern void mcsat_warn(const char *fmt, ...);

extern char *samp_truth_value_string(samp_truth_value_t val);

/* TODO what is this? put it to some where else */
extern double query_probability(samp_query_instance_t *qinst,
				samp_table_t *table);

/* Print the all predicates */
extern void print_predicates(pred_table_t *pred_table, sort_table_t *sort_table);

/* Print an atom or all atoms */
extern void print_atom(samp_atom_t *atom, samp_table_t *table);
extern void print_atom_now(samp_atom_t *atom, samp_table_t *table);
extern void print_atoms(samp_table_t *samp_table);

/* Print clauses or rule instances */
extern void print_clause(samp_clause_t *clause, samp_table_t *table);
extern void print_rule_instance(rule_inst_t *rinst, samp_table_t *table);
extern void print_rule_instances(samp_table_t *table);

/* Print clause lists */
extern void print_clause_list(samp_clause_list_t *list, samp_table_t *table);
extern void print_watched_clause(samp_literal_t lit, samp_table_t *table);
extern void print_live_clauses(samp_table_t *table);

/* Print rule_table */
extern void print_rules(rule_table_t *rule_table);

/* Print atom_table and rule_inst_table */
extern void print_state(samp_table_t *table, uint32_t round);
extern void print_assignment(samp_table_t *table);

extern void print_rule (samp_rule_t *rule, samp_table_t *table, int indent);
extern void print_rule_substit(samp_rule_t *rule, int32_t *substs, samp_table_t *table);
extern void print_rule_atom (rule_atom_t *ratom, bool neg, var_entry_t **vars,
			     samp_table_t *table, int indent);

/* Print a query instance */
extern void print_query_instance(samp_query_instance_t *qinst, samp_table_t *table,
				 int32_t indent, bool include_prob);

/* Output struct objects as string */
extern char *literal_string(samp_literal_t lit, samp_table_t *table);
extern char *atom_string(samp_atom_t *atom, samp_table_t *table);
extern char *var_string(int32_t var, samp_table_t *table);

/* Dump the tables */
extern void dump_sort_table (samp_table_t *table);
extern void dump_pred_table (samp_table_t *table);
extern void dump_atom_table (samp_table_t *table);
extern void dump_rule_inst_table (samp_table_t *table);
extern void dump_rule_table (samp_table_t *samp_table);
extern void dump_query_instance_table (samp_table_t *samp_table);
extern void dump_all_tables(samp_table_t *samp_table);

/* Brief info of the tables */
extern void summarize_sort_table (samp_table_t *table);
extern void summarize_pred_table (samp_table_t *table);
extern void summarize_atom_table (samp_table_t *table);
extern void summarize_rule_inst_table (samp_table_t *table);
extern void summarize_rule_table (samp_table_t *samp_table);
extern void summarize_tables(samp_table_t *samp_table);

#endif /* __PRINT_H */     

