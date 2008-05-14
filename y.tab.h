/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

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
     ATOM = 264,
     ASSERT = 265,
     ADD = 266,
     ASK = 267,
     MCSAT = 268,
     RESET = 269,
     DUMPTABLES = 270,
     VERBOSITY = 271,
     TEST = 272,
     HELP = 273,
     QUIT = 274,
     NAME = 275,
     NUM = 276
   };
#endif
/* Tokens.  */
#define PREDICATE 258
#define DIRECT 259
#define INDIRECT 260
#define SORT 261
#define CONST 262
#define VAR 263
#define ATOM 264
#define ASSERT 265
#define ADD 266
#define ASK 267
#define MCSAT 268
#define RESET 269
#define DUMPTABLES 270
#define VERBOSITY 271
#define TEST 272
#define HELP 273
#define QUIT 274
#define NAME 275
#define NUM 276




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 317 "yacc.y"
{
  bool bval;
  char *str;
  char **strs;
  input_clause_t *clause;
  input_literal_t *lit;
  input_atom_t *atom;
}
/* Line 1529 of yacc.c.  */
#line 100 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

