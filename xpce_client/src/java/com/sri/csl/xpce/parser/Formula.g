grammar Formula;

options {
    language=Java;
    //backtrack=true;
    //memoize=true;
}

tokens {
    AND = '&';
    OR = '|';
    IMPLIES = '=>';
    IFF = '<=>';
    NOT = '-';
    QUOTE = '\'';
    LPAR = '(';
    RPAR = ')';
    COMMA = ',';
}

@header {
package com.sri.csl.xpce.parser;

import com.sri.csl.xpce.object.Formula;
import com.sri.csl.xpce.object.Atom;
import com.sri.csl.xpce.object.NotFormula;
import com.sri.csl.xpce.object.AndFormula;
import com.sri.csl.xpce.object.OrFormula;
import com.sri.csl.xpce.object.ImpliesFormula;
import com.sri.csl.xpce.object.IffFormula;
import com.sri.csl.xpce.object.Term;
import com.sri.csl.xpce.object.Constant;
import com.sri.csl.xpce.object.Variable;
import java.util.ArrayList;
}

@members {
/** Map variable name to Integer object holding value */
//HashMap memory = new HashMap();
}

prog: formula+ ;

formula returns [Formula f]:
    at=atom { f = at; }
    |   
    (NOT
     nf=formula { f = new NotFormula(nf); })
    |
    (LPAR
     f1=formula
     op=(AND|OR|IMPLIES|IFF)
     f2=formula {switch (op.getType()) { 
     		   case AND:
     		   	f=new AndFormula(f1, f2); break;
     		   case OR:
     		   	f=new OrFormula(f1, f2); break;
     		   case IMPLIES:
     		   	f=new ImpliesFormula(f1, f2); break;
     		   case IFF:
     		   	f=new IffFormula(f1, f2); break;
     		   default:
     		   	System.out.println("Bad operator: " + op); f = null; break;
     		   }
     		}
     RPAR);

atom returns [Atom f]
    @init {
    String functor = ""; 
    ArrayList<Term> terms = new ArrayList<Term>(); }:
    FUNCONS {functor = $FUNCONS.text; }
        LPAR 
        t=term { terms.add(t); }
        (
            COMMA 
            t=term { terms.add(t); }
        )* 
        RPAR
        { f = new Atom(functor, terms); };
    

term returns [Term t]:
    v=variable {t = v;}
    |
    c=constant {t = c;};

variable returns [Variable v]:
    VAR {v = new Variable($VAR.text);};

constant returns [Constant c]:
    FUNCONS {c = new Constant($FUNCONS.text);}
    |
    CONST {c = new Constant($CONST.text);};
    
/*------------------------------------------------------------------
 * LEXER RULES
 *------------------------------------------------------------------*/

VAR	 :   '$' ('a'..'z'|'A'..'Z') ('a'..'z'|'A'..'Z'|'0'..'9'|'_'|'.')*;
FUNCONS  :   ('a'..'z'|'A'..'Z') ('a'..'z'|'A'..'Z'|'0'..'9'|'_'|'.')*;
CONST    :   QUOTE ('0'..'9'|'a'..'z'|'A'..'Z'|'_'|'.'|':'|'/'|'\\'|' ')+  QUOTE;
NEWLINE  :   '\r'? '\n' ;
WS	 :   (' '|'\t')+ {skip();} ;


