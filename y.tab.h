/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PREDICATE = 258,
     DIRECT = 259,
     INDIRECT = 260,
     SORT = 261,
     CONST = 262,
     VAR = 263,
     ASSERT = 264,
     ADD = 265,
     ASK = 266,
     DUMPTABLES = 267,
     TEST = 268,
     HELP = 269,
     QUIT = 270,
     NAME = 271,
     NUM = 272
   };
#endif
/* Tokens.  */
#define PREDICATE 258
#define DIRECT 259
#define INDIRECT 260
#define SORT 261
#define CONST 262
#define VAR 263
#define ASSERT 264
#define ADD 265
#define ASK 266
#define DUMPTABLES 267
#define TEST 268
#define HELP 269
#define QUIT 270
#define NAME 271
#define NUM 272




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 175 "yacc.y"
typedef union YYSTYPE {
  bool bval;
  char *str;
  char **strs;
  input_clause_t *clause;
  input_literal_t *lit;
  input_atom_t *atom;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 81 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



