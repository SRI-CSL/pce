package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;

public abstract class Term {
	public static Term createFromJSON(JSONObject obj) throws JSONException, XPCException {
		if ( obj.has(XPCEConstants.VARIABLE) ) {
			return new Variable(obj);
		} else if ( obj.has(XPCEConstants.CONST) ) {
			return new Constant(obj);
		} else {
			throw new XPCException("Format error in JSON Term " + obj);
		}
	}
	
	public abstract Object toJSON() throws JSONException;
}
