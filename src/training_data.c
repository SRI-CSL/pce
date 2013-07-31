/*
 * training_data.c
 *
 *  Created on: Aug 17, 2011
 *      Author: papai
 */

#include "training_data.h"
#include <stdio.h>
#include <ctype.h>
#include "string.h"
#include "memalloc.h"
#include "inttypes.h"

static training_data_t *training_data;
static int32_t num_samples;

void parse_line(char* line, training_data_t *training_data,
		int32_t current_world, samp_table_t* table) {
	char pname[50];
	char member[50];
	char *pos1, *pos2;
	bool positive;
	int32_t i, j;
	atom_table_t atom_table = table->atom_table;
	pred_table_t pred_table = table->pred_table;
	const_table_t const_table = table->const_table;
	int32_t arity, _arity, buffer_cnt;

	positive = (line[0] != NEGATION_SIGN);
	pos1 = strchr(line, '(');
	pos2 = strchr(line, ')');
	*pos2 = '\0';
	strcpy(member, pos1 + 1);
	*pos1 = '\0';
	if (positive) {
		strcpy(pname, line);
	} else {
		strcpy(pname, line + 1);
	}
	fflush(stdout);

	arity = 1;
	for (i = 0; i < strlen(member); ++i) {
		if (member[i] == ',')
			arity++;
	}

	// FIXME: this version is super inefficient, has to be rewritten with hash table lookups
	// find corresponding atom from the atom table
	for (i = 0; i < atom_table.num_vars; ++i) {
		int pred = atom_table.atom[i]->pred;
		char* _pname = pred_name(pred, &pred_table);
		//		printf("PNAME: %s\n", _pname);

		if (strcmp(_pname, pname) != 0)
			continue;

		_arity = pred_arity(pred, &pred_table);
		if (_arity != arity)
			continue;

		buffer_cnt = 0;
		for (j = 0; j < arity; ++j) {
			char* cname = const_name(atom_table.atom[i]->args[j], &const_table);
			//			printf("CNAME: %s\n", cname);

			if (j > 0) {
				buffer_cnt++;
			}
			if (strncmp(member + buffer_cnt, cname, strlen(cname) != 0)) {
				break;
			}
			buffer_cnt += strlen(cname);
		}
		if (buffer_cnt == strlen(member)) {
			// atom is found
			//			printf("FOUND\n\n");
			training_data->evidence_atoms[current_world][i].atom
					= atom_table.atom[i];
			training_data->evidence_atoms[current_world][i].truth_value
					= positive ? ev_true : ev_false;
			return;
		}
		//		else {
		//			printf("NOT FOUND\n\n");
		//		}
	}
}

extern training_data_t* parse_data_file(const char *path, samp_table_t* table) {
	FILE *fp = fopen(path, "r");
	int wspace_cnt;
	char line[100];
	int i;
	atom_table_t atom_table = table->atom_table;
	int32_t current_world = 0;

	if (fp == NULL) {
		fprintf(stderr, "Could not open %s", path);
	}

	training_data = (training_data_t*) safe_malloc(sizeof(training_data_t));
	training_data->num_evidence_atoms = atom_table.num_vars;

	// allocate memory for each world in the training data
	fscanf(fp, "%d\n", &training_data->num_data_sets);

	training_data->evidence_atoms = (evidence_atom_t**) safe_malloc(
			training_data->num_data_sets * sizeof(evidence_atom_t*));
	for (i = 0; i < training_data->num_data_sets; ++i) {
		training_data->evidence_atoms[i] = (evidence_atom_t*) safe_malloc(
				atom_table.num_vars * sizeof(evidence_atom_t));
	}

	while (!feof(fp)) {
		fscanf(fp, "%s\n", line);
		// do a left trim
		wspace_cnt = 0;
		while (wspace_cnt < strlen(line) && isspace(line[wspace_cnt]))
			wspace_cnt++;
		if (wspace_cnt == strlen(line)) // skip empty lines
			continue;
		if (strcmp(line + wspace_cnt, NEXT_WORLD) == 0) {
			current_world++;
		} else {
			parse_line(line + wspace_cnt, training_data, current_world, table);
		}
	}

	fclose(fp);
	assert(current_world + 1 == training_data->num_data_sets);
	return training_data;
}

extern void free_training_data(training_data_t *training_data) {
	int i;

	for (i = 0; i < training_data->num_data_sets; ++i) {
		safe_free(training_data->evidence_atoms[i]);
	}
	safe_free(training_data->evidence_atoms);
	safe_free(training_data);
}

extern void init_samples_output(char *path, int32_t n_samples) {
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		fprintf(stderr, "Could not open %s", path);
		exit(1);
	}
	num_samples = n_samples;

	fprintf(fp, "%d\n", n_samples);
	fclose(fp);
}

extern void append_assignment_to_file(char *path, samp_table_t *table) {
	pred_table_t *pred_table = &table->pred_table;
	const_table_t *const_table = &table->const_table;
	sort_table_t *sort_table = &table->sort_table;
	atom_table_t *atom_table = &table->atom_table;
	int32_t i, j, num_vars;
	sort_entry_t *entry;
	int32_t *psig;
	samp_atom_t *atom;
	static int32_t current_sample = 0;

	FILE *fp = fopen(path, "a");
	if (fp == NULL) {
		fprintf(stderr, "Could not open %s", path);
		exit(1);
	}

	num_vars = atom_table->num_vars;
	for (i = 0; i < num_vars; i++) {
		if (assigned_false(atom_table->assignment[i])) {
			fprintf(fp, "~");
		}
		atom = atom_table->atom[i];
		psig = pred_signature(atom->pred, pred_table);
		fprintf(fp, "%s", pred_name(atom->pred, pred_table));
		for (j = 0; j < pred_arity(atom->pred, pred_table); j++) {
			j == 0 ? fprintf(fp, "(") : fprintf(fp, ",");
			entry = &sort_table->entries[psig[j]];
			if (entry->constants == NULL) {
				// It's an integer
				fprintf(fp, "%"PRId32"", atom->args[j]);
			} else {
				if (atom->args[j] < 0) {
					// A variable - print X followed by index
					fprintf(fp, "X%"PRId32"", -(atom->args[j] + 1));
				} else {
					fprintf(fp, "%s", const_name(atom->args[j], const_table));
				}
			}
		}
		fprintf(fp, ")\n");
	}
	current_sample++;
	if (current_sample < num_samples) {
		fprintf(fp, "\n>>\n");
	}

	fclose(fp);
}

