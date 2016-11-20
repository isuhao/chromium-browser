// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.test.util.Feature;
import org.chromium.net.test.util.CertTestUtil;
import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;

/**
 * Public-Key-Pinning tests of Cronet Java API.
 */
public class PkpTest extends CronetTestBase {
    private static final String CERT_USED = "quic_test.example.com.crt";
    private static final String[] CERTS_USED = {CERT_USED};
    private static final int DISTANT_FUTURE = Integer.MAX_VALUE;
    private static final boolean INCLUDE_SUBDOMAINS = true;
    private static final boolean EXCLUDE_SUBDOMAINS = false;

    private CronetTestFramework mTestFramework;
    private CronetEngine.Builder mBuilder;
    private TestUrlRequestCallback mListener;
    private String mServerUrl; // https://test.example.com:6121
    private String mServerHost; // test.example.com
    private String mDomain; // example.com

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // Start QUIC Test Server
        System.loadLibrary("cronet_tests");
        QuicTestServer.startQuicTestServer(getContext());
        mServerUrl = QuicTestServer.getServerURL();
        mServerHost = QuicTestServer.getServerHost();
        mDomain = mServerHost.substring(mServerHost.indexOf('.') + 1, mServerHost.length());
        createCronetEngineBuilder();
    }

    @Override
    protected void tearDown() throws Exception {
        QuicTestServer.shutdownQuicTestServer();
        shutdownCronetEngine();
        super.tearDown();
    }

    /**
     * Tests the case when the pin hash does not match. The client is expected to
     * receive the error response.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testErrorCodeIfPinDoesNotMatch() throws Exception {
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256(mServerHost, nonMatchingHash, EXCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertErrorResponse();
    }

    /**
     * Tests the case when the pin hash matches. The client is expected to
     * receive the successful response with the response code 200.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testSuccessIfPinMatches() throws Exception {
        // Get PKP hash of the real certificate
        X509Certificate cert = readCertFromFileInPemFormat(CERT_USED);
        byte[] matchingHash = CertTestUtil.getPublicKeySha256(cert);

        addPkpSha256(mServerHost, matchingHash, EXCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertSuccessfulResponse();
    }

    /**
     * Tests the case when the pin hash does not match and the client accesses the subdomain of
     * the configured PKP host with includeSubdomains flag set to true. The client is
     * expected to receive the error response.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testIncludeSubdomainsFlagEqualTrue() throws Exception {
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256(mDomain, nonMatchingHash, INCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertErrorResponse();
    }

    /**
     * Tests the case when the pin hash does not match and the client accesses the subdomain of
     * the configured PKP host with includeSubdomains flag set to false. The client is expected to
     * receive the successful response with the response code 200.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testIncludeSubdomainsFlagEqualFalse() throws Exception {
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256(mDomain, nonMatchingHash, EXCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertSuccessfulResponse();
    }

    /**
     * Tests the case when the mismatching pin is set for some host that is different from the one
     * the client wants to access. In that case the other host pinning policy should not be applied
     * and the client is expected to receive the successful response with the response code 200.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testSuccessIfNoPinSpecified() throws Exception {
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256("otherhost.com", nonMatchingHash, INCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertSuccessfulResponse();
    }

    /**
     * Tests mismatching pins that will expire in 10 seconds. The pins should be still valid and
     * enforced during the request; thus returning PIN mismatch error.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testSoonExpiringPin() throws Exception {
        final int tenSecondsAhead = 10;
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256(mServerHost, nonMatchingHash, EXCLUDE_SUBDOMAINS, tenSecondsAhead);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertErrorResponse();
    }

    /**
     * Tests mismatching pins that expired 1 second ago. Since the pins have expired, they
     * should not be enforced during the request; thus a successful response is expected.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testRecentlyExpiredPin() throws Exception {
        final int oneSecondAgo = -1;
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256(mServerHost, nonMatchingHash, EXCLUDE_SUBDOMAINS, oneSecondAgo);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();

        assertSuccessfulResponse();
    }

    /**
     * Tests that host pinning is not persisted between multiple CronetEngine instances.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testPinsAreNotPersisted() throws Exception {
        byte[] nonMatchingHash = generateSomeSha256();
        addPkpSha256(mServerHost, nonMatchingHash, EXCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();
        assertErrorResponse();
        shutdownCronetEngine();

        // Restart Cronet engine and try the same request again. Since the pins are not persisted,
        // a successful response is expected.
        createCronetEngineBuilder();
        startCronetFramework();
        registerHostResolver(mTestFramework);
        sendRequestAndWaitForResult();
        assertSuccessfulResponse();
    }

    /**
     * Tests that the client receives {@code InvalidArgumentException} when the pinned host name
     * is invalid.
     *
     * @throws Exception
     */
    @SmallTest
    @Feature({"Cronet"})
    public void testHostNameArgumentValidation() throws Exception {
        final String label63 = "123456789-123456789-123456789-123456789-123456789-123456789-123";
        final String host255 = label63 + "." + label63 + "." + label63 + "." + label63;
        // Valid host names.
        assertNoExceptionWhenHostNameIsValid("domain.com");
        assertNoExceptionWhenHostNameIsValid("my-domain.com");
        assertNoExceptionWhenHostNameIsValid("section4.domain.info");
        assertNoExceptionWhenHostNameIsValid("44.domain44.info");
        assertNoExceptionWhenHostNameIsValid("very.long.long.long.long.long.long.long.domain.com");
        assertNoExceptionWhenHostNameIsValid("host");
        assertNoExceptionWhenHostNameIsValid("новости.ру");
        assertNoExceptionWhenHostNameIsValid("самые-последние.новости.рус");
        assertNoExceptionWhenHostNameIsValid("最新消息.中国");
        // Checks max size of the host label (63 characters)
        assertNoExceptionWhenHostNameIsValid(label63 + ".com");
        // Checks max size of the host name (255 characters)
        assertNoExceptionWhenHostNameIsValid(host255);
        assertNoExceptionWhenHostNameIsValid("127.0.0.z");

        // Invalid host names.
        assertExceptionWhenHostNameIsInvalid("domain.com:300");
        assertExceptionWhenHostNameIsInvalid("-domain.com");
        assertExceptionWhenHostNameIsInvalid("domain-.com");
        assertExceptionWhenHostNameIsInvalid("http://domain.com");
        assertExceptionWhenHostNameIsInvalid("domain.com:");
        assertExceptionWhenHostNameIsInvalid("domain.com/");
        assertExceptionWhenHostNameIsInvalid("новости.ру:");
        assertExceptionWhenHostNameIsInvalid("новости.ру/");
        assertExceptionWhenHostNameIsInvalid("_http.sctp.www.example.com");
        assertExceptionWhenHostNameIsInvalid("http.sctp._www.example.com");
        // Checks a host that exceeds max allowed length of the host label (63 characters)
        assertExceptionWhenHostNameIsInvalid(label63 + "4.com");
        // Checks a host that exceeds max allowed length of hostname (255 characters)
        assertExceptionWhenHostNameIsInvalid(host255.substring(3) + ".com");
        assertExceptionWhenHostNameIsInvalid("FE80:0000:0000:0000:0202:B3FF:FE1E:8329");
        assertExceptionWhenHostNameIsInvalid("[2001:db8:0:1]:80");

        // Invalid host names for PKP that contain IPv4 addresses
        // or names with digits and dots only.
        assertExceptionWhenHostNameIsInvalid("127.0.0.1");
        assertExceptionWhenHostNameIsInvalid("68.44.222.12");
        assertExceptionWhenHostNameIsInvalid("256.0.0.1");
        assertExceptionWhenHostNameIsInvalid("127.0.0.1.1");
        assertExceptionWhenHostNameIsInvalid("127.0.0");
        assertExceptionWhenHostNameIsInvalid("127.0.0.");
        assertExceptionWhenHostNameIsInvalid("127.0.0.299");
    }

    /**
     * Tests that NullPointerException is thrown if the host name or the collection of pins or
     * the expiration date is null.
     */
    @SmallTest
    @Feature({"Cronet"})
    public void testNullArguments() {
        verifyExceptionWhenAddPkpArgumentIsNull(true, false, false);
        verifyExceptionWhenAddPkpArgumentIsNull(false, true, false);
        verifyExceptionWhenAddPkpArgumentIsNull(false, false, true);
        verifyExceptionWhenAddPkpArgumentIsNull(false, false, false);
    }

    /**
     * Tests that IllegalArgumentException is thrown if SHA1 is passed as the value of a pin.
     */
    @SmallTest
    @Feature({"Cronet"})
    public void testIllegalArgumentExceptionWhenPinValueIsSHA1() {
        byte[] sha1 = new byte[20];
        try {
            addPkpSha256(mServerHost, sha1, EXCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        } catch (IllegalArgumentException ex) {
            // Expected exception
            return;
        }
        fail("Expected IllegalArgumentException with pin value: " + Arrays.toString(sha1));
    }

    /**
     * Asserts that the response from the server contains an PKP error.
     * TODO(kapishnikov): currently QUIC returns ERR_QUIC_PROTOCOL_ERROR instead of expected
     * ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN error code when the pin doesn't match.
     * This method should be changed when the bug is resolved.
     * See http://crbug.com/548378
     * See http://crbug.com/568669
     */
    private void assertErrorResponse() {
        assertNotNull("Expected an error", mListener.mError);
        int errorCode = mListener.mError.getCronetInternalErrorCode();
        Set<Integer> expectedErrors = new HashSet<>();
        expectedErrors.add(NetError.ERR_QUIC_PROTOCOL_ERROR);
        expectedErrors.add(NetError.ERR_CONNECTION_REFUSED);
        expectedErrors.add(NetError.ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN);
        assertTrue(String.format("Incorrect error code. Expected one of %s but received %s",
                           expectedErrors, errorCode),
                expectedErrors.contains(errorCode));
    }

    /**
     * Asserts a successful response with response code 200.
     */
    private void assertSuccessfulResponse() {
        if (mListener.mError != null) {
            fail("Did not expect an error but got error code "
                    + mListener.mError.getCronetInternalErrorCode());
        }
        assertNotNull("Expected non-null response from the server", mListener.mResponseInfo);
        assertEquals(200, mListener.mResponseInfo.getHttpStatusCode());
    }

    private void createCronetEngineBuilder() throws Exception {
        // Set common CronetEngine parameters
        mBuilder = new CronetEngine.Builder(getContext());
        mBuilder.enableQUIC(true);
        mBuilder.addQuicHint(QuicTestServer.getServerHost(), QuicTestServer.getServerPort(),
                QuicTestServer.getServerPort());
        JSONObject quicParams = new JSONObject().put("host_whitelist", "test.example.com");
        JSONObject experimentalOptions = new JSONObject().put("QUIC", quicParams);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        mBuilder.setStoragePath(CronetTestFramework.getTestStorage(getContext()));
        mBuilder.enableHttpCache(CronetEngine.Builder.HTTP_CACHE_DISK_NO_HTTP, 1000 * 1024);
        mBuilder.setMockCertVerifierForTesting(MockCertVerifier.createMockCertVerifier(CERTS_USED));
    }

    private void startCronetFramework() {
        mTestFramework = startCronetTestFrameworkWithUrlAndCronetEngineBuilder(null, mBuilder);
    }

    private void shutdownCronetEngine() {
        if (mTestFramework != null && mTestFramework.mCronetEngine != null) {
            mTestFramework.mCronetEngine.shutdown();
        }
    }

    private byte[] generateSomeSha256() {
        byte[] sha256 = new byte[32];
        Arrays.fill(sha256, (byte) 58);
        return sha256;
    }

    private void addPkpSha256(
            String host, byte[] pinHashValue, boolean includeSubdomain, int maxAgeInSec) {
        Set<byte[]> hashes = new HashSet<>();
        hashes.add(pinHashValue);
        mBuilder.addPublicKeyPins(host, hashes, includeSubdomain, dateInFuture(maxAgeInSec));
    }

    private void sendRequestAndWaitForResult() {
        mListener = new TestUrlRequestCallback();

        String quicURL = mServerUrl + "/simple.txt";
        UrlRequest.Builder requestBuilder = new UrlRequest.Builder(
                quicURL, mListener, mListener.getExecutor(), mTestFramework.mCronetEngine);
        requestBuilder.build().start();
        mListener.blockForDone();
    }

    private X509Certificate readCertFromFileInPemFormat(String certFileName) throws Exception {
        byte[] certDer = CertTestUtil.pemToDer(CertTestUtil.CERTS_DIRECTORY + certFileName);
        CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        return (X509Certificate) certFactory.generateCertificate(new ByteArrayInputStream(certDer));
    }

    private Date dateInFuture(int secondsIntoFuture) {
        Calendar cal = Calendar.getInstance();
        cal.add(Calendar.SECOND, secondsIntoFuture);
        return cal.getTime();
    }

    private void assertNoExceptionWhenHostNameIsValid(String hostName) {
        try {
            addPkpSha256(hostName, generateSomeSha256(), INCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        } catch (IllegalArgumentException ex) {
            fail("Host name " + hostName + " should be valid but the exception was thrown: "
                    + ex.toString());
        }
    }

    private void assertExceptionWhenHostNameIsInvalid(String hostName) {
        try {
            addPkpSha256(hostName, generateSomeSha256(), INCLUDE_SUBDOMAINS, DISTANT_FUTURE);
        } catch (IllegalArgumentException ex) {
            // Expected exception.
            return;
        }
        fail("Expected IllegalArgumentException when passing " + hostName + " host name");
    }

    private void verifyExceptionWhenAddPkpArgumentIsNull(
            boolean hostNameIsNull, boolean pinsAreNull, boolean expirationDataIsNull) {
        String hostName = hostNameIsNull ? null : "some-host.com";
        Set<byte[]> pins = pinsAreNull ? null : new HashSet<byte[]>();
        Date expirationDate = expirationDataIsNull ? null : new Date();

        boolean shouldThrowNpe = hostNameIsNull || pinsAreNull || expirationDataIsNull;
        try {
            mBuilder.addPublicKeyPins(hostName, pins, INCLUDE_SUBDOMAINS, expirationDate);
        } catch (NullPointerException ex) {
            if (!shouldThrowNpe) {
                fail("Null pointer exception was not expected: " + ex.toString());
            }
            return;
        }
        if (shouldThrowNpe) {
            fail("NullPointerException was expected");
        }
    }
}
