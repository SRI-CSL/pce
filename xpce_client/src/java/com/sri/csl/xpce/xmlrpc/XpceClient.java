package com.sri.csl.xpce.xmlrpc;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;

import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.Logger;
import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;
import com.sri.csl.xpce.json.XPCERespond;
import com.sri.csl.xpce.object.Atom;
import com.sri.csl.xpce.object.Constant;
import com.sri.csl.xpce.object.Fact;
import com.sri.csl.xpce.object.Formula;
import com.sri.csl.xpce.object.FormulaAndProbability;
import com.sri.csl.xpce.object.ImpliesFormula;
import com.sri.csl.xpce.object.NotFormula;
import com.sri.csl.xpce.object.PredicateDecl;
import com.sri.csl.xpce.object.Sort;
import com.sri.csl.xpce.object.Variable;

public class XpceClient {
	
	protected XmlRpcClient xmlrpcClient = null;
	private static final Logger logger = Logger.getLogger(XpceClient.class);
	
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
	
	/** Adds a formula to xpce server.
	 * @param formula	the formula 
	 * @param weight	the weight of the formula
	 * @param source	the name of the client that is adding this formula
	 */
	public boolean add(Formula formula, double weight, String source) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.FORMULA, formula.toJSON());
		obj.put(XPCEConstants.WEIGHT, weight);
		obj.put(XPCEConstants.SOURCE, source);
		return execute(XPCEConstants.XPCEADD, obj).succeeded();
	}
	
	/** Defines a new constant and adds it to xpce server.
	 * @param c		the constant
	 */
	public boolean addConstant(Constant c) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.SORT, c.getSort().getName());
		JSONArray names = new JSONArray();
		names.put(c.getName());
		obj.put(XPCEConstants.NAMES, names);
		return execute(XPCEConstants.XPCEADDCONSTANTS, obj).succeeded();
	}
	
	/** Defines one or more new constants and adds them to xpce server.
	 * @param sort		the sort of the new constant
	 * @param names		the constant(s)
	 */
	public boolean addConstant(Sort sort, String... names) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.SORT, sort.getName());
		obj.put(XPCEConstants.NAMES, new JSONArray(names));
		return execute(XPCEConstants.XPCEADDCONSTANTS, obj).succeeded();
	}
	
	/** Defines a new predicate and adds it to xpce server.
	 * @param p		the predicate
	 */
	public boolean addPredicate(PredicateDecl p) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEADDPREDICATE, p.toJSON()).succeeded();
	}
	
	/** Defines a new sort and adds it to xpce server.
	 * @param sort		the sort
	 */
	public boolean addSort(Sort sort) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEADDSORT, sort.toJSON()).succeeded();
	}

	/** Defines a new sort and adds it to xpce server.
	 * @param sort		the sort
	 */
	public boolean addSort(String sort) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEADDSORT, new Sort(sort).toJSON()).succeeded();
	}
	
	/** Quesries the xpce server for the probability of a given formula.
	 * @param formula		the formula
	 * @param threshold		the threshold for the probability of desired instances of the given formula (the value should be between 0 and 1)
	 * @param maxResults	maximum number of instances to be returned
	 */
	public ArrayList<FormulaAndProbability> ask(Formula formula, double threshold, int maxResults) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.FORMULA, formula.toJSON());
		obj.put(XPCEConstants.THRESHOLD, (double)threshold);
		obj.put(XPCEConstants.RESULTNUMBERS, maxResults);
		JSONArray results = (JSONArray)execute(XPCEConstants.XPCEASK, obj).getResult();
		ArrayList<FormulaAndProbability> faps = new ArrayList<FormulaAndProbability>();
		for (int i = 0 ; i < results.length() ; i++) {
			JSONObject res = results.getJSONObject(i);
			FormulaAndProbability fap = new FormulaAndProbability(formula, res);
			faps.add(fap);
		}
		return faps;
	}
		
	/** Asserts a new fact (an atom with no variable) to xpce server.
	 * @param f		the fact
	 */
	public boolean assertFact(Fact f) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEASSERT, f.toJSON()).succeeded();
	}
	
	/** Passes a mcsat formatted command to xpce server.
	 * @param command		the command in mcsat format (e.g. "sort Person;")
	 */
	public Object command(String command) throws XmlRpcException, XPCException {
    	return xmlrpcClient.execute(XPCEConstants.XPCECOMMAND, new Object[] {command});
	}
		
	private XPCERespond execute(String command, Object... params) throws XmlRpcException, JSONException, XPCException {
		Object[] newParams = new Object[params.length];
		StringBuffer paramStr = new StringBuffer();
		for (int i=0 ; i<params.length ; i++) {
			newParams[i] = params[i].toString();
			paramStr.append(params[i].toString());
		}
		logger.debug("Executing " + command + " - " + paramStr);
		Object xmlRpcResult = xmlrpcClient.execute(command, newParams);
		JSONObject jsonResult = new JSONObject(xmlRpcResult.toString());
		logger.debug("Result: " + jsonResult);
		return new XPCERespond(jsonResult);	
	}
		
	/** Commands xpce server to read all the lines of a text file and load them in.
	 * @param filename		the full path to the file
	 */
	public boolean load(String filename) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCERETRACT, filename).succeeded();
	}

	/** Requests xpce server to compute the probabilities.
	 */
	public boolean mcsat() throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCEMCSAT).succeeded();
	}


	/*	public boolean mcsat(float sa_probability, float samp_temperature, float rvar_probability, int max_flips, int max_extra_flips, int max_samples) throws XmlRpcException, XPCException, JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.SAPROBABILITY, sa_probability);
		obj.put(XPCEConstants.SAMPTEMPERATURE, samp_temperature);
		obj.put(XPCEConstants.RVARPROBABILITY, rvar_probability);
		obj.put(XPCEConstants.MAXFLIPS, max_flips);
		obj.put(XPCEConstants.MAXEXTRAFLIPS, max_extra_flips);
		obj.put(XPCEConstants.MAXSAMPLES, max_samples);
		return execute(XPCEConstants.XPCEMCSAT, obj).succeeded();
	}
*/
	
	/** Resets xpce server.
	 * @param reset		one of "all" or "probabilities"
	 */
	public boolean reset(String reset) throws XmlRpcException, XPCException, JSONException {
		if ( !reset.equalsIgnoreCase(XPCEConstants.ALL) && reset.equalsIgnoreCase(XPCEConstants.PROBABILITIES) ) 
			throw new XPCException("Unknown Reset Type");
		return execute(XPCEConstants.XPCERESET, reset.toLowerCase()).succeeded();
	}
	
	/** Retracts what is added by a given client.
	 * @param source		the client name
	 */
	public boolean retract(String source) throws XmlRpcException, XPCException, JSONException {
		return execute(XPCEConstants.XPCERETRACT, source).succeeded();
	}

	public static void main(String[] args) {
	    XpceClient client = new XpceClient("http://localhost:8080/RPC2");
	    BasicConfigurator.configure();
	    try {
	    	Sort person = new Sort("Person");
	    	client.addSort(person);
	    	client.addConstant(person, "bob", "tom jones", "lisa", "rose");
	    	PredicateDecl pred1 = new PredicateDecl("married", person, person);
	    	PredicateDecl pred2 = new PredicateDecl(false, "likes", person, person);
	    	client.addPredicate(pred1);
	    	client.addPredicate(pred2);
	    	Fact fact1 = new Fact(pred1, "bob", "lisa");
	    	Fact fact2 = new Fact(pred1, "tom jones", "rose");
	    	client.assertFact(fact1);
	    	client.assertFact(fact2);
	    	Atom a1 = (Atom)Formula.create("married($X, $Y)");
	    	Atom a2 = new Atom(pred2, new Variable("X"), new Variable("Y"));
	    	ImpliesFormula f = new ImpliesFormula(a1, a2);
	    	client.add(f, 3.2, "XpceClient");
	    	
	    	client.mcsat();
	    	Thread.sleep(1000);
	    	
	    	Formula question1 = Formula.create("-likes(bob, $Z)");
	    	ArrayList<FormulaAndProbability> answers = client.ask(question1, 0.01, 10);
	    	System.out.println("Return Value:" + answers);
	    	
	    	Formula question2 = Formula.create("likes(bob, 'tom jones') | likes(bob, lisa)");
	    	//Formula question2 = Formula.createFromString("likes('bob', 'tom') & likes('bob', 'lisa')");
	    	//Formula question2 = Formula.createFromString("likes('bob', 'tom') => likes('bob', 'lisa')");
	    	//Formula question2 = Formula.createFromString("likes('bob', 'tom') <=> likes('bob', 'lisa')");
	    	answers = client.ask(new NotFormula(question2), 0.01, 10);
	    	System.out.println("Return Value:" + answers);
	    	
	    }catch (Exception e) {
	    	e.printStackTrace();
	    }
	}
}
