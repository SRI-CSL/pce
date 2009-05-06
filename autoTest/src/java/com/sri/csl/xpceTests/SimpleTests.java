package com.sri.csl.xpceTests;

import org.junit.*;
import static org.junit.Assert.*;

public class SimpleTests extends AbstractXpceTestFixture {
    @BeforeClass
    public static void oneTimeSetUp() throws Exception {
        loadFile("food1.mcsat");
    }

    @Test
    public void billyYogurt() throws Exception {
        ask("ask likes(billy,yogurt)",0.0,0.3);
    }

    @Test
    public void sandraYogurt() throws Exception {
        ask("ask likes(sandra,yogurt)",0.6,1.0);
    }
}
