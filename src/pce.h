#include <ctype.h>
#include "memalloc.h"
#include "samplesat.h"
typedef struct input_preddecl_s {
  input_atom_t *atom;
  bool witness;
} input_preddecl_t;
typedef struct input_sortdecl_s {
  char *name;
} input_sortdecl_t;
typedef struct input_constdecl_s {
  char *name;
  char *sort;
} input_constdecl_t;
typedef struct input_vardecl_s {
  char *name;
  char *sort;
} input_vardecl_t;
typedef struct input_assertdecl_s {
  input_atom_t *atom;
} input_assertdecl_t;
typedef struct input_adddecl_s {
  input_clause_t *clause;
  double weight;
} input_adddecl_t;
typedef struct input_askdecl_s {
  input_clause_t *clause;
} input_askdecl_t;
typedef union input_decl_s {
  input_preddecl_t preddecl;
  input_sortdecl_t sortdecl;
  input_constdecl_t constdecl;
  input_vardecl_t vardecl;
  input_assertdecl_t assertdecl;
  input_adddecl_t adddecl;
  input_askdecl_t  askdecl;
} input_decl_t;

typedef struct input_command_s {
  int kind;
  input_decl_t *decl;
} input_command_t;
