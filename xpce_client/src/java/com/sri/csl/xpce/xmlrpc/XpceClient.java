package com.sri.csl.xpce.xmlrpc;

import java.net.MalformedURLException;
import java.net.URL;

import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;
import org.json.JSONException;

import com.sri.csl.exception.XPCException;
import com.sri.csl.xpce.json.XPCEConstants;
import com.sri.csl.xpce.object.Constant;
import com.sri.csl.xpce.object.PredicateDecl;
import com.sri.csl.xpce.object.Sort;

public class XpceClient {
	
	protected URL xpceServerURL;
	protected XmlRpcClient xmlrpcClient = null;
	
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
	
	public Object command(String command) throws XmlRpcException, XPCException {
    	return xmlrpcClient.execute(XPCEConstants.XPCECOMMAND, new Object[] {command});
	}
	
	public void addSort(String sort) throws XmlRpcException, XPCException {
		// TODO: This will later be replaced with a JSON-based command
		command("sort " + sort + ";");
	}
	
	public void addSort(Sort sort) throws XmlRpcException, XPCException, JSONException {
		xmlrpcClient.execute(XPCEConstants.XPCEADDSORT, new Object[] {sort.toJSON()});
	}
	
	public void addSubsort(String sort, String superSort) throws XmlRpcException, XPCException {
		// TODO: This will later be replaced with a JSON-based command
		command("subsort " + sort + " " + superSort + ";");
	}
	
	public void addPredicate(PredicateDecl p) throws XmlRpcException, XPCException, JSONException {
		xmlrpcClient.execute(XPCEConstants.XPCEADDPREDICATE, new Object[] {p.toJSON()});
	}
	
	public void addConstants(Constant c) throws XmlRpcException, XPCException, JSONException {
		xmlrpcClient.execute(XPCEConstants.XPCEADDCONSTANTS, new Object[] {c.toJSON()});
	}
	
	
	
	
	public static void main(String[] args) {
	    XpceClient client = new XpceClient("http://localhost:8080/RPC2");
    	Object ret;
	    try {
	    	client.addSort("S");
	    	client.command("const a, b, c, d, e: S;");
	    	client.command("predicate P(S) indirect;");
	    	ret = client.command("predicate Q(S) indirect;");
	    	System.out.println("Return Value:" + ret);
	    	client.command("add P(a) 2.0;");
	    	client.command("add P(c) 2.0;");
	    	client.command("add P(e) 2.0;");
	    	client.command("add P(b) or P(d) -2.0;");
	    	client.command("add ~P(a) or Q(a) 3.0;");
	    	client.command("add ~P(b) or Q(b) 3.0;");
	    	client.command("add ~P(c) or Q(c) 3.0;");
	    	client.command("add ~P(d) or Q(d) 3.0;");
	    	client.command("add ~P(e) or Q(e) 3.0;");
	    	ret = client.command("mcsat 0.01, 20.0, 0.01, 30, 500;");
	    	System.out.println("Return Value:" + ret);
	    	
	    	//Thread.sleep(10000);
	    	
	    }catch (Exception e) {
	    	e.printStackTrace();
	    }
	}
}
