package com.sri.csl.xpce.object;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;

public class FormulaAndProbability {
	protected Formula original, instance;
	protected double probability;
	protected Map<Variable, Constant> subst;
	
	@SuppressWarnings("unchecked")
	public FormulaAndProbability(Formula original, JSONObject obj) throws JSONException, XPCException {
		instance = Formula.createFromJSON(obj.getJSONObject(XPCEConstants.FORMULAINSTANCE));
		this.original = original;
		probability = obj.getDouble(XPCEConstants.PROBABILITY);
		JSONObject subs = obj.getJSONObject(XPCEConstants.SUBST);
		Iterator keys = subs.keys();
		subst = new HashMap<Variable, Constant>();
		while ( keys.hasNext() ) {
			String key = keys.next().toString();
			String value = subs.getString(key);
			subst.put(new Variable(key), new Constant(value));
		}
	}
	
	public Formula getOriginalFormula() {
		return original;
	}
	
	public Formula getFormulaInstance() {
		return instance;
	}
	
	public double getProbability() {
		return probability;
	}
	
	public Constant getValue(String var) {
		return subst.get(new Variable(var));
	}
	
	public Constant getValue(Variable var) {
		return subst.get(var);
	}
	
	public boolean contains(String var) {
		return subst.containsKey(new Variable(var));
	}

	public boolean contains(Variable var) {
		return subst.containsKey(var);
	}
	
	public Map<Variable, Constant> getAllSubst() {
		return subst;
	}
	
	public String toString() {
		return "" + instance + " -- " + probability;
	}
}
