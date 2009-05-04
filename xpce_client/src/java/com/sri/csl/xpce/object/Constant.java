package com.sri.csl.xpce.object;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Constant {
	protected ArrayList<Object> values = new ArrayList<Object>();
	protected Sort sort;
	
	public Constant(Object value, String s) {
		this(value, new Sort(s));
	}

	public Constant(Object value, Sort s) {
		values.add(value);
		sort = s;
	}

	public Constant(Object[] values, String s) {
		this(values, new Sort(s));
	}

	public Constant(Object[] values, Sort s) {
		for (Object v: values) this.values.add(v);
		sort = s;
	}

	public Constant(List<Object> values, String s) {
		this(values, new Sort(s));
	}

	public Constant(List<Object> values, Sort s) {
		for (Object v: values) this.values.add(v);
		sort = s;
	}

	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		JSONArray params = new JSONArray();
		for (Object o: values) params.put(o) ;
		obj.append(XPCEConstants.VALUES, params);
		obj.append(XPCEConstants.SORT, sort.getName());
		return obj;
	}
	
	public JSONObject toFullJSON() throws JSONException  {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.CONST, toJSON());
		return obj;
	}
	
}
