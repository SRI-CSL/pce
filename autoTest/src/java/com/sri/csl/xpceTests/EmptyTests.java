package com.sri.csl.xpceTests;

import org.junit.BeforeClass;
import org.junit.Test;

/**
 * User: hardt
 * Date: Jun 1, 2009
 */
public class EmptyTests extends AbstractXpceTestFixture {
    @BeforeClass
    public static void oneTimeSetUp() throws Exception {
        initWithFile("empty.mcsat");
    }

    @Test
    public void noInformation() throws Exception {
        ask("ask dummy(a,c)",0.4,0.6);
        ask("ask dummy(b,d)",0.4,0.6);
    }
}
