package com.sri.csl.xpce.xmlrpc;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;

import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.Result;
import com.sri.csl.xpce.json.XPCEConstants;
import com.sri.csl.xpce.object.Constant;
import com.sri.csl.xpce.object.Fact;
import com.sri.csl.xpce.object.Formula;
import com.sri.csl.xpce.object.PredicateDecl;
import com.sri.csl.xpce.object.Sort;

public class XpceClient {
	
	public static void main(String[] args) {
	    XpceClient client = new XpceClient("http://localhost:8080/RPC2");
    	Object ret;
	    try {
	    	Sort person = new Sort("Person");
	    	client.addSort(person);
	    	client.addConstant(person, "bob", "tom", "lisa", "rose");
	    	client.addPredicate(new PredicateDecl("married", person, person));
	    	client.addPredicate(new PredicateDecl("likes", new Sort[] {person, person}, false));
	    	Formula f = null;
	    	client.add(f, 3.2, "TestClient");
	    	
	    	ret = client.command("mcsat 0.01, 20.0, 0.01, 30, 500;");
	    	System.out.println("Return Value:" + ret);
	    	
	    	//Thread.sleep(10000);
	    	
	    }catch (Exception e) {
	    	e.printStackTrace();
	    }
	}
	protected XmlRpcClient xmlrpcClient = null;
	
	protected URL xpceServerURL;
	
	public XpceClient(String serverURL) {
	    try {
	    	xpceServerURL = new URL(serverURL);
		    XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();
	    	config.setServerURL(xpceServerURL);
		    xmlrpcClient = new XmlRpcClient();
		    xmlrpcClient.setConfig(config);
	    } catch (MalformedURLException m) {
	    	m.printStackTrace();
	    }
	}
	
	public boolean add(Formula formula, double weight, String source) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.FORMULA, formula);
		obj.put(XPCEConstants.WEIGHT, weight);
		obj.put(XPCEConstants.SOURCE, source);
		return execute(XPCEConstants.XPCEADD, obj).succeeded();
	}
	
	public boolean addConstant(Constant c) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.SORT, c.getSort().getName());
		JSONArray names = new JSONArray();
		names.put(c.getName());
		obj.put(XPCEConstants.NAMES, names);
		return execute(XPCEConstants.XPCEADDCONSTANTS, obj).succeeded();
	}
	
	public boolean addConstant(Sort sort, String... names) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.SORT, sort.getName());
		obj.put(XPCEConstants.NAMES, new JSONArray(names));
		return execute(XPCEConstants.XPCEADDCONSTANTS, obj).succeeded();
	}
	
	public boolean addPredicate(PredicateDecl p) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEADDPREDICATE, p.toJSON()).succeeded();
	}
	
	public boolean addSort(Sort sort) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEADDSORT, sort.toJSON()).succeeded();
	}
	public boolean addSort(String sort) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEADDSORT, new Sort(sort).toJSON()).succeeded();
	}
	
	public Map<Formula, Float> ask(Formula formula, float threshold, int numberOfResults) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.FORMULA, formula);
		obj.put(XPCEConstants.THRESHOLD, threshold);
		obj.put(XPCEConstants.RESULTNUMBERS, numberOfResults);
		JSONArray pairs = (JSONArray)execute(XPCEConstants.XPCEASK, obj).getResult();
		Map<Formula, Float> result = new HashMap<Formula, Float>();
		for (int i = 0 ; i < pairs.length() ; i++) {
			JSONObject pair = pairs.getJSONObject(i);
			Formula formul = Formula.createFromJSON(pair.getJSONObject(XPCEConstants.FORMULA));
			Float probability = (float)pair.getDouble(XPCEConstants.PROBABILITY);
			result.put(formul, probability);
		}
		return result;
	}
		
	public boolean assertFact(Fact f) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEASSERT, f.toJSON()).succeeded();
	}
	
	public Object command(String command) throws XmlRpcException, XPCException {
    	return xmlrpcClient.execute(XPCEConstants.XPCECOMMAND, new Object[] {command});
	}
		
	private Result execute(String command, Object... params) throws XmlRpcException, JSONException, XPCException {
		Object[] newParams = new Object[params.length];
		for (int i=0 ; i<params.length ; i++) newParams[i] = params[i].toString(); 
		JSONObject jsonResult = (JSONObject)xmlrpcClient.execute(command, newParams);
		return new Result(jsonResult);	
	}
		
	public boolean load(String filename) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCERETRACT, filename).succeeded();
	}
		
	public boolean mcsat(float sa_probability, float samp_temperature, float rvar_probability, int max_flips, int max_extra_flips, int max_samples) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.SAPROBABILITY, sa_probability);
		obj.put(XPCEConstants.SAMPTEMPERATURE, samp_temperature);
		obj.put(XPCEConstants.RVARPROBABILITY, rvar_probability);
		obj.put(XPCEConstants.MAXFLIPS, max_flips);
		obj.put(XPCEConstants.MAXEXTRAFLIPS, max_extra_flips);
		obj.put(XPCEConstants.MAXSAMPLES, max_samples);
		return execute(XPCEConstants.XPCEMCSAT, obj).succeeded();
	}
		
	public boolean reset(String reset) throws XmlRpcException, XPCException, JSONException {
		if ( !reset.equalsIgnoreCase(XPCEConstants.ALL) && reset.equalsIgnoreCase(XPCEConstants.PROBABILITIES) ) 
			throw new XPCException("Unknown Reset Type");
		return execute(XPCEConstants.XPCERESET, reset.toLowerCase()).succeeded();
	}
	
	public boolean retract(String source) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCERETRACT, source).succeeded();
	}
}
