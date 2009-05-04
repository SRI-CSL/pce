package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class SubSort extends Sort {
	protected String superSort;
	
	public SubSort(String n, String sup) {
		super(n);
		superSort = sup;
	}
	
	public SubSort(Sort n, String sup) {
		super(n.getName());
		superSort = sup;
	}

	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.SUB, name);
		obj.append(XPCEConstants.SUPER, superSort);
		return obj;
	}
}
