#ifndef __PRINT_H
#define __PRINT_H 1

typedef struct string_buffer_s {
  uint32_t capacity;
  uint32_t size;
  char *string;
} string_buffer_t;

extern bool output_to_string;
extern string_buffer_t output_buffer;
extern string_buffer_t error_buffer;
extern string_buffer_t warning_buffer;

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
extern char * get_string_from_buffer(string_buffer_t *strbuf);

extern void dump_sort_table (samp_table_t *table);
extern void dump_pred_table (samp_table_t *table);
extern void dump_atom_table (samp_table_t *table);
extern void dump_clause_table (samp_table_t *table);
extern void dump_rule_table (samp_table_t *samp_table);

extern void summarize_sort_table (samp_table_t *table);
extern void summarize_pred_table (samp_table_t *table);
extern void summarize_atom_table (samp_table_t *table);
extern void summarize_clause_table (samp_table_t *table);
extern void summarize_rule_table (samp_table_t *samp_table);

extern void print_literal(samp_literal_t lit, samp_table_t *table);
extern void print_clause(samp_clause_t *clause, samp_table_t *table);
extern void print_query_instance(samp_query_instance_t *qinst, samp_table_t *table,
				 int32_t indent, bool include_prob);
extern void print_rule_clause (rule_literal_t **lit, var_entry_t **vars,
			       samp_table_t *table);
extern void print_assignment(samp_table_t *table);
extern void print_atom_now(samp_atom_t *atom, samp_table_t *table);
extern char *literal_string(samp_literal_t lit, samp_table_t *table);
extern double atom_probability(int32_t atom_index, samp_table_t *table);
extern double query_probability(samp_query_instance_t *qinst,
				samp_table_t *table);

#endif /* __PRINT_H */     
