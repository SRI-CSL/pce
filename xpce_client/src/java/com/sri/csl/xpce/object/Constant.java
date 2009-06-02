package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Constant extends Term {
	protected String name;
	protected Sort sort;
		
	public Constant(JSONObject obj) throws JSONException {
		name = obj.getString(XPCEConstants.CONST);
		if ( obj.has(XPCEConstants.SORT) )
			sort = new Sort(obj.getString(XPCEConstants.SORT));
	}

	public Constant(String name) {
		this.name = name;
		this.sort = null;
	}

	public Constant(String name, Sort sort) {
		this.name = name;
		this.sort = sort;
	}

	public Object getName() {
		return name;
	}
	
	public Sort getSort() {
		return sort;
	}
	

	public String toJSON() {
		return name;
	}
/*	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.CONST, name);
		if ( sort != null )
			obj.put(XPCEConstants.SORT, sort.getName());
		return obj;
	}
*/
	
	@Override
	public String toString() {
		return name.toString();
	}
}
