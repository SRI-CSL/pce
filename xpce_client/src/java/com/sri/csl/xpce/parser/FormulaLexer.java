package com.sri.csl.xpce.parser;

// $ANTLR 3.1.2 C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g 2009-06-26 10:45:47

import org.antlr.runtime.*;
import java.util.Stack;
import java.util.List;
import java.util.ArrayList;

public class FormulaLexer extends Lexer {
    public static final int IFF=7;
    public static final int QUOTE=9;
    public static final int WS=15;
    public static final int T__16=16;
    public static final int IMPLIES=6;
    public static final int LPAR=10;
    public static final int NEWLINE=14;
    public static final int COMMA=12;
    public static final int OR=5;
    public static final int RPAR=11;
    public static final int NOT=8;
    public static final int AND=4;
    public static final int ID=13;
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

    // $ANTLR start "T__16"
    public final void mT__16() throws RecognitionException {
        try {
            int _type = T__16;
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
    // $ANTLR end "T__16"

    // $ANTLR start "ID"
    public final void mID() throws RecognitionException {
        try {
            int _type = ID;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:130:9: ( ( 'a' .. 'z' | 'A' .. 'Z' ) ( '_' | '0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' )* )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:130:13: ( 'a' .. 'z' | 'A' .. 'Z' ) ( '_' | '0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' )*
            {
            if ( (input.LA(1)>='A' && input.LA(1)<='Z')||(input.LA(1)>='a' && input.LA(1)<='z') ) {
                input.consume();

            }
            else {
                MismatchedSetException mse = new MismatchedSetException(null,input);
                recover(mse);
                throw mse;}

            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:130:33: ( '_' | '0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' )*
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
    // $ANTLR end "ID"

    // $ANTLR start "NEWLINE"
    public final void mNEWLINE() throws RecognitionException {
        try {
            int _type = NEWLINE;
            int _channel = DEFAULT_TOKEN_CHANNEL;
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:131:9: ( ( '\\r' )? '\\n' )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:131:13: ( '\\r' )? '\\n'
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:131:13: ( '\\r' )?
            int alt2=2;
            int LA2_0 = input.LA(1);

            if ( (LA2_0=='\r') ) {
                alt2=1;
            }
            switch (alt2) {
                case 1 :
                    // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:131:13: '\\r'
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
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:132:5: ( ( ' ' | '\\t' )+ )
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:132:9: ( ' ' | '\\t' )+
            {
            // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:132:9: ( ' ' | '\\t' )+
            int cnt3=0;
            loop3:
            do {
                int alt3=2;
                int LA3_0 = input.LA(1);

                if ( (LA3_0=='\t'||LA3_0==' ') ) {
                    alt3=1;
                }


                switch (alt3) {
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
            	    if ( cnt3 >= 1 ) break loop3;
                        EarlyExitException eee =
                            new EarlyExitException(3, input);
                        throw eee;
                }
                cnt3++;
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
        // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:8: ( AND | OR | IMPLIES | IFF | NOT | QUOTE | LPAR | RPAR | COMMA | T__16 | ID | NEWLINE | WS )
        int alt4=13;
        alt4 = dfa4.predict(input);
        switch (alt4) {
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
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:55: T__16
                {
                mT__16(); 

                }
                break;
            case 11 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:61: ID
                {
                mID(); 

                }
                break;
            case 12 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:64: NEWLINE
                {
                mNEWLINE(); 

                }
                break;
            case 13 :
                // C:\\pce\\xpce_client\\src\\java\\com\\sri\\csl\\xpce\\parser\\Formula.g:1:72: WS
                {
                mWS(); 

                }
                break;

        }

    }


    protected DFA4 dfa4 = new DFA4(this);
    static final String DFA4_eotS =
        "\1\uffff\2\13\13\uffff\1\13\1\21\1\22\2\uffff";
    static final String DFA4_eofS =
        "\23\uffff";
    static final String DFA4_minS =
        "\1\11\1\156\1\162\13\uffff\1\144\2\60\2\uffff";
    static final String DFA4_maxS =
        "\1\172\1\156\1\162\13\uffff\1\144\2\172\2\uffff";
    static final String DFA4_acceptS =
        "\3\uffff\1\3\1\4\1\5\1\6\1\7\1\10\1\11\1\12\1\13\1\14\1\15\3\uffff"+
        "\1\2\1\1";
    static final String DFA4_specialS =
        "\23\uffff}>";
    static final String[] DFA4_transitionS = {
            "\1\15\1\14\2\uffff\1\14\22\uffff\1\15\1\uffff\1\6\1\uffff\1"+
            "\12\3\uffff\1\7\1\10\2\uffff\1\11\1\5\16\uffff\1\4\1\3\3\uffff"+
            "\32\13\6\uffff\1\1\15\13\1\2\13\13",
            "\1\16",
            "\1\17",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "\1\20",
            "\12\13\7\uffff\32\13\4\uffff\1\13\1\uffff\32\13",
            "\12\13\7\uffff\32\13\4\uffff\1\13\1\uffff\32\13",
            "",
            ""
    };

    static final short[] DFA4_eot = DFA.unpackEncodedString(DFA4_eotS);
    static final short[] DFA4_eof = DFA.unpackEncodedString(DFA4_eofS);
    static final char[] DFA4_min = DFA.unpackEncodedStringToUnsignedChars(DFA4_minS);
    static final char[] DFA4_max = DFA.unpackEncodedStringToUnsignedChars(DFA4_maxS);
    static final short[] DFA4_accept = DFA.unpackEncodedString(DFA4_acceptS);
    static final short[] DFA4_special = DFA.unpackEncodedString(DFA4_specialS);
    static final short[][] DFA4_transition;

    static {
        int numStates = DFA4_transitionS.length;
        DFA4_transition = new short[numStates][];
        for (int i=0; i<numStates; i++) {
            DFA4_transition[i] = DFA.unpackEncodedString(DFA4_transitionS[i]);
        }
    }

    class DFA4 extends DFA {

        public DFA4(BaseRecognizer recognizer) {
            this.recognizer = recognizer;
            this.decisionNumber = 4;
            this.eot = DFA4_eot;
            this.eof = DFA4_eof;
            this.min = DFA4_min;
            this.max = DFA4_max;
            this.accept = DFA4_accept;
            this.special = DFA4_special;
            this.transition = DFA4_transition;
        }
        public String getDescription() {
            return "1:1: Tokens : ( AND | OR | IMPLIES | IFF | NOT | QUOTE | LPAR | RPAR | COMMA | T__16 | ID | NEWLINE | WS );";
        }
    }
 

}