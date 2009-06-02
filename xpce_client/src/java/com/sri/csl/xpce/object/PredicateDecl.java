package com.sri.csl.xpce.object;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class PredicateDecl {
	protected ArrayList<Sort> argumentType = new ArrayList<Sort>();
	protected String functor;
	protected boolean observable = true;
	
	public PredicateDecl(String functor, List<Object> parType, boolean obs) {
		this.functor = functor;
		for (Object p: parType) argumentType.add(new Sort(p.toString()));
		observable = obs;
	}

	public PredicateDecl(String functor, Sort... parType) {
		this.functor = functor;
		for (Sort p: parType) argumentType.add(p);
	}

	public PredicateDecl(boolean observable, String functor, Sort... parType) {
		this.functor = functor;
		for (Sort p: parType) argumentType.add(p);
		this.observable = observable;
	}

	public PredicateDecl(String functor, String... parType) {
		this.functor = functor;
		for (String p: parType) argumentType.add(new Sort(p));
	}

	public PredicateDecl(String functor, String[] parType, boolean obs) {
		this.functor = functor;
		for (String p: parType) argumentType.add(new Sort(p));
		observable = obs;
	}

	public ArrayList<Sort> getArgumentsTypes() {
		return argumentType;
	}
	
	public String getFunctor() {
		return functor;
	}
	
	public JSONObject toFullJSON() throws JSONException  {
		JSONObject obj = new JSONObject();
		obj.append(XPCEConstants.PREDICATE, toJSON());
		return obj;
	}
	
	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.PREDICATE, functor);
		JSONArray params = new JSONArray();
		for (Sort p: argumentType) params.put(p.getName()) ;
		obj.put(XPCEConstants.ARGUMENTS, params);
		obj.put(XPCEConstants.OBSERVABLE, observable);
		return obj;
	}
	
	public String toString() {
		String str = functor + "(";
		StringBuffer buffer = new StringBuffer();
		Iterator<Sort> iter = argumentType.iterator();
		while (iter.hasNext()) {
			buffer.append(iter.next());
			if (iter.hasNext())	buffer.append(", ");
		}
		return str + buffer + ")";
		
	}
}
