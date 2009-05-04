package com.sri.csl.xpce.object;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Atom extends Formula {
	protected String functor;
	protected ArrayList<Object> argument = new ArrayList<Object>();
	
	public Atom(String functor, Object... params) {
		this.functor = functor;
		for (Object p: params) argument.add(p);
	}

	public Atom(String functor, List<Object> params) {
		this.functor = functor;
		for (Object p: params) argument.add(p);
	}

	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.FUNCTOR, functor);
		JSONArray params = new JSONArray();
		for (Object p: argument) params.put(p) ;
		obj.put(XPCEConstants.ARGUMENTS, params);

		JSONObject obj1 = new JSONObject();
		obj1.put(XPCEConstants.ATOM, obj);		
		return obj1;
	}
	
	public String getFunctor() {
		return functor;
	}
	
	public Object getArgument(int index) {
		return argument.get(index);
	}
	
	public boolean equals(Object obj) {
		if ( !(obj instanceof Atom) ) return false;
		Atom other = (Atom)obj;
		if ( !functor.equals(other.getFunctor()) || argument.size()!=other.argument.size() ) return false;
		
		for (int i = 0; i< argument.size(); i++)
			if ( !argument.get(i).equals(other.argument.get(i)) ) return false;
		return true;
	}
	
	
	public String toString() {
		String str = functor + "(";
		StringBuffer buffer = new StringBuffer();
		Iterator<Object> iter = argument.iterator();
		while (iter.hasNext()) {
			buffer.append(iter.next());
			if (iter.hasNext())	buffer.append(", ");
		}
		return str + buffer + ")";
	}

}
