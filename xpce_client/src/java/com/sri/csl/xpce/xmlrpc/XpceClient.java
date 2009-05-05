package com.sri.csl.xpce.xmlrpc;

import java.net.MalformedURLException;
import java.net.URL;

import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;

public class XpceClient {
	protected String xpceServerURL;
	
	public void setServerURL(String url) {
		xpceServerURL = url;
	}
	
	public String getServerURL() {
		return xpceServerURL;
	}
	
	public XmlRpcClient getXmlRpcClient() {
	    XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();
	    try {
	    	config.setServerURL(new URL(xpceServerURL));
	    } catch (MalformedURLException m) {
	    	m.printStackTrace();
	    	return null;
	    }
	    XmlRpcClient client = new XmlRpcClient();
	    client.setConfig(config);
		return client;
	}
	
	public Object command(String command) throws XmlRpcException {
    	return getXmlRpcClient().execute("xpce.command", new Object[] {command});
	}
	
	public static void main(String[] args) {
	    XpceClient client = new XpceClient();
    	client.setServerURL("http://localhost:8080/RPC2");
    	Object ret;
	    try {
	    	ret = client.command("sort S;");
	    	System.out.println("Return Value:" + ret);
	    	client.command("const a, b, c, d, e: S;");
	    	client.command("predicate P(S) indirect;");
	    	client.command("predicate Q(S) indirect;");
	    	client.command("add P(a) 2.0;");
	    	client.command("add P(c) 2.0;");
	    	client.command("add P(e) 2.0;");
	    	client.command("add P(b) or P(d) -2.0;");
	    	client.command("add ~P(a) or Q(a) 3.0;");
	    	client.command("add ~P(b) or Q(b) 3.0;");
	    	client.command("add ~P(c) or Q(c) 3.0;");
	    	client.command("add ~P(d) or Q(d) 3.0;");
	    	client.command("add ~P(e) or Q(e) 3.0;");
	    	client.command("mcsat 0.01, 20.0, 0.01, 30, 500;");
	    }catch (XmlRpcException e) {
	    	e.printStackTrace();
	    }
	}
}
