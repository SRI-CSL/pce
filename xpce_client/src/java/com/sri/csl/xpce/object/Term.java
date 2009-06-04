package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;

public abstract class Term {
	protected String name;	
	
	public static Term createFromJSON(Object obj) throws JSONException, XPCException {
		if ( obj instanceof JSONObject )
			return new Variable((JSONObject)obj);
		else
			return new Constant(obj.toString());
	}
	
	public String getName() {
		return name;
	}
	
	public abstract Object toJSON() throws JSONException;
}
