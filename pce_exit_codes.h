/*
 * On non-recoverable errors, Pce calls exit with a non-zero status.
 * The status values are defined here for different error conditions
 * (incomplete).
 *
 * TODO: check portability, check conventions if any.
 */

#ifndef __PCE_EXIT_CODES_H
#define __PCE_EXIT_CODES_H

#include <stdlib.h>

#define PCE_EXIT_OUT_OF_MEMORY   16
#define PCE_EXIT_SYNTAX_ERROR    17
#define PCE_EXIT_FILE_NOT_FOUND  18
#define PCE_EXIT_USAGE           19
#define PCE_EXIT_ERROR           20

#define PCE_EXIT_SUCCESS         EXIT_SUCCESS

#endif /* __PCE_EXIT_CODES_H */
