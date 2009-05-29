package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;

public class Sort {
	protected String name;
	protected Sort superSort;
	
	public Sort(JSONObject obj) throws JSONException {
		name = obj.getString(XPCEConstants.NAME);
		if ( obj.has(XPCEConstants.SUPER) )
			superSort = new Sort(obj.getString(XPCEConstants.SUPER));
	}
	
	public Sort(Sort s) {
		name = s.getName();
		superSort = s.getSuperSort();
	}
	
	public Sort(String n) {
		name = n;
		superSort = null;
	}

	public Sort(String n, Sort sup) {
		name = n;
		superSort = sup;
	}
	
	public Sort(String n, String sup) throws XPCException {
		if ( n.equals(sup) ) throw new XPCException("The sort name is the same as its super sort");
		name = n;
		superSort = new Sort(sup);
	}

	public String getName() {
		return name;
	}
	
	public Sort getSuperSort() {
		return superSort;
	}
	
	public void setSuperSort(Sort s)  throws XPCException {
		if ( name.equals(s.getName()) ) throw new XPCException("The sort name is the same as its super sort");
		superSort = s;
	}
	
	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.NAME, name);
		if ( superSort != null ) obj.put(XPCEConstants.SUPER, superSort.getName());
		return obj;
	}
	
	@Override
	public String toString() {
		return name;
	}

}
