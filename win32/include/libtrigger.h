/****************************************************************************
 *   File    : libtrigger.h
 *   Author  : Adam Cheyer
 *   Updated : 12/99
 *
 *****************************************************************************/
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
#ifndef _LIBTRIGGER_H_INCLUDED
#define _LIBTRIGGER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IS_DLL
#define EXTERN __declspec(dllexport)
#else
#define EXTERN extern
#endif


  EXTERN int oaa_CheckTriggers(char *type, ICLTerm *condition_var, char *op);
  EXTERN int oaa_AddTrigger(char *type, ICLTerm *condition, ICLTerm *action,
                            ICLTerm *initial_params, ICLTerm **out_params);
  EXTERN int oaa_RemoveTrigger(char *type, ICLTerm *condition,
                               ICLTerm *action, ICLTerm *initial_params, 
                               ICLTerm **out_params);

  /* DG : Temporarily put here, should be hidden */

  int oaa_add_trigger_local(char *type, ICLTerm *condition, ICLTerm *action,
                            ICLTerm *params);
  int oaa_remove_trigger_local(char *type, ICLTerm *condition, ICLTerm *action,
                               ICLTerm *params);

#ifdef __cplusplus
}
#endif

#endif
