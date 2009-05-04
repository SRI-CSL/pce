package com.sri.csl.xpce.object;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class OrFormula extends Formula {
	
	protected Formula first, second;
	
	public OrFormula(Formula f1, Formula f2) {
		first = f1;
		second = f2;
	}

	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		JSONArray arr = new JSONArray();
		arr.put(first.toJSON());
		arr.put(second.toJSON());
		obj.put(XPCEConstants.OR, arr);
		return obj;
	}

	public Formula getFirst() {
		return first;
	}
	
	public Formula getSecond() {
		return second;
	}	
	
	public boolean equals(Object obj) {
		if ( !(obj instanceof OrFormula) ) return false;
		OrFormula other = (OrFormula)obj;
		if ( first.equals(other.getFirst()) && second.equals(other.getSecond()) ) return true;
		if ( second.equals(other.getFirst()) && first.equals(other.getSecond()) ) return true;
		return false;
	}

	public String toString() {
		return "(" + first + " | " + second + ")";
	}
	
}
