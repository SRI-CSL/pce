package com.sri.csl.xpce.object;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class ImpliesFormula extends Formula {

	protected Formula first, second;
	
	public ImpliesFormula(Formula f1, Formula f2) {
		first = f1;
		second = f2;
	}

	public boolean equals(Object obj) {
		if ( !(obj instanceof ImpliesFormula) ) return false;
		ImpliesFormula other = (ImpliesFormula)obj;
		if ( first.equals(other.getFirst()) && second.equals(other.getSecond()) ) return true;
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
		obj.put(XPCEConstants.IMPLIES, arr);
		return obj;
	}

	public String toString() {
		return "(" + first + " => " + second + ")";
	}
}
