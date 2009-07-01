package com.sri.csl.xpce.parser;

// $ANTLR 3.1.2 C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g 2009-07-01 16:46:21

import org.antlr.runtime.*;
import java.util.Stack;
import java.util.List;
import java.util.ArrayList;

public class FormulaLexer extends Lexer {
    public static final int FUNCONS=13;
    public static final int IFF=7;
    public static final int QUOTE=9;
    public static final int WS=17;
    public static final int IMPLIES=6;
    public static final int LPAR=10;
    public static final int NEWLINE=16;
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

    public FormulaLexer() {;} 
    public FormulaLexer(CharStream input) {
        this(input, new RecognizerSharedState());
    }
    public FormulaLexer(CharStream input, RecognizerSharedState state) {
        super(input,state);

    }
    public String getGrammarFileName() { return "C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g"; }

    // $ANTLR start "AND"
    public final void mAND() throws RecognitionException {
        try {
            int _type = AND;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:7:5: ( '&' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:7:7: '&'
            {
            match('&'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "AND"

    // $ANTLR start "OR"
    public final void mOR() throws RecognitionException {
        try {
            int _type = OR;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:8:4: ( '|' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:8:6: '|'
            {
            match('|'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "OR"

    // $ANTLR start "IMPLIES"
    public final void mIMPLIES() throws RecognitionException {
        try {
            int _type = IMPLIES;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:9:9: ( '=>' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:9:11: '=>'
            {
            match("=>"); 


            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "IMPLIES"

    // $ANTLR start "IFF"
    public final void mIFF() throws RecognitionException {
        try {
            int _type = IFF;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:10:5: ( '<=>' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:10:7: '<=>'
            {
            match("<=>"); 


            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "IFF"

    // $ANTLR start "NOT"
    public final void mNOT() throws RecognitionException {
        try {
            int _type = NOT;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:11:5: ( '-' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:11:7: '-'
            {
            match('-'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "NOT"

    // $ANTLR start "QUOTE"
    public final void mQUOTE() throws RecognitionException {
        try {
            int _type = QUOTE;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:12:7: ( '\\'' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:12:9: '\\''
            {
            match('\''); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "QUOTE"

    // $ANTLR start "LPAR"
    public final void mLPAR() throws RecognitionException {
        try {
            int _type = LPAR;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:13:6: ( '(' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:13:8: '('
            {
            match('('); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "LPAR"

    // $ANTLR start "RPAR"
    public final void mRPAR() throws RecognitionException {
        try {
            int _type = RPAR;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:14:6: ( ')' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:14:8: ')'
            {
            match(')'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "RPAR"

    // $ANTLR start "COMMA"
    public final void mCOMMA() throws RecognitionException {
        try {
            int _type = COMMA;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:15:7: ( ',' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:15:9: ','
            {
            match(','); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "COMMA"

    // $ANTLR start "VAR"
    public final void mVAR() throws RecognitionException {
        try {
            int _type = VAR;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:100:6: ( '$' ( 'a' .. 'z' | 'A' .. 'Z' ) ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' | '.' )* )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:100:10: '$' ( 'a' .. 'z' | 'A' .. 'Z' ) ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' | '.' )*
            {
            match('$'); 
            if ( (input.LA(1)>='A' && input.LA(1)<='Z')||(input.LA(1)>='a' && input.LA(1)<='z') ) {
                input.consume();

            }
            else {
                MismatchedSetException mse = new MismatchedSetException(null,input);
                recover(mse);
                throw mse;}

            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:100:34: ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' | '.' )*
            loop1:
            do {
                int alt1=2;
                int LA1_0 = input.LA(1);

                if ( (LA1_0=='.'||(LA1_0>='0' && LA1_0<='9')||(LA1_0>='A' && LA1_0<='Z')||LA1_0=='_'||(LA1_0>='a' && LA1_0<='z')) ) {
                    alt1=1;
                }


                switch (alt1) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            	    {
            	    if ( input.LA(1)=='.'||(input.LA(1)>='0' && input.LA(1)<='9')||(input.LA(1)>='A' && input.LA(1)<='Z')||input.LA(1)=='_'||(input.LA(1)>='a' && input.LA(1)<='z') ) {
            	        input.consume();

            	    }
            	    else {
            	        MismatchedSetException mse = new MismatchedSetException(null,input);
            	        recover(mse);
            	        throw mse;}


            	    }
            	    break;

            	default :
            	    break loop1;
                }
            } while (true);


            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "VAR"

    // $ANTLR start "FUNCONS"
    public final void mFUNCONS() throws RecognitionException {
        try {
            int _type = FUNCONS;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:101:10: ( ( 'a' .. 'z' | 'A' .. 'Z' ) ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' | '.' )* )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:101:14: ( 'a' .. 'z' | 'A' .. 'Z' ) ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' | '.' )*
            {
            if ( (input.LA(1)>='A' && input.LA(1)<='Z')||(input.LA(1)>='a' && input.LA(1)<='z') ) {
                input.consume();

            }
            else {
                MismatchedSetException mse = new MismatchedSetException(null,input);
                recover(mse);
                throw mse;}

            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:101:34: ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' | '.' )*
            loop2:
            do {
                int alt2=2;
                int LA2_0 = input.LA(1);

                if ( (LA2_0=='.'||(LA2_0>='0' && LA2_0<='9')||(LA2_0>='A' && LA2_0<='Z')||LA2_0=='_'||(LA2_0>='a' && LA2_0<='z')) ) {
                    alt2=1;
                }


                switch (alt2) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            	    {
            	    if ( input.LA(1)=='.'||(input.LA(1)>='0' && input.LA(1)<='9')||(input.LA(1)>='A' && input.LA(1)<='Z')||input.LA(1)=='_'||(input.LA(1)>='a' && input.LA(1)<='z') ) {
            	        input.consume();

            	    }
            	    else {
            	        MismatchedSetException mse = new MismatchedSetException(null,input);
            	        recover(mse);
            	        throw mse;}


            	    }
            	    break;

            	default :
            	    break loop2;
                }
            } while (true);


            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "FUNCONS"

    // $ANTLR start "CONST"
    public final void mCONST() throws RecognitionException {
        try {
            int _type = CONST;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:102:10: ( QUOTE ( '0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' | '_' | '.' | ':' | '/' | '\\\\' | ' ' )+ QUOTE )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:102:14: QUOTE ( '0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' | '_' | '.' | ':' | '/' | '\\\\' | ' ' )+ QUOTE
            {
            mQUOTE(); 
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:102:20: ( '0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' | '_' | '.' | ':' | '/' | '\\\\' | ' ' )+
            int cnt3=0;
            loop3:
            do {
                int alt3=2;
                int LA3_0 = input.LA(1);

                if ( (LA3_0==' '||(LA3_0>='.' && LA3_0<=':')||(LA3_0>='A' && LA3_0<='Z')||LA3_0=='\\'||LA3_0=='_'||(LA3_0>='a' && LA3_0<='z')) ) {
                    alt3=1;
                }


                switch (alt3) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            	    {
            	    if ( input.LA(1)==' '||(input.LA(1)>='.' && input.LA(1)<=':')||(input.LA(1)>='A' && input.LA(1)<='Z')||input.LA(1)=='\\'||input.LA(1)=='_'||(input.LA(1)>='a' && input.LA(1)<='z') ) {
            	        input.consume();

            	    }
            	    else {
            	        MismatchedSetException mse = new MismatchedSetException(null,input);
            	        recover(mse);
            	        throw mse;}


            	    }
            	    break;

            	default :
            	    if ( cnt3 >= 1 ) break loop3;
                        EarlyExitException eee =
                            new EarlyExitException(3, input);
                        throw eee;
                }
                cnt3++;
            } while (true);

            mQUOTE(); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "CONST"

    // $ANTLR start "NEWLINE"
    public final void mNEWLINE() throws RecognitionException {
        try {
            int _type = NEWLINE;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:103:10: ( ( '\\r' )? '\\n' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:103:14: ( '\\r' )? '\\n'
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:103:14: ( '\\r' )?
            int alt4=2;
            int LA4_0 = input.LA(1);

            if ( (LA4_0=='\r') ) {
                alt4=1;
            }
            switch (alt4) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:103:14: '\\r'
                    {
                    match('\r'); 

                    }
                    break;

            }

            match('\n'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "NEWLINE"

    // $ANTLR start "WS"
    public final void mWS() throws RecognitionException {
        try {
            int _type = WS;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:104:5: ( ( ' ' | '\\t' )+ )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:104:9: ( ' ' | '\\t' )+
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:104:9: ( ' ' | '\\t' )+
            int cnt5=0;
            loop5:
            do {
                int alt5=2;
                int LA5_0 = input.LA(1);

                if ( (LA5_0=='\t'||LA5_0==' ') ) {
                    alt5=1;
                }


                switch (alt5) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            	    {
            	    if ( input.LA(1)=='\t'||input.LA(1)==' ' ) {
            	        input.consume();

            	    }
            	    else {
            	        MismatchedSetException mse = new MismatchedSetException(null,input);
            	        recover(mse);
            	        throw mse;}


            	    }
            	    break;

            	default :
            	    if ( cnt5 >= 1 ) break loop5;
                        EarlyExitException eee =
                            new EarlyExitException(5, input);
                        throw eee;
                }
                cnt5++;
            } while (true);

            skip();

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "WS"

    public void mTokens() throws RecognitionException {
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:8: ( AND | OR | IMPLIES | IFF | NOT | QUOTE | LPAR | RPAR | COMMA | VAR | FUNCONS | CONST | NEWLINE | WS )
        int alt6=14;
        alt6 = dfa6.predict(input);
        switch (alt6) {
            case 1 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:10: AND
                {
                mAND(); 

                }
                break;
            case 2 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:14: OR
                {
                mOR(); 

                }
                break;
            case 3 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:17: IMPLIES
                {
                mIMPLIES(); 

                }
                break;
            case 4 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:25: IFF
                {
                mIFF(); 

                }
                break;
            case 5 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:29: NOT
                {
                mNOT(); 

                }
                break;
            case 6 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:33: QUOTE
                {
                mQUOTE(); 

                }
                break;
            case 7 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:39: LPAR
                {
                mLPAR(); 

                }
                break;
            case 8 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:44: RPAR
                {
                mRPAR(); 

                }
                break;
            case 9 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:49: COMMA
                {
                mCOMMA(); 

                }
                break;
            case 10 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:55: VAR
                {
                mVAR(); 

                }
                break;
            case 11 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:59: FUNCONS
                {
                mFUNCONS(); 

                }
                break;
            case 12 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:67: CONST
                {
                mCONST(); 

                }
                break;
            case 13 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:73: NEWLINE
                {
                mNEWLINE(); 

                }
                break;
            case 14 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:81: WS
                {
                mWS(); 

                }
                break;

        }

    }


    protected DFA6 dfa6 = new DFA6(this);
    static final String DFA6_eotS =
        "\6\uffff\1\16\11\uffff";
    static final String DFA6_eofS =
        "\20\uffff";
    static final String DFA6_minS =
        "\1\11\5\uffff\1\40\11\uffff";
    static final String DFA6_maxS =
        "\1\174\5\uffff\1\172\11\uffff";
    static final String DFA6_acceptS =
        "\1\uffff\1\1\1\2\1\3\1\4\1\5\1\uffff\1\7\1\10\1\11\1\12\1\13\1"+
        "\15\1\16\1\6\1\14";
    static final String DFA6_specialS =
        "\20\uffff}>";
    static final String[] DFA6_transitionS = {
            "\1\15\1\14\2\uffff\1\14\22\uffff\1\15\3\uffff\1\12\1\uffff"+
            "\1\1\1\6\1\7\1\10\2\uffff\1\11\1\5\16\uffff\1\4\1\3\3\uffff"+
            "\32\13\6\uffff\32\13\1\uffff\1\2",
            "",
            "",
            "",
            "",
            "",
            "\1\17\15\uffff\15\17\6\uffff\32\17\1\uffff\1\17\2\uffff\1"+
            "\17\1\uffff\32\17",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            ""
    };

    static final short[] DFA6_eot = DFA.unpackEncodedString(DFA6_eotS);
    static final short[] DFA6_eof = DFA.unpackEncodedString(DFA6_eofS);
    static final char[] DFA6_min = DFA.unpackEncodedStringToUnsignedChars(DFA6_minS);
    static final char[] DFA6_max = DFA.unpackEncodedStringToUnsignedChars(DFA6_maxS);
    static final short[] DFA6_accept = DFA.unpackEncodedString(DFA6_acceptS);
    static final short[] DFA6_special = DFA.unpackEncodedString(DFA6_specialS);
    static final short[][] DFA6_transition;

    static {
        int numStates = DFA6_transitionS.length;
        DFA6_transition = new short[numStates][];
        for (int i=0; i<numStates; i++) {
            DFA6_transition[i] = DFA.unpackEncodedString(DFA6_transitionS[i]);
        }
    }

    class DFA6 extends DFA {

        public DFA6(BaseRecognizer recognizer) {
            this.recognizer = recognizer;
            this.decisionNumber = 6;
            this.eot = DFA6_eot;
            this.eof = DFA6_eof;
            this.min = DFA6_min;
            this.max = DFA6_max;
            this.accept = DFA6_accept;
            this.special = DFA6_special;
            this.transition = DFA6_transition;
        }
        public String getDescription() {
            return "1:1: Tokens : ( AND | OR | IMPLIES | IFF | NOT | QUOTE | LPAR | RPAR | COMMA | VAR | FUNCONS | CONST | NEWLINE | WS );";
        }
    }
 

}