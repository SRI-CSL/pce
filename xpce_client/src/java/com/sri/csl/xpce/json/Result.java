package com.sri.csl.xpce.json;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;

public class Result {
	protected String err = null, warn = null;
	protected Object result = null;
	protected boolean successful = true;
	
	
	public Result(JSONObject obj) throws JSONException, XPCException {
		if ( obj == JSONObject.NULL ) return; // Successful call.
		if ( obj.has(XPCEConstants.XPCEERROR) ) {
			successful = false;
			err = obj.getString(XPCEConstants.XPCEERROR);
			throw new XPCException(err);
		}
		if ( obj.has(XPCEConstants.XPCEWARNING) )
			warn = obj.getString(XPCEConstants.XPCEWARNING);
			System.out.println(warn);
		// TODO: result should be translated from JSON to the proper object
		if ( obj.has(XPCEConstants.XPCERESULT) )
			result = obj.get(XPCEConstants.XPCERESULT);
	}

	public String getError() {
		return err;
	}

	public String getWarning() {
		return warn;
	}
	
	public Object getResult() {
		return result;
	}

	public boolean succeeded() {
		return successful;
	}

}
