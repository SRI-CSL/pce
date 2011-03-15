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

/* Make sure only loaded once... */
#ifndef _LIBDICO_H_INCLUDED
#define _LIBDICO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define INIT_CAPACITY 512

  typedef struct dictionary {
    int size;
    int capacity;
    int  (*compar)(void *, void *);    /* user fn for comparing keys & values */
    void (*keyfree)(void *);	      /* user fn for freeing keeys */
    void **key;
    void **value;
  } DICTIONARY;

  /*
   * prototypes
   */

  extern DICTIONARY *dict_new(int (*_compar)(void *, void *), void (*_keyfree)(void *));
  extern int 	   dict_expand_capacity(DICTIONARY *dict);
  extern void 	  *dict_get(DICTIONARY *d, void *key);
  extern int dict_get_nth(DICTIONARY *d, void **key, void **value, int n);
  extern int 	   dict_index_for_key(DICTIONARY *d, void *key);
  extern void 	  *dict_put(DICTIONARY *d, void *key, void *value);
  extern void	  *dict_put_nonunique(DICTIONARY *d, void *key, void *value);
  extern void	  *dict_remove(DICTIONARY *d, void *key);
  extern void	  *dict_remove_specific(DICTIONARY *d, void *key, void *value);
  extern void	 **dict_remove_all(DICTIONARY *d, void *key, int *num_removed);
  extern void	   dict_free(DICTIONARY *d);

#ifdef __cplusplus
}
#endif

#endif
