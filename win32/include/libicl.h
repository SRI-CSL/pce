/****************************************************************************
 *   File    : libicl.h
 *   Author  : Adam Cheyer
 *   Updated : 5/21/97
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
#ifndef _ICLLIB_H_INCLUDED
#define _ICLLIB_H_INCLUDED

#include <stdlib.h>
#include "glib.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IS_DLL
#define EXTERN __declspec(dllexport)
#else
#define EXTERN extern
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#ifdef NORMAL_GC
#undef CHECK_LEAKS
#include <gc/leak_detector.h>
#undef strdup
  EXTERN char* gc_strdup(char*s);
#define strdup(s) gc_strdup(s)
#else
#define CHECK_LEAKS()
#endif

  enum ICLType
    {icl_no_type, icl_int_type, icl_float_type, icl_str_type, icl_struct_type,
     icl_list_type, icl_group_type, icl_var_type, icl_callback_type, icl_dataq_type};

  struct iclTerm;
  struct iclListType;
  struct iclGroupType;
  struct iclStructType;
  struct dyn_array;
  typedef struct iclTerm ICLTerm;
  typedef struct iclListType ICLListType;
  typedef struct iclGroupType ICLGroupType;
  typedef struct iclStructType ICLStructType;

  /* Structure construction & testing routines */
  EXTERN ICLTerm *	icl_NewTermFromString(char* s);
  EXTERN ICLTerm *	icl_NewTermFromData(char* data, size_t len);
  EXTERN char *		icl_NewStringFromTerm(ICLTerm const*t);
  EXTERN char *           icl_NewStringStructFromTerm(ICLTerm *t);
  /**
   * Get an unquoted string from an IclStr.  The newly created string
   * should be freed with icl_stFree(...).
   *
   * @return the new string or NULL if t is not an atom 
   */
  EXTERN char*      icl_UnquotedStringFromStr(ICLTerm* t);
  /**
   * Get a string from an IclStr, quoting it even if the quotes are not
   * necessary.  The newly created string should be freed with
   * icl_stFree(...)
   *
   * @return the new string or NULL if t is not an atom 
   */
  EXTERN char*      icl_ForcedQuotedStringFromStr(ICLTerm* t);
  /**
   * Get a minimally quoted string from an IclStr.  The newly created string
   * should be freed with icl_stFree(...).  This is equivalent to calling
   * icl_NewStringFromTerm(t), but is named consistently with the other
   * quoted string functions.
   *
   * @return the new string or NULL if t is not an atom 
   */
  EXTERN char*      icl_MinimallyQuotedStringFromStr(ICLTerm* t);
  EXTERN int		icl_WriteTerm(ICLTerm *t);
  EXTERN ICLTerm *	icl_CopyTerm(ICLTerm const*t);
  EXTERN ICLListType *    icl_CopyListType(ICLListType *list);
  EXTERN ICLTerm * 	icl_NewInt(gint64 i);
  EXTERN ICLTerm * 	icl_NewFloat(double f);
  EXTERN ICLTerm * 	icl_NewStr(char const* s);
  EXTERN ICLTerm * 	icl_NewVar(char *name);
  EXTERN ICLTerm * 	icl_NewStruct(char const* functor, int arity, ...);
  EXTERN ICLTerm * 	icl_NewStructFromList(char const* functor, ICLTerm *args);
  EXTERN ICLListType * 	icl_NewCons(ICLTerm *elt, ICLListType *tail);
  EXTERN ICLTerm * 	icl_NewList(ICLListType *list);
  EXTERN ICLTerm * 	icl_NewGroup(char sChar, char *sep, ICLListType *list);
  EXTERN ICLTerm *      icl_NewDataQ(void const* data, size_t dataLen);
  EXTERN void		icl_FreeTerm(ICLTerm *elt);
  EXTERN void		icl_FreeTermSingle(ICLTerm *elt);
  EXTERN int 		icl_IsList(ICLTerm const*elt);
  EXTERN int 		icl_IsGroup(ICLTerm const*elt);
  EXTERN int 		icl_IsStruct(ICLTerm const*elt);
  EXTERN int 		icl_IsStr(ICLTerm const*elt);
  EXTERN int 		icl_IsVar(ICLTerm const*elt);
  EXTERN int 		icl_IsInt(ICLTerm const*elt);
  EXTERN int 		icl_IsFloat(ICLTerm const*elt);
  EXTERN int            icl_IsDataQ(ICLTerm const*elt);
  EXTERN int 		icl_IsValid(ICLTerm const*elt);
  EXTERN int 		icl_IsGround(ICLTerm const*elt);
  EXTERN void*          icl_DataQ(ICLTerm const* elt);
  EXTERN size_t         icl_DataQLen(ICLTerm const* elt);
  EXTERN size_t         icl_Len(ICLTerm const* elt);
  EXTERN gint64		icl_Int(ICLTerm const*elt);
  EXTERN double		icl_Float(ICLTerm const*elt);
  EXTERN char *		icl_Str(ICLTerm const*elt);
  EXTERN char * 		icl_Functor(ICLTerm const*elt);
  EXTERN ICLListType *	icl_Arguments(ICLTerm const*elt);
  EXTERN int 		icl_GetGroupChars(ICLTerm const*group, char *startC,
                                          char **sep);
  EXTERN int              icl_Arity(ICLTerm const* inTerm);
  EXTERN int            icl_ReplaceElement(ICLTerm* term, int index, ICLTerm* replacement, int freeReplaced);
  EXTERN int            icl_ReplaceUnifying(ICLTerm* term, ICLTerm const* selector, ICLTerm const* replacement, int freeReplaced);

  /* List manipulations */
  EXTERN ICLListType *	icl_List(ICLTerm const*elt);
  EXTERN ICLListType *  icl_ListNext(ICLListType const* t);
  EXTERN ICLTerm *  icl_ListElt(ICLListType const* t);
  EXTERN ICLListType *	icl_ListCopy(ICLTerm const*elt);
  EXTERN ICLTerm *	icl_NthTerm(ICLTerm const*elt, int n);
  EXTERN int	        icl_NthTermAsInt(ICLTerm const*elt, int n, int *Value);
  EXTERN int 		icl_NumTerms(ICLTerm const*elt);
  EXTERN int 		icl_ListLen(ICLTerm const*elt);
  EXTERN int              icl_AddToList(ICLTerm *list, ICLTerm *elt, int atEnd);
  EXTERN int              icl_ClearList(ICLTerm *list);
  EXTERN int              icl_SortList(
   ICLTerm *list,
   int (*user_function)(ICLTerm *Elt1, ICLTerm *Elt2));
  EXTERN int  		icl_Append(ICLTerm *list1, ICLTerm *list2);
  EXTERN int  		icl_AppendCopy(ICLTerm *list1, ICLTerm const*list2);
  EXTERN int              icl_Union(ICLTerm *list1, ICLTerm *list2,
                                    ICLTerm **dest);
  EXTERN int              icl_ListHasMoreElements(ICLListType const*l);
  EXTERN ICLListType*            icl_ListNextElement(ICLListType const*l);
  EXTERN ICLTerm*                icl_ListElement(ICLListType const*list);
  EXTERN int                     icl_ListDelete(ICLTerm *list,
                                                ICLTerm *elem,
                                                ICLTerm **residue);
  EXTERN ICLTerm*            icl_GenerateSimpleUnifyingTerm(ICLTerm const*term);

  EXTERN int		icl_Unify(ICLTerm const*t1, ICLTerm const*t2, ICLTerm **answer);
  EXTERN int		icl_ParamValue(char *func, ICLTerm *match,
				       ICLTerm *params, ICLTerm **answer);
  EXTERN gint64 	        icl_ParamValueAsInt(char *func, ICLTerm *params,
                                            gint64 *Value);

  EXTERN int 	       icl_Member(ICLTerm const *elt, ICLTerm const *list, ICLTerm **res);
#if 0
  EXTERN ICLTerm * 	icl_ReuseMem(ICLTerm *elt);
#endif

  EXTERN char * icl_stFixQuotes(char *s);
  /**
   * return TRUE if the given string is properly quoted
   */
  EXTERN int icl_stIsProperlyQuoted(char* s);

  /* List management utility functions */
  EXTERN int		icl_list_has_more_elements(ICLListType *l);
  EXTERN ICLListType     *icl_list_next_element(ICLListType *l);
  EXTERN ICLTerm 	       *icl_list_element(ICLListType *list);
  EXTERN int		icl_list_delete(ICLTerm *list, ICLTerm *elem,
					ICLTerm **residue);
  EXTERN int		icl_append_to_list(ICLTerm *list1, ICLTerm *list2,
					   ICLTerm **list3);

  /* Convenience functions for representing often-used terms */
  EXTERN ICLTerm *	icl_True();
  EXTERN ICLTerm *	icl_False();
  EXTERN ICLTerm *	icl_Empty();
  EXTERN ICLTerm *	icl_Var();

  /****************************************************************************
   * Macros
   ****************************************************************************/
#define ICL_TRUE icl_True()
#define ICL_FALSE icl_False()
#define ICL_EMPTY icl_Empty()
#define ICL_VAR   icl_Var()


  EXTERN void icl_stFree(void *p);

  /*#define icl_Free(A) if (A) { void* vp = current_text_addr();icl_FreeTerm(A,n,vp); A = 0;}*/
  /*
    #ifdef LINUX
    #include <asm/processor.h>
    #define icl_Free(A,n) if (A) { void* vp = current_text_addr();icl_FreeTerm(A,n,vp); A = 0; }
    #else
    #define icl_Free(A,n) if (A) { icl_FreeTerm(A,n,0); A = 0; }
    #endif
  */
#define icl_Free(A) if (A) {icl_FreeTerm(A); A = 0;}

  EXTERN int icl_match_terms(ICLTerm *t1, ICLTerm *t2, struct dyn_array *vars);
  /*EXTERN ICLTerm *icl_copy_term(ICLTerm *t, struct dyn_array *vars);*/
  EXTERN ICLTerm *icl_copy_term_nonrec(ICLTerm const*t, struct dyn_array *vars);
#define icl_copy_term(T,V) icl_copy_term_nonrec(T,V)
  EXTERN void icl_init_dyn_array(struct dyn_array *da);
  /* static void icl_deref(ICLTerm **var, struct dyn_array var_bindings); */

#ifdef __cplusplus
}
#endif

#endif

