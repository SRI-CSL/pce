package com.sri.csl.xpceTests;

import com.sri.csl.xpce.xmlrpc.XpceClient;
import com.sri.csl.exception.XPCException;

import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import org.apache.xmlrpc.XmlRpcException;
import static org.junit.Assert.*;
import org.junit.BeforeClass;
import org.json.JSONException;

public abstract class AbstractXpceTestFixture {
    /** Must be the same as xpceServer.port in trunk/autoTest/build.xml */
    private static final int SERVER_PORT = 8080;

    private static final String DATA_DIR = "data";

    private static String RESET_COMMAND = "reset";

    private static XpceClient client;

    /**
     * Load an mcsat file into mcsat, reset any existing data.
     * @param filename relative to the "data" directory
     */
    protected static void initWithFile(String filename) throws XPCException, XmlRpcException, IOException, JSONException {
        XpceClient client = getClient();
        client.command(RESET_COMMAND);

        // TODO: This will not handle statements that are split over several lines.
        File file = new File(DATA_DIR,filename);
        BufferedReader in = new BufferedReader(new FileReader(file));
        try {
            if (!in.ready()) {
                throw new IOException();
            }

            String line;
            while ((line = in.readLine()) != null) {
                line = line.trim();
                if (line.length() > 0) {
                    Object ret = client.command(line);
                }
            }
        }
        finally {
            in.close();
        }

        // Run the mcsat algorithm once.
        getClient().mcsat();
    }

    protected static XpceClient getClient() {
        if (client == null) {
            client = new XpceClient("http://localhost:" + SERVER_PORT + "/RPC2");
        }
        return client;
    }

    protected void ask(String question,double min,double max) throws XPCException, XmlRpcException {
        Object ret = getClient().command(question);
        // TODO: parse ret
        double dummyValue = 0.5;
        assertTrue("Value " + dummyValue + " returned from \"" + question +
                "\" is less than minimum dummyValue of " + min,
                dummyValue >= min);
        assertTrue("Value " + dummyValue + " returned from \"" + question +
                "\" is greater than maximum dummyValue of " + max,
                dummyValue <= max);
    }
}
