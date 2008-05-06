/* rpcalc.y  MR 20/09/98
   Input file to 'bison' (or 'yacc').  Specification for the very simple
   reverse-polish caluculator, exactly from the end of gnu Linux file 
   'bison.info-1' and the beginning of file 'bison.info-2'.

   How to generate the parser program:
       1) Run bison on this file:     bison rpcalc.y
       2) Compile the bison output:   gcc rpcalc.tab.c -o rpcalc -lm
*/

%{
#define YYSTYPE double
#include <math.h>
%}

%token NUM
%start input


%%

input       : /* empty */
            | input line
;

line        : '\n'      
            | expr '\n' { printf( "%g\n", $1 ); }
;

expr        : NUM             { $$ = $1; }
            | expr expr '+'   { $$ = $1 + $2; }
            | expr expr '*'   { $$ = $1 * $2; }
;



%%

#include <stdio.h>
#include <ctype.h>

main()
	{
	yyparse();
	}

yyerror( char * str )
	{
	printf( "rpcalc: %s\n", str );
	}

int yylex( void )
	{
	int ic;

	while ( ic = getchar(), ic == ' ' || ic == '\t' ) { ; }
	if ( ic == EOF ) 
		{ 
		return 0; 
		}
	else if ( ic == '.' || isdigit( ic ) )
		{ 
		ungetc( ic, stdin );
		scanf( "%lf", &yylval );
		return NUM;
		}
	else
		{
		return ic;
		}
	}
