package com.sri.csl.xpce.object;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;

public class Fact extends Atom {

	public Fact(PredicateDecl pred, Constant... params) {
		super(pred, params);
	}

	public Fact(PredicateDecl pred, Object... params) throws XPCException {
		super(pred, params);
	}	

	@Override
	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.PREDICATEFUNCTOR, functor);
		JSONArray params = new JSONArray();
		for (Term p: argument) params.put(p.toJSON()) ;
		obj.put(XPCEConstants.ARGUMENTS, params);

		JSONObject obj1 = new JSONObject();
		obj1.put(XPCEConstants.FACT, obj);		
		return obj1;
	}

}
