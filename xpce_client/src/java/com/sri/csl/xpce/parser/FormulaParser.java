// $ANTLR 3.1.2 C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g 2009-06-26 10:45:47

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
        "<invalid>", "<EOR>", "<DOWN>", "<UP>", "AND", "OR", "IMPLIES", "IFF", "NOT", "QUOTE", "LPAR", "RPAR", "COMMA", "ID", "NEWLINE", "WS", "'$'"
    };
    public static final int IFF=7;
    public static final int QUOTE=9;
    public static final int WS=15;
    public static final int T__16=16;
    public static final int IMPLIES=6;
    public static final int NEWLINE=14;
    public static final int LPAR=10;
    public static final int COMMA=12;
    public static final int OR=5;
    public static final int RPAR=11;
    public static final int NOT=8;
    public static final int ID=13;
    public static final int AND=4;
    public static final int EOF=-1;

    // delegates
    // delegators


        public FormulaParser(TokenStream input) {
            this(input, new RecognizerSharedState());
        }
        public FormulaParser(TokenStream input, RecognizerSharedState state) {
            super(input, state);
             
        }
        

    public String[] getTokenNames() { return FormulaParser.tokenNames; }
    public String getGrammarFileName() { return "C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g"; }


    /** Map variable name to Integer object holding value */
    //HashMap memory = new HashMap();



    // $ANTLR start "prog"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:41:1: prog : ( formula )+ ;
    public final void prog() throws RecognitionException {
        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:41:5: ( ( formula )+ )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:41:7: ( formula )+
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:41:7: ( formula )+
            int cnt1=0;
            loop1:
            do {
                int alt1=2;
                int LA1_0 = input.LA(1);

                if ( (LA1_0==NOT||LA1_0==LPAR||LA1_0==ID) ) {
                    alt1=1;
                }


                switch (alt1) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:0:0: formula
            	    {
            	    pushFollow(FOLLOW_formula_in_prog148);
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
        }
        return ;
    }
    // $ANTLR end "prog"


    // $ANTLR start "formula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:43:1: formula returns [Formula f] : (a1= atom | n= notFormula | d= andFormula | o= orFormula | m= impliesFormula | i= iffFormula );
    public final Formula formula() throws RecognitionException {
        Formula f = null;

        Atom a1 = null;

        NotFormula n = null;

        AndFormula d = null;

        OrFormula o = null;

        ImpliesFormula m = null;

        IffFormula i = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:43:28: (a1= atom | n= notFormula | d= andFormula | o= orFormula | m= impliesFormula | i= iffFormula )
            int alt2=6;
            switch ( input.LA(1) ) {
            case ID:
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
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:44:5: a1= atom
                    {
                    pushFollow(FOLLOW_atom_in_formula167);
                    a1=atom();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = a1; 
                    }

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:46:5: n= notFormula
                    {
                    pushFollow(FOLLOW_notFormula_in_formula186);
                    n=notFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = n; 
                    }

                    }
                    break;
                case 3 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:48:5: d= andFormula
                    {
                    pushFollow(FOLLOW_andFormula_in_formula202);
                    d=andFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = d; 
                    }

                    }
                    break;
                case 4 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:50:5: o= orFormula
                    {
                    pushFollow(FOLLOW_orFormula_in_formula218);
                    o=orFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = o; 
                    }

                    }
                    break;
                case 5 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:52:5: m= impliesFormula
                    {
                    pushFollow(FOLLOW_impliesFormula_in_formula234);
                    m=impliesFormula();

                    state._fsp--;
                    if (state.failed) return f;
                    if ( state.backtracking==0 ) {
                       f = m; 
                    }

                    }
                    break;
                case 6 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:54:5: i= iffFormula
                    {
                    pushFollow(FOLLOW_iffFormula_in_formula250);
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
        }
        return f;
    }
    // $ANTLR end "formula"


    // $ANTLR start "andFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:57:1: andFormula returns [AndFormula af] : LPAR f1= formula AND f2= formula RPAR ;
    public final AndFormula andFormula() throws RecognitionException {
        AndFormula af = null;

        Formula f1 = null;

        Formula f2 = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:57:35: ( LPAR f1= formula AND f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:58:5: LPAR f1= formula AND f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_andFormula268); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_andFormula276);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,AND,FOLLOW_AND_in_andFormula282); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_andFormula291);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_andFormula297); if (state.failed) return af;
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
        }
        return af;
    }
    // $ANTLR end "andFormula"


    // $ANTLR start "orFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:65:1: orFormula returns [OrFormula af] : LPAR f1= formula OR f2= formula RPAR ;
    public final OrFormula orFormula() throws RecognitionException {
        OrFormula af = null;

        Formula f1 = null;

        Formula f2 = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:65:33: ( LPAR f1= formula OR f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:66:5: LPAR f1= formula OR f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_orFormula318); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_orFormula326);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,OR,FOLLOW_OR_in_orFormula332); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_orFormula340);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_orFormula346); if (state.failed) return af;
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
        }
        return af;
    }
    // $ANTLR end "orFormula"


    // $ANTLR start "impliesFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:73:1: impliesFormula returns [ImpliesFormula af] : LPAR f1= formula IMPLIES f2= formula RPAR ;
    public final ImpliesFormula impliesFormula() throws RecognitionException {
        ImpliesFormula af = null;

        Formula f1 = null;

        Formula f2 = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:73:43: ( LPAR f1= formula IMPLIES f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:74:5: LPAR f1= formula IMPLIES f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_impliesFormula367); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_impliesFormula375);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,IMPLIES,FOLLOW_IMPLIES_in_impliesFormula381); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_impliesFormula390);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_impliesFormula396); if (state.failed) return af;
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
        }
        return af;
    }
    // $ANTLR end "impliesFormula"


    // $ANTLR start "iffFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:81:1: iffFormula returns [IffFormula af] : LPAR f1= formula IFF f2= formula RPAR ;
    public final IffFormula iffFormula() throws RecognitionException {
        IffFormula af = null;

        Formula f1 = null;

        Formula f2 = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:81:35: ( LPAR f1= formula IFF f2= formula RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:82:5: LPAR f1= formula IFF f2= formula RPAR
            {
            match(input,LPAR,FOLLOW_LPAR_in_iffFormula417); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_iffFormula425);
            f1=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,IFF,FOLLOW_IFF_in_iffFormula431); if (state.failed) return af;
            pushFollow(FOLLOW_formula_in_iffFormula440);
            f2=formula();

            state._fsp--;
            if (state.failed) return af;
            match(input,RPAR,FOLLOW_RPAR_in_iffFormula446); if (state.failed) return af;
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
        }
        return af;
    }
    // $ANTLR end "iffFormula"


    // $ANTLR start "notFormula"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:90:1: notFormula returns [NotFormula nf] : NOT f= formula ;
    public final NotFormula notFormula() throws RecognitionException {
        NotFormula nf = null;

        Formula f = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:90:35: ( NOT f= formula )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:91:5: NOT f= formula
            {
            match(input,NOT,FOLLOW_NOT_in_notFormula468); if (state.failed) return nf;
            pushFollow(FOLLOW_formula_in_notFormula476);
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
        }
        return nf;
    }
    // $ANTLR end "notFormula"


    // $ANTLR start "atom"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:94:1: atom returns [Atom f] : ID LPAR t= term ( COMMA t= term )* RPAR ;
    public final Atom atom() throws RecognitionException {
        Atom f = null;

        Token ID1=null;
        Term t = null;



            String functor = ""; 
            ArrayList<Term> terms = new ArrayList<Term>(); 
        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:97:53: ( ID LPAR t= term ( COMMA t= term )* RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:98:5: ID LPAR t= term ( COMMA t= term )* RPAR
            {
            ID1=(Token)match(input,ID,FOLLOW_ID_in_atom502); if (state.failed) return f;
            if ( state.backtracking==0 ) {
              functor = (ID1!=null?ID1.getText():null); 
            }
            match(input,LPAR,FOLLOW_LPAR_in_atom514); if (state.failed) return f;
            pushFollow(FOLLOW_term_in_atom527);
            t=term();

            state._fsp--;
            if (state.failed) return f;
            if ( state.backtracking==0 ) {
               terms.add(t); 
            }
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:101:9: ( COMMA t= term )*
            loop3:
            do {
                int alt3=2;
                int LA3_0 = input.LA(1);

                if ( (LA3_0==COMMA) ) {
                    alt3=1;
                }


                switch (alt3) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:102:13: COMMA t= term
            	    {
            	    match(input,COMMA,FOLLOW_COMMA_in_atom553); if (state.failed) return f;
            	    pushFollow(FOLLOW_term_in_atom570);
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

            match(input,RPAR,FOLLOW_RPAR_in_atom594); if (state.failed) return f;
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
        }
        return f;
    }
    // $ANTLR end "atom"


    // $ANTLR start "term"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:109:1: term returns [Term t] : (v= variable | c= constant );
    public final Term term() throws RecognitionException {
        Term t = null;

        Variable v = null;

        Constant c = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:109:22: (v= variable | c= constant )
            int alt4=2;
            int LA4_0 = input.LA(1);

            if ( (LA4_0==16) ) {
                alt4=1;
            }
            else if ( (LA4_0==QUOTE||LA4_0==ID) ) {
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
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:110:5: v= variable
                    {
                    pushFollow(FOLLOW_variable_in_term626);
                    v=variable();

                    state._fsp--;
                    if (state.failed) return t;
                    if ( state.backtracking==0 ) {
                      t = v;
                    }

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:112:5: c= constant
                    {
                    pushFollow(FOLLOW_constant_in_term642);
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
        }
        return t;
    }
    // $ANTLR end "term"


    // $ANTLR start "variable"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:114:1: variable returns [Variable v] : '$' ID ;
    public final Variable variable() throws RecognitionException {
        Variable v = null;

        Token ID2=null;

        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:114:30: ( '$' ID )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:115:5: '$' ID
            {
            match(input,16,FOLLOW_16_in_variable659); if (state.failed) return v;
            ID2=(Token)match(input,ID,FOLLOW_ID_in_variable665); if (state.failed) return v;
            if ( state.backtracking==0 ) {
              v = new Variable((ID2!=null?ID2.getText():null));
            }

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
        }
        return v;
    }
    // $ANTLR end "variable"


    // $ANTLR start "constant"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:118:1: constant returns [Constant c] : ( ID | QUOTE ID QUOTE );
    public final Constant constant() throws RecognitionException {
        Constant c = null;

        Token ID3=null;
        Token ID4=null;

        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:118:30: ( ID | QUOTE ID QUOTE )
            int alt5=2;
            int LA5_0 = input.LA(1);

            if ( (LA5_0==ID) ) {
                alt5=1;
            }
            else if ( (LA5_0==QUOTE) ) {
                alt5=2;
            }
            else {
                if (state.backtracking>0) {state.failed=true; return c;}
                NoViableAltException nvae =
                    new NoViableAltException("", 5, 0, input);

                throw nvae;
            }
            switch (alt5) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:119:5: ID
                    {
                    ID3=(Token)match(input,ID,FOLLOW_ID_in_constant682); if (state.failed) return c;
                    if ( state.backtracking==0 ) {
                      c = new Constant((ID3!=null?ID3.getText():null));
                    }

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:121:5: QUOTE ID QUOTE
                    {
                    match(input,QUOTE,FOLLOW_QUOTE_in_constant696); if (state.failed) return c;
                    ID4=(Token)match(input,ID,FOLLOW_ID_in_constant702); if (state.failed) return c;
                    if ( state.backtracking==0 ) {
                      c = new Constant((ID4!=null?ID4.getText():null));
                    }
                    match(input,QUOTE,FOLLOW_QUOTE_in_constant710); if (state.failed) return c;

                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
        }
        return c;
    }
    // $ANTLR end "constant"

    // $ANTLR start synpred4_Formula
    public final void synpred4_Formula_fragment() throws RecognitionException {   
        AndFormula d = null;


        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:48:5: (d= andFormula )
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:48:5: d= andFormula
        {
        pushFollow(FOLLOW_andFormula_in_synpred4_Formula202);
        d=andFormula();

        state._fsp--;
        if (state.failed) return ;

        }
    }
    // $ANTLR end synpred4_Formula

    // $ANTLR start synpred5_Formula
    public final void synpred5_Formula_fragment() throws RecognitionException {   
        OrFormula o = null;


        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:50:5: (o= orFormula )
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:50:5: o= orFormula
        {
        pushFollow(FOLLOW_orFormula_in_synpred5_Formula218);
        o=orFormula();

        state._fsp--;
        if (state.failed) return ;

        }
    }
    // $ANTLR end synpred5_Formula

    // $ANTLR start synpred6_Formula
    public final void synpred6_Formula_fragment() throws RecognitionException {   
        ImpliesFormula m = null;


        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:52:5: (m= impliesFormula )
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:52:5: m= impliesFormula
        {
        pushFollow(FOLLOW_impliesFormula_in_synpred6_Formula234);
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


 

    public static final BitSet FOLLOW_formula_in_prog148 = new BitSet(new long[]{0x0000000000002502L});
    public static final BitSet FOLLOW_atom_in_formula167 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_notFormula_in_formula186 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_andFormula_in_formula202 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_orFormula_in_formula218 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_impliesFormula_in_formula234 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_iffFormula_in_formula250 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_andFormula268 = new BitSet(new long[]{0x0000000000002510L});
    public static final BitSet FOLLOW_formula_in_andFormula276 = new BitSet(new long[]{0x0000000000000010L});
    public static final BitSet FOLLOW_AND_in_andFormula282 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_andFormula291 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_andFormula297 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_orFormula318 = new BitSet(new long[]{0x0000000000002520L});
    public static final BitSet FOLLOW_formula_in_orFormula326 = new BitSet(new long[]{0x0000000000000020L});
    public static final BitSet FOLLOW_OR_in_orFormula332 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_orFormula340 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_orFormula346 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_impliesFormula367 = new BitSet(new long[]{0x0000000000002540L});
    public static final BitSet FOLLOW_formula_in_impliesFormula375 = new BitSet(new long[]{0x0000000000000040L});
    public static final BitSet FOLLOW_IMPLIES_in_impliesFormula381 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_impliesFormula390 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_impliesFormula396 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_iffFormula417 = new BitSet(new long[]{0x0000000000002580L});
    public static final BitSet FOLLOW_formula_in_iffFormula425 = new BitSet(new long[]{0x0000000000000080L});
    public static final BitSet FOLLOW_IFF_in_iffFormula431 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_iffFormula440 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_iffFormula446 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_NOT_in_notFormula468 = new BitSet(new long[]{0x0000000000002500L});
    public static final BitSet FOLLOW_formula_in_notFormula476 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_ID_in_atom502 = new BitSet(new long[]{0x0000000000000400L});
    public static final BitSet FOLLOW_LPAR_in_atom514 = new BitSet(new long[]{0x0000000000012200L});
    public static final BitSet FOLLOW_term_in_atom527 = new BitSet(new long[]{0x0000000000001800L});
    public static final BitSet FOLLOW_COMMA_in_atom553 = new BitSet(new long[]{0x0000000000012200L});
    public static final BitSet FOLLOW_term_in_atom570 = new BitSet(new long[]{0x0000000000001800L});
    public static final BitSet FOLLOW_RPAR_in_atom594 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_variable_in_term626 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_constant_in_term642 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_16_in_variable659 = new BitSet(new long[]{0x0000000000002000L});
    public static final BitSet FOLLOW_ID_in_variable665 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_ID_in_constant682 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_QUOTE_in_constant696 = new BitSet(new long[]{0x0000000000002000L});
    public static final BitSet FOLLOW_ID_in_constant702 = new BitSet(new long[]{0x0000000000000200L});
    public static final BitSet FOLLOW_QUOTE_in_constant710 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_andFormula_in_synpred4_Formula202 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_orFormula_in_synpred5_Formula218 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_impliesFormula_in_synpred6_Formula234 = new BitSet(new long[]{0x0000000000000002L});

}