/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     ALL = 258,
     BEST = 259,
     PROBABILITIES = 260,
     PREDICATE = 261,
     DIRECT = 262,
     INDIRECT = 263,
     CLAUSE = 264,
     RULE = 265,
     SORT = 266,
     SUBSORT = 267,
     CONSTD = 268,
     VAR = 269,
     ATOMD = 270,
     ASSERT = 271,
     ADD = 272,
     ASK = 273,
     ADD_CLAUSE = 274,
     MCSAT = 275,
     MCSAT_PARAMS = 276,
     RESET = 277,
     RETRACT = 278,
     DUMPTABLE = 279,
     SUMMARY = 280,
     LOAD = 281,
     VERBOSITY = 282,
     HELP = 283,
     QUIT = 284,
     IFF = 285,
     IMPLIES = 286,
     OR = 287,
     XOR = 288,
     AND = 289,
     NOT = 290,
     EQ = 291,
     NEQ = 292,
     LE = 293,
     LT = 294,
     GE = 295,
     GT = 296,
     DDOT = 297,
     INTEGER = 298,
     PLUS = 299,
     MINUS = 300,
     TIMES = 301,
     DIV = 302,
     REM = 303,
     NAME = 304,
     NUM = 305,
     STRING = 306
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1685 of yacc.c  */
#line 571 "yacc.y"

  bool bval;
  char *str;
  char **strs;
  input_sortdef_t *sortdef;
  input_fmla_t *fmla;
  input_formula_t *formula;
  input_clause_t *clause;
  input_literal_t *lit;
  input_literal_t **lits;
  input_atom_t *atom;
  int32_t ival;



/* Line 1685 of yacc.c  */
#line 118 "yacc.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;

