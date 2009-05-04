package com.sri.csl.xpce.object;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Predicate {
	protected String functor;
	protected ArrayList<Sort> argumentType = new ArrayList<Sort>();
	protected boolean observable = true;
	
	public Predicate(String functor, String[] parType, boolean obs) {
		this.functor = functor;
		for (String p: parType) argumentType.add(new Sort(p));
		observable = obs;
	}

	public Predicate(String functor, Sort[] parType, boolean obs) {
		this.functor = functor;
		for (Sort p: parType) argumentType.add(p);
		observable = obs;
	}

	public Predicate(String functor, List<Object> parType, boolean obs) {
		this.functor = functor;
		for (Object p: parType) argumentType.add(new Sort(p.toString()));
		observable = obs;
	}

	public Predicate(String functor, String[] parType) {
		this.functor = functor;
		for (String p: parType) argumentType.add(new Sort(p));
	}

	public Predicate(String functor, Sort[] parType) {
		this.functor = functor;
		for (Sort p: parType) argumentType.add(p);
	}

	public Predicate(String functor, List<Object> parType) {
		this.functor = functor;
		for (Object p: parType) argumentType.add(new Sort(p.toString()));
	}

	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.FUNCTOR, functor);
		JSONArray params = new JSONArray();
		for (Sort p: argumentType) params.put(p.getName()) ;
		obj.append(XPCEConstants.ARGUMENTS, params);
		obj.append(XPCEConstants.OBSERVABLE, observable);
		return obj;
	}
	
	public JSONObject toFullJSON() throws JSONException  {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.PREDICATE, toJSON());
		return obj;
	}
}
