/****************************************************************************
 *   File    : liboaa.h
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
#ifndef _AGENTLIB_H_INCLUDED
#define _AGENTLIB_H_INCLUDED

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

#ifndef _WINDOWS
#include <unistd.h>
#else
#include "oaa-windows.h"
#endif

#include <libtrigger.h>
#include <libutils.h>

  /*****************************************************************************
   * Initialization and connection functions
   *****************************************************************************/

  EXTERN int  oaa_Register(char *ConnectionId, char *AgentName,
                           ICLTerm *Solvables);
  EXTERN void oaa_Ready(int ShouldPrint);


  /*****************************************************************************
   * Classifying and Manipulating ICL expressions
   *****************************************************************************/

  EXTERN int  icl_Builtin(ICLTerm *goal);
  EXTERN int  icl_BasicGoal(ICLTerm *goal);
  EXTERN int  icl_GetParamValue(ICLTerm *Param, ICLTerm *ParamList, ICLTerm **Result);
  EXTERN int  icl_GetPermValue(ICLTerm *Perm, ICLTerm *PermList, ICLTerm **Result);
  EXTERN int  icl_ConvertSolvables(int toStandard,
                                   ICLTerm *ShorthandSolvables, ICLTerm **StandardSolvables);


  /* **************************************************************************
   * Retrieving and managing events
   * *************************************************************************/

  EXTERN int oaa_Init(int argc, char* argv[]);
  EXTERN void oaa_MainLoop(int ShouldPrint);
  EXTERN void oaa_ProcessAllEvents();

  EXTERN void oaa_ProcessEvent(ICLTerm *Event, ICLTerm *Params);
  EXTERN void oaa_SetTimeout(double NSecs);
  EXTERN void oaa_GetEvent(ICLTerm **Event, ICLTerm **Params, int LowestPriority);
  EXTERN int oaa_ValidateEvent(ICLTerm *E, ICLTerm **OkEvent);
  EXTERN int oaa_Interpret(ICLTerm *goal, ICLTerm *params, ICLTerm **solutions);
  EXTERN int oaa_DelaySolution(ICLTerm *id);
  EXTERN int oaa_ReturnDelayedSolutions(ICLTerm *id, ICLTerm *solution_list);
  EXTERN int oaa_AddDelayedContextParams(ICLTerm *id, ICLTerm *params,
                                         ICLTerm **new_params);
  EXTERN int oaa_PostEvent(ICLTerm *contents, ICLTerm *params);
  EXTERN int oaa_Version(ICLTerm *agent_id, ICLTerm **language,
                         ICLTerm **version);
  EXTERN int oaa_CanSolve(ICLTerm *goal, ICLTerm **kslist);
  EXTERN int oaa_Ping(ICLTerm *agent_addr, double time_limit,
                      double *total_response_time);
  EXTERN int oaa_Declare(ICLTerm *solvable, ICLTerm *initial_common_perms,
                         ICLTerm *initial_common_params, ICLTerm *initial_params,
                         ICLTerm **declared_solvables);
  EXTERN int oaa_DeclareData(ICLTerm *solv, ICLTerm *params,
                             ICLTerm **declared_solvs);
  EXTERN int oaa_Undeclare(ICLTerm *solvable, ICLTerm *initial_params,
                           ICLTerm **undeclared_solvables);
  EXTERN int oaa_Redeclare(ICLTerm *initial_solvable,
                           ICLTerm *initial_new_solvable,
                           ICLTerm *initial_params);
  EXTERN int oaa_AddData(ICLTerm *clause, ICLTerm *in_params,
                         ICLTerm **out_params);

  EXTERN int oaa_RemoveData(ICLTerm *clause, ICLTerm *in_params,
                            ICLTerm **out_params);

  EXTERN int oaa_ReplaceData(ICLTerm *clause1, ICLTerm *clause2,
                             ICLTerm *in_params, ICLTerm **out_params);

  EXTERN int oaa_Id(ICLTerm **my_id);
  EXTERN int oaa_Name(ICLTerm **my_name);
  EXTERN int oaa_Address(char* connectionId, ICLTerm* Type, ICLTerm **myAddress);
  EXTERN int oaa_PrimaryAddress(ICLTerm** primaryAddress);
  EXTERN int oaa_PrimaryId(ICLTerm** primaryId);
  EXTERN int oaa_Solve(ICLTerm *goal, ICLTerm *initial_params,
                       ICLTerm **out_params, ICLTerm **solutions);
  EXTERN int oaa_InCache(ICLTerm *goal, ICLTerm **solutions);
  EXTERN int oaa_AddToCache(ICLTerm *goal, ICLTerm *solutions);
  EXTERN int oaa_ClearCache();
  EXTERN int oaa_RegisterCallback(char *callback_id,
                                  int (*callback_proc)(ICLTerm*, ICLTerm*, ICLTerm*));
  EXTERN int oaa_GetCallback(char* callback_id,
                             int (**callback_proc)(ICLTerm*, ICLTerm*, ICLTerm*));
  EXTERN int oaa_TraceMsg(char *format_string, ...);
  EXTERN char *oaa_name_string(void);


  EXTERN int memberchk(ICLTerm *Param, ICLTerm *ParamList);
  EXTERN int oaa_Connect(char *ConnectionId, ICLTerm *Address,
                         char *InitialAgentName, ICLTerm *Params);
  EXTERN int oaa_Disconnect(char* ConnectionId, ICLTerm *Params);
  EXTERN int oaa_SetupCommunication(char *InitialAgentName);
  EXTERN int oaa_LibraryVersion(ICLTerm** versionCopy);
  EXTERN int oaa_SupportsSequenceNumbers(char* connectionId);

  /**
   * @deprecated use oaa_LibraryVersion()
   */
  EXTERN char const* oaa_library_version_str;

  /****************************************************************************
   * Macros
   ****************************************************************************/

  /**
   * @deprecated use oaa_LibraryVersion()
   */
#define OAA_LIBRARY_VERSION oaa_library_version_str;

#ifndef STREQ
#define STREQ(str1, str2) (strcmp((str1), (str2)) == 0)
#endif

// Function which sleeps a specified number of milliseconds
#ifdef _WINDOWS
#define sleep_millis(n) Sleep(n)
#else
#define sleep_millis(n) usleep((n)*1000L)
#endif

#ifdef __cplusplus
}
#endif

#endif


