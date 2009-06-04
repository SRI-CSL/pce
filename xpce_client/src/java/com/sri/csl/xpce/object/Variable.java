package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Variable extends Term {
	
	public Variable(JSONObject obj) throws JSONException {
		name = obj.getString(XPCEConstants.VARIABLE);
	}
	
	public Variable(String name) {
		this.name = name;
	}

	@Override
	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.VARIABLE, name);
		return obj;
	}

	@Override
	public String toString() {
		return XPCEConstants.VARIABLEPREFIX + name;
	}	

}
