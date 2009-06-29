package com.sri.csl.xpce.parser;

// $ANTLR 3.1.2 C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g 2009-06-29 16:28:49

import org.antlr.runtime.*;
import java.util.Stack;
import java.util.List;
import java.util.ArrayList;

public class FormulaLexer extends Lexer {
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
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:7:5: ( 'and' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:7:7: 'and'
            {
            match("and"); 


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
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:8:4: ( 'or' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:8:6: 'or'
            {
            match("or"); 


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
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:12:7: ( '\\\"' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:12:9: '\\\"'
            {
            match('\"'); 

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

    // $ANTLR start "T__20"
    public final void mT__20() throws RecognitionException {
        try {
            int _type = T__20;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:16:7: ( '$' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:16:9: '$'
            {
            match('$'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "T__20"

    // $ANTLR start "DIGIT"
    public final void mDIGIT() throws RecognitionException {
        try {
            int _type = DIGIT;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:131:8: ( '0' .. '9' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:131:12: '0' .. '9'
            {
            matchRange('0','9'); 

            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "DIGIT"

    // $ANTLR start "ALPHA"
    public final void mALPHA() throws RecognitionException {
        try {
            int _type = ALPHA;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:132:8: ( 'a' .. 'z' | 'A' .. 'Z' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            {
            if ( (input.LA(1)>='A' && input.LA(1)<='Z')||(input.LA(1)>='a' && input.LA(1)<='z') ) {
                input.consume();

            }
            else {
                MismatchedSetException mse = new MismatchedSetException(null,input);
                recover(mse);
                throw mse;}


            }

            state.type = _type;
            state.channel = _channel;
        }
        finally {
        }
    }
    // $ANTLR end "ALPHA"

    // $ANTLR start "ID1"
    public final void mID1() throws RecognitionException {
        try {
            int _type = ID1;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:133:9: ( ALPHA ( DIGIT | ALPHA | '_' )* )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:133:13: ALPHA ( DIGIT | ALPHA | '_' )*
            {
            mALPHA(); 
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:133:19: ( DIGIT | ALPHA | '_' )*
            loop1:
            do {
                int alt1=2;
                int LA1_0 = input.LA(1);

                if ( ((LA1_0>='0' && LA1_0<='9')||(LA1_0>='A' && LA1_0<='Z')||LA1_0=='_'||(LA1_0>='a' && LA1_0<='z')) ) {
                    alt1=1;
                }


                switch (alt1) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            	    {
            	    if ( (input.LA(1)>='0' && input.LA(1)<='9')||(input.LA(1)>='A' && input.LA(1)<='Z')||input.LA(1)=='_'||(input.LA(1)>='a' && input.LA(1)<='z') ) {
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
    // $ANTLR end "ID1"

    // $ANTLR start "CONST1"
    public final void mCONST1() throws RecognitionException {
        try {
            int _type = CONST1;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:134:10: ( DIGIT ( DIGIT | ALPHA | '_' | '.' | ':' | '/' | '\\\\' )* )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:134:14: DIGIT ( DIGIT | ALPHA | '_' | '.' | ':' | '/' | '\\\\' )*
            {
            mDIGIT(); 
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:134:20: ( DIGIT | ALPHA | '_' | '.' | ':' | '/' | '\\\\' )*
            loop2:
            do {
                int alt2=2;
                int LA2_0 = input.LA(1);

                if ( ((LA2_0>='.' && LA2_0<=':')||(LA2_0>='A' && LA2_0<='Z')||LA2_0=='\\'||LA2_0=='_'||(LA2_0>='a' && LA2_0<='z')) ) {
                    alt2=1;
                }


                switch (alt2) {
            	case 1 :
            	    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:
            	    {
            	    if ( (input.LA(1)>='.' && input.LA(1)<=':')||(input.LA(1)>='A' && input.LA(1)<='Z')||input.LA(1)=='\\'||input.LA(1)=='_'||(input.LA(1)>='a' && input.LA(1)<='z') ) {
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
    // $ANTLR end "CONST1"

    // $ANTLR start "CONST2"
    public final void mCONST2() throws RecognitionException {
        try {
            int _type = CONST2;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:135:10: ( QUOTE ( DIGIT | ALPHA | '_' | '.' | ':' | '/' | '\\\\' | ' ' )+ QUOTE )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:135:14: QUOTE ( DIGIT | ALPHA | '_' | '.' | ':' | '/' | '\\\\' | ' ' )+ QUOTE
            {
            mQUOTE(); 
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:135:20: ( DIGIT | ALPHA | '_' | '.' | ':' | '/' | '\\\\' | ' ' )+
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
    // $ANTLR end "CONST2"

    // $ANTLR start "NEWLINE"
    public final void mNEWLINE() throws RecognitionException {
        try {
            int _type = NEWLINE;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:136:9: ( ( '\\r' )? '\\n' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:136:13: ( '\\r' )? '\\n'
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:136:13: ( '\\r' )?
            int alt4=2;
            int LA4_0 = input.LA(1);

            if ( (LA4_0=='\r') ) {
                alt4=1;
            }
            switch (alt4) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:136:13: '\\r'
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
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:137:5: ( ( ' ' | '\\t' )+ )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:137:9: ( ' ' | '\\t' )+
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:137:9: ( ' ' | '\\t' )+
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
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:8: ( AND | OR | IMPLIES | IFF | NOT | QUOTE | LPAR | RPAR | COMMA | T__20 | DIGIT | ALPHA | ID1 | CONST1 | CONST2 | NEWLINE | WS )
        int alt6=17;
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
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:55: T__20
                {
                mT__20(); 

                }
                break;
            case 11 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:61: DIGIT
                {
                mDIGIT(); 

                }
                break;
            case 12 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:67: ALPHA
                {
                mALPHA(); 

                }
                break;
            case 13 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:73: ID1
                {
                mID1(); 

                }
                break;
            case 14 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:77: CONST1
                {
                mCONST1(); 

                }
                break;
            case 15 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:84: CONST2
                {
                mCONST2(); 

                }
                break;
            case 16 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:91: NEWLINE
                {
                mNEWLINE(); 

                }
                break;
            case 17 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:99: WS
                {
                mWS(); 

                }
                break;

        }

    }


    protected DFA6 dfa6 = new DFA6(this);
    static final String DFA6_eotS =
        "\1\uffff\2\20\3\uffff\1\23\4\uffff\1\25\1\20\2\uffff\1\21\2\uffff"+
        "\1\30\4\uffff\1\31\2\uffff";
    static final String DFA6_eofS =
        "\32\uffff";
    static final String DFA6_minS =
        "\1\11\2\60\3\uffff\1\40\4\uffff\1\56\1\60\2\uffff\1\144\2\uffff"+
        "\1\60\4\uffff\1\60\2\uffff";
    static final String DFA6_maxS =
        "\3\172\3\uffff\1\172\4\uffff\2\172\2\uffff\1\144\2\uffff\1\172"+
        "\4\uffff\1\172\2\uffff";
    static final String DFA6_acceptS =
        "\3\uffff\1\3\1\4\1\5\1\uffff\1\7\1\10\1\11\1\12\2\uffff\1\20\1"+
        "\21\1\uffff\1\14\1\15\1\uffff\1\6\1\17\1\13\1\16\1\uffff\1\2\1\1";
    static final String DFA6_specialS =
        "\32\uffff}>";
    static final String[] DFA6_transitionS = {
            "\1\16\1\15\2\uffff\1\15\22\uffff\1\16\1\uffff\1\6\1\uffff\1"+
            "\12\3\uffff\1\7\1\10\2\uffff\1\11\1\5\2\uffff\12\13\2\uffff"+
            "\1\4\1\3\3\uffff\32\14\6\uffff\1\1\15\14\1\2\13\14",
            "\12\21\7\uffff\32\21\4\uffff\1\21\1\uffff\15\21\1\17\14\21",
            "\12\21\7\uffff\32\21\4\uffff\1\21\1\uffff\21\21\1\22\10\21",
            "",
            "",
            "",
            "\1\24\15\uffff\15\24\6\uffff\32\24\1\uffff\1\24\2\uffff\1"+
            "\24\1\uffff\32\24",
            "",
            "",
            "",
            "",
            "\15\26\6\uffff\32\26\1\uffff\1\26\2\uffff\1\26\1\uffff\32"+
            "\26",
            "\12\21\7\uffff\32\21\4\uffff\1\21\1\uffff\32\21",
            "",
            "",
            "\1\27",
            "",
            "",
            "\12\21\7\uffff\32\21\4\uffff\1\21\1\uffff\32\21",
            "",
            "",
            "",
            "",
            "\12\21\7\uffff\32\21\4\uffff\1\21\1\uffff\32\21",
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
            return "1:1: Tokens : ( AND | OR | IMPLIES | IFF | NOT | QUOTE | LPAR | RPAR | COMMA | T__20 | DIGIT | ALPHA | ID1 | CONST1 | CONST2 | NEWLINE | WS );";
        }
    }
 

}