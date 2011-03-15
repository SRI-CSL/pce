/*
 * Copyright (C) 2006  SRI International
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SRI International: 333 Ravenswood Ave, Menlo Park, CA 94025
 */

#include "libicl.h"
#include "dictionary.h"

/* Make sure only loaded once... */
#ifndef _LIBUTILS_H_INCLUDED
#define _LIBUTILS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_LEVEL 1

  EXTERN int oaa_ResolveVariable(char* inVarName, ICLTerm **resolvedVar);
  EXTERN void printDebug(int level, char *str, ...);
  EXTERN void printWarning(int level, char *str, ...);
  void print_dictionary(DICTIONARY *d);

  EXTERN char* get64BitFormatWrapped(char* pre, char* post);
  EXTERN char* get64BitFormat();

#include <stddef.h>           /* For size_t     */

  /*
   * A hash table consists of an array of these buckets.  Each bucket
   * holds a copy of the key, a pointer to the data associated with the
   * key, and a pointer to the next bucket that collided with this one,
   * if there was one.
   */

  typedef struct htbucket_struct {
    char *key;
    void *data;
    struct htbucket_struct *next;
  } htbucket
  ;

  /*
   * This is what you actually declare an instance of to create a table.
   * You then call 'construct_table' with the address of this structure,
   * and a guess at the size of the table.  Note that more nodes than this
   * can be inserted in the table, but performance degrades as this
   * happens.  Performance should still be quite adequate until 2 or 3
   * times as many nodes have been inserted as the table was created with.
   */

  typedef struct hthash_table_struct {
    size_t size;
    htbucket **table;
  } hthash_table;

  typedef struct htfreeNodeData_struct
  {
    void (*function)(void *);
    hthash_table *the_table;  
  } htfreeNodeData
  ;

  /*
   * This is used to construct the table.  If it doesn't succeed, it sets
   * the table's size to 0, and the pointer to the table to NULL.
   */

  hthash_table *htconstruct_table(hthash_table *table,size_t size);

  /*
   * Inserts a pointer to 'data' in the table, with a copy of 'key' as its
   * key.  Note that this makes a copy of the key, but NOT of the
   * associated data.
   */

  void *htinsert(char *key,void *data,hthash_table *table);

  /*
   * Returns a pointer to the data associated with a key.  If the key has
   * not been inserted in the table, returns NULL.
   */

  void *htlookup(char *key,hthash_table *table);

  /*
   * Deletes an entry from the table.  Returns a pointer to the data that
   * was associated with the key so the calling code can dispose of it
   * properly.
   */

  void *htdel(char *key,hthash_table *table);

  /*
   * Goes through a hash table and calls the function passed to it
   * for each node that has been inserted.  The function is passed
   * a pointer to the key, and a pointer to the data associated
   * with it.
   */

  void htenumerate(hthash_table *table,void (*func)(char *,void *, void *),void *otherData);

  /*
   * Frees a hash table.  For each node that was inserted in the table,
   * it calls the function whose address it was passed, with a pointer
   * to the data that was in the table.  The function is expected to
   * free the data.  Typical usage would be:
   * free_table(&table, free);
   * if the data placed in the table was dynamically allocated, or:
   * free_table(&table, NULL);
   * if not.  ( If the parameter passed is NULL, it knows not to call
   * any function with the data. )
   */

  void htfree_table(hthash_table *table, void (*func)(void *));

  void htprint(hthash_table *table);

#ifdef __cplusplus
}
#endif

#endif

