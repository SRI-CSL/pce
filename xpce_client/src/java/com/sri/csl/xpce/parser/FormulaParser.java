// $ANTLR 3.1.2 C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g 2009-06-29 16:28:49

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


import org.antlr.runtime.*;
import java.util.Stack;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
public class FormulaParser extends Parser {
    public static final String[] tokenNames = new String[] {
        "<invalid>", "<EOR>", "<DOWN>", "<UP>", "AND", "OR", "IMPLIES", "IFF", "NOT", "QUOTE", "LPAR", "RPAR", "COMMA", "ID1", "CONST1", "CONST2", "DIGIT", "ALPHA", "NEWLINE", "WS", "'$'"
    };
    public static final int IFF=7;
    public static final int IMPLIES=6;
    public static final int T__20=20;
    public static final int NOT=8;
    public static final int AND=4;
    public static final int EOF=-1;
    public static final int ALPHA=17;
    public static final int QUOTE=9;
    public static final int WS=19;
    public static final int LPAR=10;
    public static final int NEWLINE=18;
    public static final int COMMA=12;
    public static final int OR=5;
    public static final int RPAR=11;
    public static final int ID1=13;
    public static final int DIGIT=16;
    public static final int CONST2=15;
    public static final int CONST1=14;

    // delegates
    // delegators


        public FormulaParser(TokenStream input) {
            this(input, new RecognizerSharedState());
        }
        public FormulaParser(TokenStream input, RecognizerSharedState state) {
            super(input, state);
            this.state.ruleMemo = new HashMap[21+1];
             
             
        }
        

    public String[] getTokenNames() { return FormulaParser.tokenNames; }
    public String getGrammarFileName() { return "C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g"; }


    /** Map variable name to Integer object holding value */
    //HashMap memory = new HashMap();



    // $ANTLR start "prog"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:1: prog : ( formula )+ ;
    public final void prog() throws RecognitionException {
        int prog_StartIndex = input.index();
        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 1) ) { return ; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:5: ( ( formula )+ )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:7: ( formula )+
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:7: ( formula )+
            int cnt1=0;
            loop1:
            do {
                int alt1=2;
                int LA1_0 = input.LA(1);

                if ( (LA1_0==NOT||LA1_0==LPAR||LA1_0==ID1) ) {
                    alt1=1;
                }


                switch (alt1) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:0:0: formula
            	    {
            	    pushFollow(FOLLOW_formula_in_prog157);
            	    formula();

            	    state._fsp--;
            	    if (state.failed) return ;

            	    }
            	    break;

            	default :
            	    if ( cnt1 >= 1 ) break loop1;
            	    if (state.backtracking>0) {state.failed=true; return ;}
                        EarlyExitException eee =
                            new EarlyExitException(1, input);
                        throw eee;
                }
                cnt1++;
            } while (true);


            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 1, prog_StartIndex); }
        }
        return ;
    }
    // $ANTLR end "prog"


    // $ANTLR start "formula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:44:1: formula returns [Formula f] : (a1= atom | n= notFormula | d= andFormula | o= orFormula | m= impliesFormula | i= iffFormula );
    public final Formula formula() throws RecognitionException {
        Formula f = null;
        int formula_StartIndex = input.index();
        Atom a1 = null;

        NotFormula n = null;

        AndFormula d = null;

        OrFormula o = null;

        ImpliesFormula m = null;

        IffFormula i = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 2) ) { return f; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:44:28: (a1= atom | n= notFormula | d= andFormula | o= orFormula | m= impliesFormula | i= iffFormula )
            int alt2=6;
            switch ( input.LA(1) ) {
            case ID1:
                {
                alt2=1;
                }
                break;
            case NOT:
                {
                alt2=2;
                }
                break;
            case LPAR:
                {
                int LA2_3 = input.LA(2);

                if ( (synpred4_Formula()) ) {
                    alt2=3;
                }
                else if ( (synpred5_Formula()) ) {
                    alt2=4;
                }
                else if ( (synpred6_Formula()) ) {
                    alt2=5;
                }
                else if ( (true) ) {
                    alt2=6;
                }
                else {
                    if (state.backtracking>0) {state.failed=true; return f;}
                    NoViableAltException nvae =
                        new NoViableAltException("", 2, 3, input);

                    throw nvae;
                }
                }
                break;
            default:
                if (state.backtracking>0) {state.failed=true; return f;}
                NoViableAltException nvae =
                    new NoViableAltException("", 2, 0, input);

                throw nvae;
            }

            switch (alt2) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:45:5: a1= atom
                    {
                    pushFollow(FOLLOW_atom_in_formula176);
                    a1=atom();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = a1; 
                    }

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:47:5: n= notFormula
                    {
                    pushFollow(FOLLOW_notFormula_in_formula195);
                    n=notFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = n; 
                    }

                    }
                    break;
                case 3 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:49:5: d= andFormula
                    {
                    pushFollow(FOLLOW_andFormula_in_formula211);
                    d=andFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = d; 
                    }

                    }
                    break;
                case 4 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:51:5: o= orFormula
                    {
                    pushFollow(FOLLOW_orFormula_in_formula227);
                    o=orFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = o; 
                    }

                    }
                    break;
                case 5 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:53:5: m= impliesFormula
                    {
                    pushFollow(FOLLOW_impliesFormula_in_formula243);
                    m=impliesFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = m; 
                    }

                    }
                    break;
                case 6 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:55:5: i= iffFormula
                    {
                    pushFollow(FOLLOW_iffFormula_in_formula259);
                    i=iffFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = i; 
                    }

                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 2, formula_StartIndex); }
        }
        return f;
    }
    // $ANTLR end "formula"


    // $ANTLR start "andFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:58:1: andFormula returns [AndFormula af] : LPAR f1= formula AND f2= formula RPAR ;
    public final AndFormula andFormula() throws RecognitionException {
        AndFormula af = null;
        int andFormula_StartIndex = input.index();
        Formula f1 = null;

        Formula f2 = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 3) ) { return af; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:58:35: ( LPAR f1= formula AND f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:59:5: LPAR f1= formula AND f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_andFormula277); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_andFormula285);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,AND,FOLLOW_AND_in_andFormula291); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_andFormula300);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_andFormula306); if (state.failed) return af;
            if ( state.backtracking==0 ) {
               af = new AndFormula(f1, f2); 
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 3, andFormula_StartIndex); }
        }
        return af;
    }
    // $ANTLR end "andFormula"


    // $ANTLR start "orFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:66:1: orFormula returns [OrFormula af] : LPAR f1= formula OR f2= formula RPAR ;
    public final OrFormula orFormula() throws RecognitionException {
        OrFormula af = null;
        int orFormula_StartIndex = input.index();
        Formula f1 = null;

        Formula f2 = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 4) ) { return af; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:66:33: ( LPAR f1= formula OR f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:67:5: LPAR f1= formula OR f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_orFormula327); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_orFormula335);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,OR,FOLLOW_OR_in_orFormula341); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_orFormula349);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_orFormula355); if (state.failed) return af;
            if ( state.backtracking==0 ) {
               af = new OrFormula(f1, f2); 
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 4, orFormula_StartIndex); }
        }
        return af;
    }
    // $ANTLR end "orFormula"


    // $ANTLR start "impliesFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:74:1: impliesFormula returns [ImpliesFormula af] : LPAR f1= formula IMPLIES f2= formula RPAR ;
    public final ImpliesFormula impliesFormula() throws RecognitionException {
        ImpliesFormula af = null;
        int impliesFormula_StartIndex = input.index();
        Formula f1 = null;

        Formula f2 = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 5) ) { return af; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:74:43: ( LPAR f1= formula IMPLIES f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:75:5: LPAR f1= formula IMPLIES f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_impliesFormula376); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_impliesFormula384);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,IMPLIES,FOLLOW_IMPLIES_in_impliesFormula390); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_impliesFormula399);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_impliesFormula405); if (state.failed) return af;
            if ( state.backtracking==0 ) {
               af = new ImpliesFormula(f1, f2); 
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 5, impliesFormula_StartIndex); }
        }
        return af;
    }
    // $ANTLR end "impliesFormula"


    // $ANTLR start "iffFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:82:1: iffFormula returns [IffFormula af] : LPAR f1= formula IFF f2= formula RPAR ;
    public final IffFormula iffFormula() throws RecognitionException {
        IffFormula af = null;
        int iffFormula_StartIndex = input.index();
        Formula f1 = null;

        Formula f2 = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 6) ) { return af; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:82:35: ( LPAR f1= formula IFF f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:83:5: LPAR f1= formula IFF f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_iffFormula426); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_iffFormula434);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,IFF,FOLLOW_IFF_in_iffFormula440); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_iffFormula449);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_iffFormula455); if (state.failed) return af;
            if ( state.backtracking==0 ) {
               af = new IffFormula(f1, f2); 
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 6, iffFormula_StartIndex); }
        }
        return af;
    }
    // $ANTLR end "iffFormula"


    // $ANTLR start "notFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:91:1: notFormula returns [NotFormula nf] : NOT f= formula ;
    public final NotFormula notFormula() throws RecognitionException {
        NotFormula nf = null;
        int notFormula_StartIndex = input.index();
        Formula f = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 7) ) { return nf; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:91:35: ( NOT f= formula )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:92:5: NOT f= formula
            {
            match(input,NOT,FOLLOW_NOT_in_notFormula477); if (state.failed) return nf;
            pushFollow(FOLLOW_formula_in_notFormula485);
            f=formula();

            state._fsp--;
            if (state.failed) return nf;
            if ( state.backtracking==0 ) {
               nf = new NotFormula(f); 
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 7, notFormula_StartIndex); }
        }
        return nf;
    }
    // $ANTLR end "notFormula"


    // $ANTLR start "atom"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:95:1: atom returns [Atom f] : ID1 LPAR t= term ( COMMA t= term )* RPAR ;
    public final Atom atom() throws RecognitionException {
        Atom f = null;
        int atom_StartIndex = input.index();
        Token ID11=null;
        Term t = null;



            String functor = ""; 
            ArrayList<Term> terms = new ArrayList<Term>(); 
        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 8) ) { return f; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:98:53: ( ID1 LPAR t= term ( COMMA t= term )* RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:99:5: ID1 LPAR t= term ( COMMA t= term )* RPAR
            {
            ID11=(Token)match(input,ID1,FOLLOW_ID1_in_atom511); if (state.failed) return f;
            if ( state.backtracking==0 ) {
              functor = (ID11!=null?ID11.getText():null); 
            }
            match(input,LPAR,FOLLOW_LPAR_in_atom523); if (state.failed) return f;
            pushFollow(FOLLOW_term_in_atom536);
            t=term();

            state._fsp--;
            if (state.failed) return f;
            if ( state.backtracking==0 ) {
               terms.add(t); 
            }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:102:9: ( COMMA t= term )*
            loop3:
            do {
                int alt3=2;
                int LA3_0 = input.LA(1);

                if ( (LA3_0==COMMA) ) {
                    alt3=1;
                }


                switch (alt3) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:103:13: COMMA t= term
            	    {
            	    match(input,COMMA,FOLLOW_COMMA_in_atom562); if (state.failed) return f;
            	    pushFollow(FOLLOW_term_in_atom579);
            	    t=term();

            	    state._fsp--;
            	    if (state.failed) return f;
            	    if ( state.backtracking==0 ) {
            	       terms.add(t); 
            	    }

            	    }
            	    break;

            	default :
            	    break loop3;
                }
            } while (true);

            match(input,RPAR,FOLLOW_RPAR_in_atom603); if (state.failed) return f;
            if ( state.backtracking==0 ) {
               f = new Atom(functor, terms); 
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 8, atom_StartIndex); }
        }
        return f;
    }
    // $ANTLR end "atom"


    // $ANTLR start "term"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:110:1: term returns [Term t] : (v= variable | c= constant );
    public final Term term() throws RecognitionException {
        Term t = null;
        int term_StartIndex = input.index();
        Variable v = null;

        Constant c = null;


        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 9) ) { return t; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:110:22: (v= variable | c= constant )
            int alt4=2;
            int LA4_0 = input.LA(1);

            if ( (LA4_0==20) ) {
                alt4=1;
            }
            else if ( ((LA4_0>=ID1 && LA4_0<=CONST2)) ) {
                alt4=2;
            }
            else {
                if (state.backtracking>0) {state.failed=true; return t;}
                NoViableAltException nvae =
                    new NoViableAltException("", 4, 0, input);

                throw nvae;
            }
            switch (alt4) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:111:5: v= variable
                    {
                    pushFollow(FOLLOW_variable_in_term635);
                    v=variable();

                    state._fsp--;
                    if (state.failed) return t;
                    if ( state.backtracking==0 ) {
                      t = v;
                    }

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:113:5: c= constant
                    {
                    pushFollow(FOLLOW_constant_in_term651);
                    c=constant();

                    state._fsp--;
                    if (state.failed) return t;
                    if ( state.backtracking==0 ) {
                      t = c;
                    }

                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 9, term_StartIndex); }
        }
        return t;
    }
    // $ANTLR end "term"


    // $ANTLR start "variable"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:115:1: variable returns [Variable v] : '$' ID1 ;
    public final Variable variable() throws RecognitionException {
        Variable v = null;
        int variable_StartIndex = input.index();
        Token ID12=null;

        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 10) ) { return v; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:115:30: ( '$' ID1 )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:116:5: '$' ID1
            {
            match(input,20,FOLLOW_20_in_variable668); if (state.failed) return v;
            ID12=(Token)match(input,ID1,FOLLOW_ID1_in_variable674); if (state.failed) return v;
            if ( state.backtracking==0 ) {
              v = new Variable((ID12!=null?ID12.getText():null));
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 10, variable_StartIndex); }
        }
        return v;
    }
    // $ANTLR end "variable"


    // $ANTLR start "constant"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:119:1: constant returns [Constant c] : ( ID1 | CONST1 | CONST2 );
    public final Constant constant() throws RecognitionException {
        Constant c = null;
        int constant_StartIndex = input.index();
        Token ID13=null;
        Token CONST14=null;
        Token CONST25=null;

        try {
            if ( state.backtracking>0 && alreadyParsedRule(input, 11) ) { return c; }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:119:30: ( ID1 | CONST1 | CONST2 )
            int alt5=3;
            switch ( input.LA(1) ) {
            case ID1:
                {
                alt5=1;
                }
                break;
            case CONST1:
                {
                alt5=2;
                }
                break;
            case CONST2:
                {
                alt5=3;
                }
                break;
            default:
                if (state.backtracking>0) {state.failed=true; return c;}
                NoViableAltException nvae =
                    new NoViableAltException("", 5, 0, input);

                throw nvae;
            }

            switch (alt5) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:120:5: ID1
                    {
                    ID13=(Token)match(input,ID1,FOLLOW_ID1_in_constant691); if (state.failed) return c;
                    if ( state.backtracking==0 ) {
                      c = new Constant((ID13!=null?ID13.getText():null));
                    }

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:122:5: CONST1
                    {
                    CONST14=(Token)match(input,CONST1,FOLLOW_CONST1_in_constant705); if (state.failed) return c;
                    if ( state.backtracking==0 ) {
                      c = new Constant((CONST14!=null?CONST14.getText():null));
                    }

                    }
                    break;
                case 3 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:124:5: CONST2
                    {
                    CONST25=(Token)match(input,CONST2,FOLLOW_CONST2_in_constant719); if (state.failed) return c;
                    if ( state.backtracking==0 ) {
                      c = new Constant((CONST25!=null?CONST25.getText():null));
                    }

                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            if ( state.backtracking>0 ) { memoize(input, 11, constant_StartIndex); }
        }
        return c;
    }
    // $ANTLR end "constant"

    // $ANTLR start synpred4_Formula
    public final void synpred4_Formula_fragment() throws RecognitionException {   
        AndFormula d = null;


        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:49:5: (d= andFormula )
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:49:5: d= andFormula
        {
        pushFollow(FOLLOW_andFormula_in_synpred4_Formula211);
        d=andFormula();

        state._fsp--;
        if (state.failed) return ;

        }
    }
    // $ANTLR end synpred4_Formula

    // $ANTLR start synpred5_Formula
    public final void synpred5_Formula_fragment() throws RecognitionException {   
        OrFormula o = null;


        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:51:5: (o= orFormula )
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:51:5: o= orFormula
        {
        pushFollow(FOLLOW_orFormula_in_synpred5_Formula227);
        o=orFormula();

        state._fsp--;
        if (state.failed) return ;

        }
    }
    // $ANTLR end synpred5_Formula

    // $ANTLR start synpred6_Formula
    public final void synpred6_Formula_fragment() throws RecognitionException {   
        ImpliesFormula m = null;


        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:53:5: (m= impliesFormula )
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:53:5: m= impliesFormula
        {
        pushFollow(FOLLOW_impliesFormula_in_synpred6_Formula243);
        m=impliesFormula();

        state._fsp--;
        if (state.failed) return ;

        }
    }
    // $ANTLR end synpred6_Formula

    // Delegated rules

    public final boolean synpred6_Formula() {
        state.backtracking++;
        int start = input.mark();
        try {
            synpred6_Formula_fragment(); // can never throw exception
        } catch (RecognitionException re) {
            System.err.println("impossible: "+re);
        }
        boolean success = !state.failed;
        input.rewind(start);
        state.backtracking--;
        state.failed=false;
        return success;
    }
    public final boolean synpred5_Formula() {
        state.backtracking++;
        int start = input.mark();
        try {
            synpred5_Formula_fragment(); // can never throw exception
        } catch (RecognitionException re) {
            System.err.println("impossible: "+re);
        }
        boolean success = !state.failed;
        input.rewind(start);
        state.backtracking--;
        state.failed=false;
        return success;
    }
    public final boolean synpred4_Formula() {
        state.backtracking++;
        int start = input.mark();
        try {
            synpred4_Formula_fragment(); // can never throw exception
        } catch (RecognitionException re) {
            System.err.println("impossible: "+re);
        }
        boolean success = !state.failed;
        input.rewind(start);
        state.backtracking--;
        state.failed=false;
        return success;
    }


 

    public static final BitSet FOLLOW_formula_in_prog157 = new BitSet(new long[]{0x0000000000002502L});
    public static final BitSet FOLLOW_atom_in_formula176 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_notFormula_in_formula195 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_andFormula_in_formula211 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_orFormula_in_formula227 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_impliesFormula_in_formula243 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_iffFormula_in_formula259 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_andFormula277 = new BitSet(new long[]{0x0000000000002510L});
    public static final BitSet FOLLOW_formula_in_andFormula285 = new BitSet(new long[]{0x0000000000000010L});
    public static final BitSet FOLLOW_AND_in_andFormula291 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_andFormula300 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_andFormula306 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_orFormula327 = new BitSet(new long[]{0x0000000000002520L});
    public static final BitSet FOLLOW_formula_in_orFormula335 = new BitSet(new long[]{0x0000000000000020L});
    public static final BitSet FOLLOW_OR_in_orFormula341 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_orFormula349 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_orFormula355 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_impliesFormula376 = new BitSet(new long[]{0x0000000000002540L});
    public static final BitSet FOLLOW_formula_in_impliesFormula384 = new BitSet(new long[]{0x0000000000000040L});
    public static final BitSet FOLLOW_IMPLIES_in_impliesFormula390 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_impliesFormula399 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_impliesFormula405 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_iffFormula426 = new BitSet(new long[]{0x0000000000002580L});
    public static final BitSet FOLLOW_formula_in_iffFormula434 = new BitSet(new long[]{0x0000000000000080L});
    public static final BitSet FOLLOW_IFF_in_iffFormula440 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_iffFormula449 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_iffFormula455 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_NOT_in_notFormula477 = new BitSet(new long[]{0x0000000000002500L});
    public static final BitSet FOLLOW_formula_in_notFormula485 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_ID1_in_atom511 = new BitSet(new long[]{0x0000000000000400L});
    public static final BitSet FOLLOW_LPAR_in_atom523 = new BitSet(new long[]{0x000000000010E000L});
    public static final BitSet FOLLOW_term_in_atom536 = new BitSet(new long[]{0x0000000000001800L});
    public static final BitSet FOLLOW_COMMA_in_atom562 = new BitSet(new long[]{0x000000000010E000L});
    public static final BitSet FOLLOW_term_in_atom579 = new BitSet(new long[]{0x0000000000001800L});
    public static final BitSet FOLLOW_RPAR_in_atom603 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_variable_in_term635 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_constant_in_term651 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_20_in_variable668 = new BitSet(new long[]{0x0000000000002000L});
    public static final BitSet FOLLOW_ID1_in_variable674 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_ID1_in_constant691 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_CONST1_in_constant705 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_CONST2_in_constant719 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_andFormula_in_synpred4_Formula211 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_orFormula_in_synpred5_Formula227 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_impliesFormula_in_synpred6_Formula243 = new BitSet(new long[]{0x0000000000000002L});

}