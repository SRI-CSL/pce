package com.sri.csl.xpceTests;

import org.junit.*;
import static org.junit.Assert.*;
import java.util.*;

public class SimpleTests {
    @BeforeClass
    public static void oneTimeSetUp() {
		System.out.println("setup");
    }

    @AfterClass
    public static void oneTimeTearDown() {
		System.out.println("teardown");
    }

    @Test
    public void testOk() {
        assertTrue("foo" == "foo");
    }

    @Test
    public void testFail() {
        assertEquals(1, 3);
    }
}
