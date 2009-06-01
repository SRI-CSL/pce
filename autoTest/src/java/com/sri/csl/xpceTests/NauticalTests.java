package com.sri.csl.xpceTests;

import org.junit.*;

public class NauticalTests extends AbstractXpceTestFixture {
    @BeforeClass
    public static void oneTimeSetUp() throws Exception {
        initWithFile("nautical.mcsat");
    }

    @Test
    public void regattaAt() throws Exception {
        ask("ask regatta_at(r001c072,tuesday)",0.6,1.0);
    }
}