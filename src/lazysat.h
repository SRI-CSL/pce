#ifndef __LAZYSAT_H
#define __LAZYSAT_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "array_hash_map.h"
#include "symbol_tables.h"
#include "integer_stack.h"
#include "utils.h"

bool builtin_inst_p(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table);
bool literal_falsifiable(rule_literal_t *lit, substit_entry_t *substs, samp_table_t *table);
bool check_clause_falsifiable(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table);

bool check_clause_duplicate(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table);

int32_t substit_rule(samp_rule_t *rule, substit_entry_t *substs, samp_table_t *table);

int32_t push_alive_clause(samp_clause_t *clause, samp_table_t *table);
void push_newly_activated_clause(int32_t clsidx, samp_table_t *table);

int32_t pred_default_value(pred_entry_t *pred);
int32_t rule_atom_default_value(rule_atom_t *rule_atom, pred_table_t *pred_table);

void fixed_const_rule_instances(samp_rule_t *rule, samp_table_t *table,
		int32_t atom_index);

void smart_rule_instances_rec(int32_t order, int32_t *ordered_lits, samp_rule_t *rule,
		samp_table_t *table, int32_t atom_index);

void activate_rules(int32_t atom_index, samp_table_t *table);

int32_t add_and_activate_atom(samp_table_t *table, samp_atom_t *atom);
int32_t activate_atom(samp_table_t *table, int32_t atom_index);

#endif /* __LAZYSAT_H */
