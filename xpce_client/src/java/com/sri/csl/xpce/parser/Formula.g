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
     f2=formula {if (op.getType()==AND) f=new AndFormula(f1, f2);
     		 else if (op.getType()==OR) f=new OrFormula(f1, f2);
     		 else if (op.getType()==IMPLIES) f=new ImpliesFormula(f1, f2);
     		 else if (op.getType()==IFF) f=new IffFormula(f1, f2);
     		 else { System.out.println("Bad operator: " + op); f = null; }
     		 }
     RPAR);

atom returns [Atom f]
    @init {
    String functor = ""; 
    ArrayList<Term> terms = new ArrayList<Term>(); }:
    FUNCTOR {functor = $FUNCTOR.text; }
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
    CONST {c = new Constant($CONST.text);};
    
/*------------------------------------------------------------------
 * LEXER RULES
 *------------------------------------------------------------------*/

DIGIT	 :   '0'..'9';
ALPHA	 :   'a'..'z'|'A'..'Z';
VAR	 :   '$' ALPHA (ALPHA|DIGIT|'_'|'.')*;
FUNCTOR  :   ALPHA (ALPHA|DIGIT|'_'|'.')*;
CONST    :   QUOTE (DIGIT|ALPHA|'_'|'.'|':'|'/'|'\\'|' ')+  QUOTE;
NEWLINE  :   '\r'? '\n' ;
WS	 :   (' '|'\t')+ {skip();} ;


