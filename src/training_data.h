/*
 * training_data.h
 *
 *  Created on: Aug 17, 2011
 *      Author: papai
 */

#ifndef TRAINING_DATA_H_
#define TRAINING_DATA_H_ 1

#include "utils.h"		// has to be included before tables.h
#include "tables.h"

#define NEGATION_SIGN '~'
#define NEXT_WORLD ">>"			// to separate the worlds in the training data

typedef enum {
  ev_unknown = -1,
  ev_false = 0,
  ev_true = 1,
} evidence_truth_value_t;

typedef struct evidence_atom_s
{
	samp_atom_t *atom;
	evidence_truth_value_t truth_value;
} evidence_atom_t;

typedef struct training_data_s
{
	int32_t num_evidence_atoms;		// the number of atoms in each possible world
	int32_t num_data_sets;			// the number of elements (worlds) in the training data
	evidence_atom_t **evidence_atoms;           // the array of evidence atoms
} training_data_t;

void parse_line(char* line, training_data_t *training_data, int32_t current_world, samp_table_t* table);
extern training_data_t* parse_data_file(const char *path, samp_table_t* table);
extern void free_training_data(training_data_t *training_data);

extern void init_samples_output(char *path, int32_t num_samples);
extern void append_assignment_to_file(char *path, samp_table_t *table);

#endif /* TRAINING_DATA_H_ */
