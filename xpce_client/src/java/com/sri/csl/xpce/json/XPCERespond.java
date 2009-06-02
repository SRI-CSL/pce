package com.sri.csl.xpce.json;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;

public class XPCERespond {
	protected String err = null, warn = null;
	protected Object result = null;
	protected boolean successful = true;
	
	
	public XPCERespond(JSONObject obj) throws JSONException, XPCException {
		if ( obj == JSONObject.NULL ) return; // Successful call.
		if ( obj.has(XPCEConstants.XPCEERROR) ) {
			successful = false;
			err = obj.getString(XPCEConstants.XPCEERROR);
			throw new XPCException("XPCE SERVER ERROR: " + err);
		}
		if ( obj.has(XPCEConstants.XPCEWARNING) ) {
			warn = obj.getString(XPCEConstants.XPCEWARNING);
			System.out.println(warn);
		}
		if ( obj.has(XPCEConstants.XPCERESULT) )
			result = obj.get(XPCEConstants.XPCERESULT);
	}

	public String getError() {
		return err;
	}

	public Object getResult() {
		return result;
	}
	
	public String getWarning() {
		return warn;
	}

	public boolean succeeded() {
		return successful;
	}

}
