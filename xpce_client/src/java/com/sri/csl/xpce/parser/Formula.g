grammar Formula;

options {
    language=Java;
    backtrack=true;
    memoize=true;
}

tokens {
    AND = 'and';
    OR = 'or';
    IMPLIES = '=>';
    IFF = '<=>';
    NOT = '-';
    QUOTE = '\"';
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
    a1=atom { f = a1; }
    |   
    n=notFormula { f = n; }
    |
    d=andFormula { f = d; }
    |
    o=orFormula { f = o; }
    |
    m=impliesFormula { f = m; }
    |
    i=iffFormula { f = i; };


andFormula returns [AndFormula af]:
    LPAR
    f1=formula
    AND 
    f2=formula
    RPAR
    { af = new AndFormula(f1, f2); };

orFormula returns [OrFormula af]:
    LPAR
    f1=formula
    OR
    f2=formula
    RPAR
    { af = new OrFormula(f1, f2); };

impliesFormula returns [ImpliesFormula af]:
    LPAR
    f1=formula
    IMPLIES 
    f2=formula
    RPAR
    { af = new ImpliesFormula(f1, f2); };

iffFormula returns [IffFormula af]:
    LPAR
    f1=formula
    IFF 
    f2=formula
    RPAR
    { af = new IffFormula(f1, f2); };


notFormula returns [NotFormula nf]:
    NOT
    f=formula { nf = new NotFormula(f); };

atom returns [Atom f]
    @init {
    String functor = ""; 
    ArrayList<Term> terms = new ArrayList<Term>(); }:
    ID1 {functor = $ID1.text; }
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
    '$'
    ID1 {v = new Variable($ID1.text);};

constant returns [Constant c]:
    ID1 {c = new Constant($ID1.text);}
    |
    CONST1 {c = new Constant($CONST1.text);}
    |
    CONST2 {c = new Constant($CONST2.text);}
    ;
    
/*------------------------------------------------------------------
 * LEXER RULES
 *------------------------------------------------------------------*/

DIGIT	 :   '0'..'9';
ALPHA	 :   'a'..'z'|'A'..'Z';
ID1   	 :   ALPHA (DIGIT|ALPHA|'_')* ;
CONST1   :   DIGIT (DIGIT|ALPHA|'_'|'.'|':'|'/'|'\\')*;
CONST2   :   QUOTE (DIGIT|ALPHA|'_'|'.'|':'|'/'|'\\'|' ')+  QUOTE;
NEWLINE :   '\r'? '\n' ;
WS  :   (' '|'\t')+ {skip();} ;


