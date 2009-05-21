package com.sri.csl.xpce.object;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class AndFormula extends Formula {

	protected Formula first, second;
	
	public AndFormula(Formula f1, Formula f2) {
		first = f1;
		second = f2;
	}

	public boolean equals(Object obj) {
		if ( !(obj instanceof AndFormula) ) return false;
		AndFormula other = (AndFormula)obj;
		if ( first.equals(other.getFirst()) && second.equals(other.getSecond()) ) return true;
		if ( second.equals(other.getFirst()) && first.equals(other.getSecond()) ) return true;
		return false;
	}

	public Formula getFirst() {
		return first;
	}
	
	public Formula getSecond() {
		return second;
	}	
	
	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		JSONArray arr = new JSONArray();
		arr.put(first.toJSON());
		arr.put(second.toJSON());
		obj.put(XPCEConstants.AND, arr);
		return obj;
	}

	public String toString() {
		return "(" + first + " & " + second + ")";
	}
}
