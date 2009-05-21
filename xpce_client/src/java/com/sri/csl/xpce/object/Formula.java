package com.sri.csl.xpce.object;

import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;

public abstract class Formula {
	public static Formula createFromJSON(JSONObject obj) throws JSONException, XPCException {
		if ( obj.has(XPCEConstants.ATOM) ) {
			JSONObject obj2 = obj.getJSONObject(XPCEConstants.ATOM);
			String functor =  obj2.getString(XPCEConstants.FUNCTOR);
			JSONArray params = obj2.getJSONArray(XPCEConstants.ARGUMENTS);
			ArrayList<Term> argument = new ArrayList<Term>();
			for (int i=0 ; i<params.length() ; i++)
				argument.add(Term.createFromJSON(params.getJSONObject(i)));
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
	
	public static void main(String args[]) {
		Atom r = new Atom("family(bob, $X, tom, rose)");
		System.out.println("r is: " + r);
		PredicateDecl p1 = new PredicateDecl("Father", "Person", "Person");
		PredicateDecl p2 = new PredicateDecl("Male", "Person");
		Atom a1, a2, a3;
		try {
			a1 = new Atom(p1, "Bob", "Sam");
			a2 = new Atom(p2, "Sam");
			a3 = new Atom("Age", "Sam", new Variable("x"));
			NotFormula na3 = new NotFormula(a3);
			OrFormula f = new OrFormula(a1, a2);
			ImpliesFormula f2 = new ImpliesFormula(na3, f);
			AndFormula f1 = new AndFormula(f2, f);
			AndFormula f3 = new AndFormula(f, f2);
			System.out.println("Formula f1 is: " + f1);
			System.out.println("Formula f3 is: " + f3);
			System.out.println("\nFormula f1(JSON) is: " + f1.toJSON());
			System.out.println("Formula f3(JSON) is: " + f3.toJSON());

			Formula ff = Formula.createFromJSON(f1.toJSON());
			System.out.println("\nFormula ff is: " + ff);
			
			if ( f1.equals(f3) )
				System.out.println("\nf1 and f3 are equal");
			else
				System.out.println("\nf1 and f3 are NOT equal");
		} catch (Exception rr) {
			rr.printStackTrace();
		}		
	}

	public boolean equals(Object o) {
		return false;
	}
	
	public abstract JSONObject toJSON() throws JSONException;


}
