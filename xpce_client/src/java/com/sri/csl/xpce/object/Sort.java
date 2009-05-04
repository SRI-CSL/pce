package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Sort {
	protected String name;
	
	public Sort(String n) {
		name = n;
	}
	
	public Sort(Sort s) {
		name = s.getName();
	}
	
	public String getName() {
		return name;
	}
	
	public String toString() {
		return name;
	}
	
	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.NAME , name);
		return obj;
	}

}
