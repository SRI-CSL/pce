package com.sri.csl.xpce.object;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;

import org.antlr.runtime.ANTLRInputStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;
import com.sri.csl.xpce.parser.FormulaLexer;
import com.sri.csl.xpce.parser.FormulaParser;

public abstract class Formula {
	private	enum State {Begin, NonAtomicBegin, AtomicBegin, TermsBegin};

	
	public static Formula createFromJSON(JSONObject obj) throws JSONException, XPCException {
		if ( obj.has(XPCEConstants.ATOM) ) {
			JSONObject obj2 = obj.getJSONObject(XPCEConstants.ATOM);
			String functor =  obj2.getString(XPCEConstants.PREDICATEFUNCTOR);
			JSONArray params = obj2.getJSONArray(XPCEConstants.ARGUMENTS);
			ArrayList<Term> argument = new ArrayList<Term>();
			for (int i=0 ; i<params.length() ; i++)
				argument.add(Term.createFromJSON(params.get(i)));
			return new Atom(functor, argument);
		} else if ( obj.has(XPCEConstants.FACT) ) {
				JSONObject obj2 = obj.getJSONObject(XPCEConstants.FACT);
				String functor =  obj2.getString(XPCEConstants.PREDICATEFUNCTOR);
				JSONArray params = obj2.getJSONArray(XPCEConstants.ARGUMENTS);
				ArrayList<Term> argument = new ArrayList<Term>();
				for (int i=0 ; i<params.length() ; i++)
					argument.add(Term.createFromJSON(params.get(i)));
				return new Atom(functor, argument);
		} else if ( obj.has(XPCEConstants.OR) ) {
			JSONArray formulas = obj.getJSONArray(XPCEConstants.OR);
			return new OrFormula(Formula.createFromJSON(formulas.getJSONObject(0)), Formula.createFromJSON(formulas.getJSONObject(1)));
		} else if ( obj.has(XPCEConstants.AND) ) {
			JSONArray formulas = obj.getJSONArray(XPCEConstants.AND);
			return new AndFormula(Formula.createFromJSON(formulas.getJSONObject(0)), Formula.createFromJSON(formulas.getJSONObject(1)));
		} else if ( obj.has(XPCEConstants.IMPLIES) ) {
			JSONArray formulas = obj.getJSONArray(XPCEConstants.IMPLIES);
			return new ImpliesFormula(Formula.createFromJSON(formulas.getJSONObject(0)), Formula.createFromJSON(formulas.getJSONObject(1)));
		} else if ( obj.has(XPCEConstants.IFF) ) {
			JSONArray formulas = obj.getJSONArray(XPCEConstants.IFF);
			return new IffFormula(Formula.createFromJSON(formulas.getJSONObject(0)), Formula.createFromJSON(formulas.getJSONObject(1)));
		} else if ( obj.has(XPCEConstants.NOT) ) {
			return new NotFormula(Formula.createFromJSON(obj.getJSONObject(XPCEConstants.NOT)));
		} else {
			throw new XPCException("Format error in JSON object " + obj);
		}
	}
	
	public static Formula createFromString(String str) throws IOException, RecognitionException {
		ByteArrayInputStream stream = new ByteArrayInputStream(str.getBytes());
		ANTLRInputStream input = new ANTLRInputStream(stream);
		FormulaLexer lexer   = new FormulaLexer(input);
        CommonTokenStream tokens = new CommonTokenStream(lexer);
		FormulaParser parser = new FormulaParser(tokens);
		return parser.formula();
	}

	public static void main(String args[]) {
		try {
			Formula f1 = new NotFormula(new Atom("Age", "Sam", new Variable("x")));
			Formula f2 = Formula.createFromString("Father(Bob, 'Tom Jones')");
			Formula f3 = Formula.createFromString("(Married($x, $y) & Likes($x, $y))");
			Formula f4 = Formula.createFromString(f1.toString());
			Formula f5 = Formula.createFromString(f2.toString());
			Formula f6 = Formula.createFromString(f3.toString());
			
			System.out.println("" + f4);
			System.out.println("" + f5);
			System.out.println("" + f6);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
		
	public abstract JSONObject toJSON() throws JSONException;
	
	public abstract boolean equals(Object obj);
}
