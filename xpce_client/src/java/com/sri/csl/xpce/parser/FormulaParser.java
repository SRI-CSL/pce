// $ANTLR 3.1.2 C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g 2009-07-01 16:38:38

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

public class FormulaParser extends Parser {
    public static final String[] tokenNames = new String[] {
        "<invalid>", "<EOR>", "<DOWN>", "<UP>", "AND", "OR", "IMPLIES", "IFF", "NOT", "QUOTE", "LPAR", "RPAR", "COMMA", "FUNCONS", "VAR", "CONST", "NEWLINE", "WS"
    };
    public static final int FUNCONS=13;
    public static final int IFF=7;
    public static final int QUOTE=9;
    public static final int WS=17;
    public static final int IMPLIES=6;
    public static final int NEWLINE=16;
    public static final int LPAR=10;
    public static final int COMMA=12;
    public static final int CONST=15;
    public static final int OR=5;
    public static final int RPAR=11;
    public static final int VAR=14;
    public static final int NOT=8;
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
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:1: prog : ( formula )+ ;
    public final void prog() throws RecognitionException {
        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:5: ( ( formula )+ )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:7: ( formula )+
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:7: ( formula )+
            int cnt1=0;
            loop1:
            do {
                int alt1=2;
                int LA1_0 = input.LA(1);

                if ( (LA1_0==NOT||LA1_0==LPAR||LA1_0==FUNCONS) ) {
                    alt1=1;
                }


                switch (alt1) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:42:7: formula
            	    {
            	    pushFollow(FOLLOW_formula_in_prog149);
            	    formula();

            	    state._fsp--;


            	    }
            	    break;

            	default :
            	    if ( cnt1 >= 1 ) break loop1;
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
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:44:1: formula returns [Formula f] : (at= atom | ( NOT nf= formula ) | ( LPAR f1= formula op= ( AND | OR | IMPLIES | IFF ) f2= formula RPAR ) );
    public final Formula formula() throws RecognitionException {
        Formula f = null;

        Token op=null;
        Atom at = null;

        Formula nf = null;

        Formula f1 = null;

        Formula f2 = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:44:28: (at= atom | ( NOT nf= formula ) | ( LPAR f1= formula op= ( AND | OR | IMPLIES | IFF ) f2= formula RPAR ) )
            int alt2=3;
            switch ( input.LA(1) ) {
            case FUNCONS:
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
                alt2=3;
                }
                break;
            default:
                NoViableAltException nvae =
                    new NoViableAltException("", 2, 0, input);

                throw nvae;
            }

            switch (alt2) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:45:5: at= atom
                    {
                    pushFollow(FOLLOW_atom_in_formula168);
                    at=atom();

                    state._fsp--;

                     f = at; 

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:47:5: ( NOT nf= formula )
                    {
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:47:5: ( NOT nf= formula )
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:47:6: NOT nf= formula
                    {
                    match(input,NOT,FOLLOW_NOT_in_formula186); 
                    pushFollow(FOLLOW_formula_in_formula195);
                    nf=formula();

                    state._fsp--;

                     f = new NotFormula(nf); 

                    }


                    }
                    break;
                case 3 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:50:5: ( LPAR f1= formula op= ( AND | OR | IMPLIES | IFF ) f2= formula RPAR )
                    {
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:50:5: ( LPAR f1= formula op= ( AND | OR | IMPLIES | IFF ) f2= formula RPAR )
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:50:6: LPAR f1= formula op= ( AND | OR | IMPLIES | IFF ) f2= formula RPAR
                    {
                    match(input,LPAR,FOLLOW_LPAR_in_formula211); 
                    pushFollow(FOLLOW_formula_in_formula220);
                    f1=formula();

                    state._fsp--;

                    op=(Token)input.LT(1);
                    if ( (input.LA(1)>=AND && input.LA(1)<=IFF) ) {
                        input.consume();
                        state.errorRecovery=false;
                    }
                    else {
                        MismatchedSetException mse = new MismatchedSetException(null,input);
                        throw mse;
                    }

                    pushFollow(FOLLOW_formula_in_formula246);
                    f2=formula();

                    state._fsp--;

                    switch (op.getType()) { 
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
                         		
                    match(input,RPAR,FOLLOW_RPAR_in_formula255); 

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


    // $ANTLR start "atom"
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:68:1: atom returns [Atom f] : FUNCONS LPAR t= term ( COMMA t= term )* RPAR ;
    public final Atom atom() throws RecognitionException {
        Atom f = null;

        Token FUNCONS1=null;
        Term t = null;



            String functor = ""; 
            ArrayList<Term> terms = new ArrayList<Term>(); 
        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:71:53: ( FUNCONS LPAR t= term ( COMMA t= term )* RPAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:72:5: FUNCONS LPAR t= term ( COMMA t= term )* RPAR
            {
            FUNCONS1=(Token)match(input,FUNCONS,FOLLOW_FUNCONS_in_atom280); 
            functor = (FUNCONS1!=null?FUNCONS1.getText():null); 
            match(input,LPAR,FOLLOW_LPAR_in_atom292); 
            pushFollow(FOLLOW_term_in_atom305);
            t=term();

            state._fsp--;

             terms.add(t); 
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:75:9: ( COMMA t= term )*
            loop3:
            do {
                int alt3=2;
                int LA3_0 = input.LA(1);

                if ( (LA3_0==COMMA) ) {
                    alt3=1;
                }


                switch (alt3) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:76:13: COMMA t= term
            	    {
            	    match(input,COMMA,FOLLOW_COMMA_in_atom331); 
            	    pushFollow(FOLLOW_term_in_atom348);
            	    t=term();

            	    state._fsp--;

            	     terms.add(t); 

            	    }
            	    break;

            	default :
            	    break loop3;
                }
            } while (true);

            match(input,RPAR,FOLLOW_RPAR_in_atom372); 
             f = new Atom(functor, terms); 

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
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:83:1: term returns [Term t] : (v= variable | c= constant );
    public final Term term() throws RecognitionException {
        Term t = null;

        Variable v = null;

        Constant c = null;


        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:83:22: (v= variable | c= constant )
            int alt4=2;
            int LA4_0 = input.LA(1);

            if ( (LA4_0==VAR) ) {
                alt4=1;
            }
            else if ( (LA4_0==FUNCONS||LA4_0==CONST) ) {
                alt4=2;
            }
            else {
                NoViableAltException nvae =
                    new NoViableAltException("", 4, 0, input);

                throw nvae;
            }
            switch (alt4) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:84:5: v= variable
                    {
                    pushFollow(FOLLOW_variable_in_term404);
                    v=variable();

                    state._fsp--;

                    t = v;

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:86:5: c= constant
                    {
                    pushFollow(FOLLOW_constant_in_term420);
                    c=constant();

                    state._fsp--;

                    t = c;

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
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:88:1: variable returns [Variable v] : VAR ;
    public final Variable variable() throws RecognitionException {
        Variable v = null;

        Token VAR2=null;

        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:88:30: ( VAR )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:89:5: VAR
            {
            VAR2=(Token)match(input,VAR,FOLLOW_VAR_in_variable437); 
            v = new Variable((VAR2!=null?VAR2.getText():null));

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
    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:91:1: constant returns [Constant c] : ( FUNCONS | CONST );
    public final Constant constant() throws RecognitionException {
        Constant c = null;

        Token FUNCONS3=null;
        Token CONST4=null;

        try {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:91:30: ( FUNCONS | CONST )
            int alt5=2;
            int LA5_0 = input.LA(1);

            if ( (LA5_0==FUNCONS) ) {
                alt5=1;
            }
            else if ( (LA5_0==CONST) ) {
                alt5=2;
            }
            else {
                NoViableAltException nvae =
                    new NoViableAltException("", 5, 0, input);

                throw nvae;
            }
            switch (alt5) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:92:5: FUNCONS
                    {
                    FUNCONS3=(Token)match(input,FUNCONS,FOLLOW_FUNCONS_in_constant454); 
                    c = new Constant((FUNCONS3!=null?FUNCONS3.getText():null));

                    }
                    break;
                case 2 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:94:5: CONST
                    {
                    CONST4=(Token)match(input,CONST,FOLLOW_CONST_in_constant468); 
                    c = new Constant((CONST4!=null?CONST4.getText():null));

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

    // Delegated rules


 

    public static final BitSet FOLLOW_formula_in_prog149 = new BitSet(new long[]{0x0000000000002502L});
    public static final BitSet FOLLOW_atom_in_formula168 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_NOT_in_formula186 = new BitSet(new long[]{0x0000000000002500L});
    public static final BitSet FOLLOW_formula_in_formula195 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LPAR_in_formula211 = new BitSet(new long[]{0x00000000000025F0L});
    public static final BitSet FOLLOW_formula_in_formula220 = new BitSet(new long[]{0x00000000000000F0L});
    public static final BitSet FOLLOW_set_in_formula229 = new BitSet(new long[]{0x0000000000002D00L});
    public static final BitSet FOLLOW_formula_in_formula246 = new BitSet(new long[]{0x0000000000000800L});
    public static final BitSet FOLLOW_RPAR_in_formula255 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_FUNCONS_in_atom280 = new BitSet(new long[]{0x0000000000000400L});
    public static final BitSet FOLLOW_LPAR_in_atom292 = new BitSet(new long[]{0x000000000000E000L});
    public static final BitSet FOLLOW_term_in_atom305 = new BitSet(new long[]{0x0000000000001800L});
    public static final BitSet FOLLOW_COMMA_in_atom331 = new BitSet(new long[]{0x000000000000E000L});
    public static final BitSet FOLLOW_term_in_atom348 = new BitSet(new long[]{0x0000000000001800L});
    public static final BitSet FOLLOW_RPAR_in_atom372 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_variable_in_term404 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_constant_in_term420 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_VAR_in_variable437 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_FUNCONS_in_constant454 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_CONST_in_constant468 = new BitSet(new long[]{0x0000000000000002L});

}