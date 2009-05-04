package com.sri.csl.xpce.json;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.object.Atom;
import com.sri.csl.xpce.object.Constant;
import com.sri.csl.xpce.object.Predicate;
import com.sri.csl.xpce.object.Sort;
import com.sri.csl.xpce.object.SubSort;

public class JSONCommand {
	public static JSONObject fullAddSort(Sort s) throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.SORT, s.toJSON());
		return obj;
	}

	public static JSONObject fullAddSubSort(SubSort s) throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.SUBSORT, s.toJSON());
		return obj;
	}

	public static JSONObject fullAddPredicate(Predicate p) throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.PREDICATE, p.toJSON());
		return obj;
	}
	
	public static JSONObject fullAddConstant(Constant c) throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.CONST, c.toJSON());
		return obj;
	}
	
	public static JSONObject assertAtom(Atom a) throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.ASSERT, a.toJSON());
		return obj;
	}
	
	
}



