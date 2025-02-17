/*
 * This file contains prototypes for the public SSL functions.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ssl_h_
#define __ssl_h_

#include "prtypes.h"
#include "prerror.h"
#include "prio.h"
#include "seccomon.h"
#include "cert.h"
#include "keyt.h"

#include "sslt.h"  /* public ssl data types */

#if defined(_WIN32) && !defined(IN_LIBSSL) && !defined(NSS_USE_STATIC_LIBS)
#define SSL_IMPORT extern __declspec(dllimport)
#else
#define SSL_IMPORT extern
#endif

SEC_BEGIN_PROTOS

/* constant table enumerating all implemented SSL 2 and 3 cipher suites. */
SSL_IMPORT const PRUint16 SSL_ImplementedCiphers[];

/* the same as the above, but is a function */
SSL_IMPORT const PRUint16 *SSL_GetImplementedCiphers(void);

/* number of entries in the above table. */
SSL_IMPORT const PRUint16 SSL_NumImplementedCiphers;

/* the same as the above, but is a function */
SSL_IMPORT PRUint16 SSL_GetNumImplementedCiphers(void);

/* Macro to tell which ciphers in table are SSL2 vs SSL3/TLS. */
#define SSL_IS_SSL2_CIPHER(which) (((which) & 0xfff0) == 0xff00)

/*
** Imports fd into SSL, returning a new socket.  Copies SSL configuration
** from model.
*/
SSL_IMPORT PRFileDesc *SSL_ImportFD(PRFileDesc *model, PRFileDesc *fd);

/*
** Imports fd into DTLS, returning a new socket.  Copies DTLS configuration
** from model.
*/
SSL_IMPORT PRFileDesc *DTLS_ImportFD(PRFileDesc *model, PRFileDesc *fd);

/*
** Enable/disable an ssl mode
**
** 	SSL_SECURITY:
** 		enable/disable use of SSL security protocol before connect
**
** 	SSL_SOCKS:
** 		enable/disable use of socks before connect
**		(No longer supported).
**
** 	SSL_REQUEST_CERTIFICATE:
** 		require a certificate during secure connect
*/
/* options */
#define SSL_SECURITY			1 /* (on by default) */
#define SSL_SOCKS			2 /* (off by default) */
#define SSL_REQUEST_CERTIFICATE		3 /* (off by default) */
#define SSL_HANDSHAKE_AS_CLIENT		5 /* force accept to hs as client */
                               		  /* (off by default) */
#define SSL_HANDSHAKE_AS_SERVER		6 /* force connect to hs as server */
                               		  /* (off by default) */

/* OBSOLETE: SSL v2 is obsolete and may be removed soon. */
#define SSL_ENABLE_SSL2			7 /* enable ssl v2 (off by default) */

/* OBSOLETE: See "SSL Version Range API" below for the replacement and a
** description of the non-obvious semantics of using SSL_ENABLE_SSL3.
*/
#define SSL_ENABLE_SSL3		        8 /* enable ssl v3 (on by default) */

#define SSL_NO_CACHE		        9 /* don't use the session cache */
                    		          /* (off by default) */
#define SSL_REQUIRE_CERTIFICATE        10 /* (SSL_REQUIRE_FIRST_HANDSHAKE */
                                          /* by default) */
#define SSL_ENABLE_FDX                 11 /* permit simultaneous read/write */
                                          /* (off by default) */

/* OBSOLETE: SSL v2 compatible hellos are not accepted by some TLS servers
** and cannot negotiate extensions. SSL v2 is obsolete. This option may be
** removed soon.
*/
#define SSL_V2_COMPATIBLE_HELLO        12 /* send v3 client hello in v2 fmt */
                                          /* (off by default) */

/* OBSOLETE: See "SSL Version Range API" below for the replacement and a
** description of the non-obvious semantics of using SSL_ENABLE_TLS.
*/
#define SSL_ENABLE_TLS		       13 /* enable TLS (on by default) */

#define SSL_ROLLBACK_DETECTION         14 /* for compatibility, default: on */
#define SSL_NO_STEP_DOWN               15 /* Disable export cipher suites   */
                                          /* if step-down keys are needed.  */
					  /* default: off, generate         */
					  /* step-down keys if needed.      */
#define SSL_BYPASS_PKCS11              16 /* use PKCS#11 for pub key only   */
#define SSL_NO_LOCKS                   17 /* Don't use locks for protection */
#define SSL_ENABLE_SESSION_TICKETS     18 /* Enable TLS SessionTicket       */
                                          /* extension (off by default)     */
#define SSL_ENABLE_DEFLATE             19 /* Enable TLS compression with    */
                                          /* DEFLATE (off by default)       */
#define SSL_ENABLE_RENEGOTIATION       20 /* Values below (default: never)  */
#define SSL_REQUIRE_SAFE_NEGOTIATION   21 /* Peer must send Signaling       */
					  /* Cipher Suite Value (SCSV) or   */
                                          /* Renegotiation  Info (RI)       */
					  /* extension in ALL handshakes.   */
                                          /* default: off                   */
#define SSL_ENABLE_FALSE_START         22 /* Enable SSL false start (off by */
                                          /* default, applies only to       */
                                          /* clients). False start is a     */
/* mode where an SSL client will start sending application data before
 * verifying the server's Finished message. This means that we could end up
 * sending data to an imposter. However, the data will be encrypted and
 * only the true server can derive the session key. Thus, so long as the
 * cipher isn't broken this is safe. The advantage of false start is that
 * it saves a round trip for client-speaks-first protocols when performing a
 * full handshake.
 *
 * In addition to enabling this option, the application must register a
 * callback using the SSL_SetCanFalseStartCallback function.
 */

/* For SSL 3.0 and TLS 1.0, by default we prevent chosen plaintext attacks
 * on SSL CBC mode cipher suites (see RFC 4346 Section F.3) by splitting
 * non-empty application_data records into two records; the first record has
 * only the first byte of plaintext, and the second has the rest.
 *
 * This only prevents the attack in the sending direction; the connection may
 * still be vulnerable to such attacks if the peer does not implement a similar
 * countermeasure.
 *
 * This protection mechanism is on by default; the default can be overridden by
 * setting NSS_SSL_CBC_RANDOM_IV=0 in the environment prior to execution,
 * and/or by the application setting the option SSL_CBC_RANDOM_IV to PR_FALSE.
 *
 * The per-record IV in TLS 1.1 and later adds one block of overhead per
 * record, whereas this hack will add at least two blocks of overhead per
 * record, so TLS 1.1+ will always be more efficient.
 *
 * Other implementations (e.g. some versions of OpenSSL, in some
 * configurations) prevent the same attack by prepending an empty
 * application_data record to every application_data record they send; we do
 * not do that because some implementations cannot handle empty
 * application_data records. Also, we only split application_data records and
 * not other types of records, because some implementations will not accept
 * fragmented records of some other types (e.g. some versions of NSS do not
 * accept fragmented alerts).
 */
#define SSL_CBC_RANDOM_IV 23
#define SSL_ENABLE_OCSP_STAPLING       24 /* Request OCSP stapling (client) */

/* SSL_ENABLE_NPN controls whether the NPN extension is enabled for the initial
 * handshake when protocol negotiation is used. SSL_SetNextProtoCallback
 * or SSL_SetNextProtoNego must be used to control the protocol negotiation;
 * otherwise, the NPN extension will not be negotiated. SSL_ENABLE_NPN is
 * currently enabled by default but this may change in future versions.
 */
#define SSL_ENABLE_NPN 25

/* SSL_ENABLE_ALPN controls whether the ALPN extension is enabled for the
 * initial handshake when protocol negotiation is used. SSL_SetNextProtoNego
 * (not SSL_SetNextProtoCallback) must be used to control the protocol
 * negotiation; otherwise, the ALPN extension will not be negotiated. ALPN is
 * not negotiated for renegotiation handshakes, even though the ALPN
 * specification defines a way to use ALPN during renegotiations.
 * SSL_ENABLE_ALPN is currently disabled by default, but this may change in
 * future versions.
 */
#define SSL_ENABLE_ALPN 26

/* Request Signed Certificate Timestamps via TLS extension (client) */
#define SSL_ENABLE_SIGNED_CERT_TIMESTAMPS 27
#define SSL_ENABLE_FALLBACK_SCSV       28 /* Send fallback SCSV in
                                           * handshakes. */

#ifdef SSL_DEPRECATED_FUNCTION 
/* Old deprecated function names */
SSL_IMPORT SECStatus SSL_Enable(PRFileDesc *fd, int option, PRBool on);
SSL_IMPORT SECStatus SSL_EnableDefault(int option, PRBool on);
#endif

/* New function names */
SSL_IMPORT SECStatus SSL_OptionSet(PRFileDesc *fd, PRInt32 option, PRBool on);
SSL_IMPORT SECStatus SSL_OptionGet(PRFileDesc *fd, PRInt32 option, PRBool *on);
SSL_IMPORT SECStatus SSL_OptionSetDefault(PRInt32 option, PRBool on);
SSL_IMPORT SECStatus SSL_OptionGetDefault(PRInt32 option, PRBool *on);
SSL_IMPORT SECStatus SSL_CertDBHandleSet(PRFileDesc *fd, CERTCertDBHandle *dbHandle);

/* SSLNextProtoCallback is called during the handshake for the client, when a
 * Next Protocol Negotiation (NPN) extension has been received from the server.
 * |protos| and |protosLen| define a buffer which contains the server's
 * advertisement. This data is guaranteed to be well formed per the NPN spec.
 * |protoOut| is a buffer provided by the caller, of length 255 (the maximum
 * allowed by the protocol). On successful return, the protocol to be announced
 * to the server will be in |protoOut| and its length in |*protoOutLen|.
 *
 * The callback must return SECFailure or SECSuccess (not SECWouldBlock).
 */
typedef SECStatus (PR_CALLBACK *SSLNextProtoCallback)(
    void *arg,
    PRFileDesc *fd,
    const unsigned char* protos,
    unsigned int protosLen,
    unsigned char* protoOut,
    unsigned int* protoOutLen,
    unsigned int protoMaxOut);

/* SSL_SetNextProtoCallback sets a callback function to handle Next Protocol
 * Negotiation. It causes a client to advertise NPN. */
SSL_IMPORT SECStatus SSL_SetNextProtoCallback(PRFileDesc *fd,
                                              SSLNextProtoCallback callback,
                                              void *arg);

/* SSL_SetNextProtoNego can be used as an alternative to
 * SSL_SetNextProtoCallback. It also causes a client to advertise NPN and
 * installs a default callback function which selects the first supported
 * protocol in server-preference order. If no matching protocol is found it
 * selects the first supported protocol.
 *
 * Using this function also allows the client to transparently support ALPN.
 * The same set of protocols will be advertised via ALPN and, if the server
 * uses ALPN to select a protocol, SSL_GetNextProto will return
 * SSL_NEXT_PROTO_SELECTED as the state.
 *
 * Since NPN uses the first protocol as the fallback protocol, when sending an
 * ALPN extension, the first protocol is moved to the end of the list. This
 * indicates that the fallback protocol is the least preferred. The other
 * protocols should be in preference order.
 *
 * The supported protocols are specified in |data| in wire-format (8-bit
 * length-prefixed). For example: "\010http/1.1\006spdy/2". */
SSL_IMPORT SECStatus SSL_SetNextProtoNego(PRFileDesc *fd,
					  const unsigned char *data,
					  unsigned int length);

typedef enum SSLNextProtoState { 
  SSL_NEXT_PROTO_NO_SUPPORT = 0, /* No peer support                */
  SSL_NEXT_PROTO_NEGOTIATED = 1, /* Mutual agreement               */
  SSL_NEXT_PROTO_NO_OVERLAP = 2, /* No protocol overlap found      */
  SSL_NEXT_PROTO_SELECTED   = 3  /* Server selected proto (ALPN)   */
} SSLNextProtoState;

/* SSL_GetNextProto can be used in the HandshakeCallback or any time after
 * a handshake to retrieve the result of the Next Protocol negotiation.
 *
 * The length of the negotiated protocol, if any, is written into *bufLen.
 * If the negotiated protocol is longer than bufLenMax, then SECFailure is
 * returned. Otherwise, the negotiated protocol, if any, is written into buf,
 * and SECSuccess is returned. */
SSL_IMPORT SECStatus SSL_GetNextProto(PRFileDesc *fd,
				      SSLNextProtoState *state,
				      unsigned char *buf,
				      unsigned int *bufLen,
				      unsigned int bufLenMax);

/*
** Control ciphers that SSL uses. If on is non-zero then the named cipher
** is enabled, otherwise it is disabled. 
** The "cipher" values are defined in sslproto.h (the SSL_EN_* values).
** EnableCipher records user preferences.
** SetPolicy sets the policy according to the policy module.
*/
#ifdef SSL_DEPRECATED_FUNCTION 
/* Old deprecated function names */
SSL_IMPORT SECStatus SSL_EnableCipher(long which, PRBool enabled);
SSL_IMPORT SECStatus SSL_SetPolicy(long which, int policy);
#endif

/* New function names */
SSL_IMPORT SECStatus SSL_CipherPrefSet(PRFileDesc *fd, PRInt32 cipher, PRBool enabled);
SSL_IMPORT SECStatus SSL_CipherPrefGet(PRFileDesc *fd, PRInt32 cipher, PRBool *enabled);
SSL_IMPORT SECStatus SSL_CipherPrefSetDefault(PRInt32 cipher, PRBool enabled);
SSL_IMPORT SECStatus SSL_CipherPrefGetDefault(PRInt32 cipher, PRBool *enabled);
SSL_IMPORT SECStatus SSL_CipherPolicySet(PRInt32 cipher, PRInt32 policy);
SSL_IMPORT SECStatus SSL_CipherPolicyGet(PRInt32 cipher, PRInt32 *policy);

/* SSL_CipherOrderSet sets the cipher suite preference order from |ciphers|,
 * which must be an array of cipher suite ids of length |len|. All the given
 * cipher suite ids must appear in the array that is returned by
 * |SSL_GetImplementedCiphers| and may only appear once, at most. */
SSL_IMPORT SECStatus SSL_CipherOrderSet(PRFileDesc *fd, const PRUint16 *ciphers,
                                        unsigned int len);

/* SSLChannelBindingType enumerates the types of supported channel binding
 * values. See RFC 5929. */
typedef enum SSLChannelBindingType {
    SSL_CHANNEL_BINDING_TLS_UNIQUE = 1,
} SSLChannelBindingType;

/* SSL_GetChannelBinding copies the requested channel binding value, as defined
 * in RFC 5929, into |out|. The full length of the binding value is written
 * into |*outLen|.
 *
 * At most |outLenMax| bytes of data are copied. If |outLenMax| is
 * insufficient then the function returns SECFailure and sets the error to
 * SEC_ERROR_OUTPUT_LEN, but |*outLen| is still set.
 *
 * This call will fail if made during a renegotiation. */
SSL_IMPORT SECStatus SSL_GetChannelBinding(PRFileDesc *fd,
					   SSLChannelBindingType binding_type,
					   unsigned char *out,
					   unsigned int *outLen,
					   unsigned int outLenMax);

/* SSL Version Range API
**
** This API should be used to control SSL 3.0 & TLS support instead of the
** older SSL_Option* API; however, the SSL_Option* API MUST still be used to
** control SSL 2.0 support. In this version of libssl, SSL 3.0 and TLS 1.0 are
** enabled by default. Future versions of libssl may change which versions of
** the protocol are enabled by default.
**
** The SSLProtocolVariant enum indicates whether the protocol is of type
** stream or datagram. This must be provided to the functions that do not
** take an fd. Functions which take an fd will get the variant from the fd,
** which is typed.
**
** Using the new version range API in conjunction with the older
** SSL_OptionSet-based API for controlling the enabled protocol versions may
** cause unexpected results. Going forward, we guarantee only the following:
**
** SSL_OptionGet(SSL_ENABLE_TLS) will return PR_TRUE if *ANY* versions of TLS
** are enabled.
**
** SSL_OptionSet(SSL_ENABLE_TLS, PR_FALSE) will disable *ALL* versions of TLS,
** including TLS 1.0 and later.
**
** The above two properties provide compatibility for applications that use
** SSL_OptionSet to implement the insecure fallback from TLS 1.x to SSL 3.0.
**
** SSL_OptionSet(SSL_ENABLE_TLS, PR_TRUE) will enable TLS 1.0, and may also
** enable some later versions of TLS, if it is necessary to do so in order to
** keep the set of enabled versions contiguous. For example, if TLS 1.2 is
** enabled, then after SSL_OptionSet(SSL_ENABLE_TLS, PR_TRUE), TLS 1.0,
** TLS 1.1, and TLS 1.2 will be enabled, and the call will have no effect on
** whether SSL 3.0 is enabled. If no later versions of TLS are enabled at the
** time SSL_OptionSet(SSL_ENABLE_TLS, PR_TRUE) is called, then no later
** versions of TLS will be enabled by the call.
**
** SSL_OptionSet(SSL_ENABLE_SSL3, PR_FALSE) will disable SSL 3.0, and will not
** change the set of TLS versions that are enabled.
**
** SSL_OptionSet(SSL_ENABLE_SSL3, PR_TRUE) will enable SSL 3.0, and may also
** enable some versions of TLS if TLS 1.1 or later is enabled at the time of
** the call, the same way SSL_OptionSet(SSL_ENABLE_TLS, PR_TRUE) works, in
** order to keep the set of enabled versions contiguous.
*/

/* Returns, in |*vrange|, the range of SSL3/TLS versions supported for the
** given protocol variant by the version of libssl linked-to at runtime.
*/
SSL_IMPORT SECStatus SSL_VersionRangeGetSupported(
    SSLProtocolVariant protocolVariant, SSLVersionRange *vrange);

/* Returns, in |*vrange|, the range of SSL3/TLS versions enabled by default
** for the given protocol variant.
*/
SSL_IMPORT SECStatus SSL_VersionRangeGetDefault(
    SSLProtocolVariant protocolVariant, SSLVersionRange *vrange);

/* Sets the range of enabled-by-default SSL3/TLS versions for the given
** protocol variant to |*vrange|.
*/
SSL_IMPORT SECStatus SSL_VersionRangeSetDefault(
    SSLProtocolVariant protocolVariant, const SSLVersionRange *vrange);

/* Returns, in |*vrange|, the range of enabled SSL3/TLS versions for |fd|. */
SSL_IMPORT SECStatus SSL_VersionRangeGet(PRFileDesc *fd,
					 SSLVersionRange *vrange);

/* Sets the range of enabled SSL3/TLS versions for |fd| to |*vrange|. */
SSL_IMPORT SECStatus SSL_VersionRangeSet(PRFileDesc *fd,
					 const SSLVersionRange *vrange);


/* Values for "policy" argument to SSL_CipherPolicySet */
/* Values returned by SSL_CipherPolicyGet. */
#define SSL_NOT_ALLOWED		 0	      /* or invalid or unimplemented */
#define SSL_ALLOWED		 1
#define SSL_RESTRICTED		 2	      /* only with "Step-Up" certs. */

/* Values for "on" with SSL_REQUIRE_CERTIFICATE. */
#define SSL_REQUIRE_NEVER           ((PRBool)0)
#define SSL_REQUIRE_ALWAYS          ((PRBool)1)
#define SSL_REQUIRE_FIRST_HANDSHAKE ((PRBool)2)
#define SSL_REQUIRE_NO_ERROR        ((PRBool)3)

/* Values for "on" with SSL_ENABLE_RENEGOTIATION */
/* Never renegotiate at all.                                               */
#define SSL_RENEGOTIATE_NEVER        ((PRBool)0)
/* Renegotiate without restriction, whether or not the peer's client hello */
/* bears the renegotiation info extension.  Vulnerable, as in the past.    */
#define SSL_RENEGOTIATE_UNRESTRICTED ((PRBool)1)
/* Only renegotiate if the peer's hello bears the TLS renegotiation_info   */
/* extension. This is safe renegotiation.                                  */
#define SSL_RENEGOTIATE_REQUIRES_XTN ((PRBool)2) 
/* Disallow unsafe renegotiation in server sockets only, but allow clients */
/* to continue to renegotiate with vulnerable servers.                     */
/* This value should only be used during the transition period when few    */
/* servers have been upgraded.                                             */
#define SSL_RENEGOTIATE_TRANSITIONAL ((PRBool)3)

/*
** Reset the handshake state for fd. This will make the complete SSL
** handshake protocol execute from the ground up on the next i/o
** operation.
*/
SSL_IMPORT SECStatus SSL_ResetHandshake(PRFileDesc *fd, PRBool asServer);

/*
** Force the handshake for fd to complete immediately.  This blocks until
** the complete SSL handshake protocol is finished.
*/
SSL_IMPORT SECStatus SSL_ForceHandshake(PRFileDesc *fd);

/*
** Same as above, but with an I/O timeout.
 */
SSL_IMPORT SECStatus SSL_ForceHandshakeWithTimeout(PRFileDesc *fd,
                                                   PRIntervalTime timeout);

SSL_IMPORT SECStatus SSL_RestartHandshakeAfterCertReq(PRFileDesc *fd,
					    CERTCertificate *cert,
					    SECKEYPrivateKey *key,
					    CERTCertificateList *certChain);

/*
** Query security status of socket. *on is set to one if security is
** enabled. *keySize will contain the stream key size used. *issuer will
** contain the RFC1485 verison of the name of the issuer of the
** certificate at the other end of the connection. For a client, this is
** the issuer of the server's certificate; for a server, this is the
** issuer of the client's certificate (if any). Subject is the subject of
** the other end's certificate. The pointers can be zero if the desired
** data is not needed.  All strings returned by this function are owned
** by the caller, and need to be freed with PORT_Free.
*/
SSL_IMPORT SECStatus SSL_SecurityStatus(PRFileDesc *fd, int *on, char **cipher,
			                int *keySize, int *secretKeySize,
			                char **issuer, char **subject);

/* Values for "on" */
#define SSL_SECURITY_STATUS_NOOPT	-1
#define SSL_SECURITY_STATUS_OFF		0
#define SSL_SECURITY_STATUS_ON_HIGH	1
#define SSL_SECURITY_STATUS_ON_LOW	2
#define SSL_SECURITY_STATUS_FORTEZZA	3 /* NO LONGER SUPPORTED */

/*
** Return the certificate for our SSL peer. If the client calls this
** it will always return the server's certificate. If the server calls
** this, it may return NULL if client authentication is not enabled or
** if the client had no certificate when asked.
**	"fd" the socket "file" descriptor
*/
SSL_IMPORT CERTCertificate *SSL_PeerCertificate(PRFileDesc *fd);

/*
** Return the certificates presented by the SSL peer. If the SSL peer
** did not present certificates, return NULL with the
** SSL_ERROR_NO_CERTIFICATE error. On failure, return NULL with an error
** code other than SSL_ERROR_NO_CERTIFICATE.
**	"fd" the socket "file" descriptor
*/
SSL_IMPORT CERTCertList *SSL_PeerCertificateChain(PRFileDesc *fd);

/* SSL_PeerStapledOCSPResponses returns the OCSP responses that were provided
 * by the TLS server. The return value is a pointer to an internal SECItemArray
 * that contains the returned OCSP responses; it is only valid until the
 * callback function that calls SSL_PeerStapledOCSPResponses returns.
 *
 * If no OCSP responses were given by the server then the result will be empty.
 * If there was an error, then the result will be NULL.
 *
 * You must set the SSL_ENABLE_OCSP_STAPLING option to enable OCSP stapling.
 * to be provided by a server.
 *
 * libssl does not do any validation of the OCSP response itself; the
 * authenticate certificate hook is responsible for doing so. The default
 * authenticate certificate hook, SSL_AuthCertificate, does not implement
 * any OCSP stapling funtionality, but this may change in future versions.
 */
SSL_IMPORT const SECItemArray * SSL_PeerStapledOCSPResponses(PRFileDesc *fd);

/* SSL_PeerSignedCertTimestamps returns the signed_certificate_timestamp
 * extension data provided by the TLS server. The return value is a pointer
 * to an internal SECItem that contains the returned response (as a serialized
 * SignedCertificateTimestampList, see RFC 6962). The returned pointer is only
 * valid until the callback function that calls SSL_PeerSignedCertTimestamps
 * (e.g. the authenticate certificate hook, or the handshake callback) returns.
 *
 * If no Signed Certificate Timestamps were given by the server then the result
 * will be empty. If there was an error, then the result will be NULL.
 *
 * You must set the SSL_ENABLE_SIGNED_CERT_TIMESTAMPS option to indicate support
 * for Signed Certificate Timestamps to a server.
 *
 * libssl does not do any parsing or validation of the response itself.
 */
SSL_IMPORT const SECItem * SSL_PeerSignedCertTimestamps(PRFileDesc *fd);

/* SSL_SetStapledOCSPResponses stores an array of one or multiple OCSP responses
 * in the fd's data, which may be sent as part of a server side cert_status
 * handshake message. Parameter |responses| is for the server certificate of
 * the key exchange type |kea|.
 * The function will duplicate the responses array.
 */
SSL_IMPORT SECStatus
SSL_SetStapledOCSPResponses(PRFileDesc *fd, const SECItemArray *responses,
			    SSLKEAType kea);

/*
** Authenticate certificate hook. Called when a certificate comes in
** (because of SSL_REQUIRE_CERTIFICATE in SSL_Enable) to authenticate the
** certificate.
**
** The authenticate certificate hook must return SECSuccess to indicate the
** certificate is valid, SECFailure to indicate the certificate is invalid,
** or SECWouldBlock if the application will authenticate the certificate
** asynchronously. SECWouldBlock is only supported for non-blocking sockets.
**
** If the authenticate certificate hook returns SECFailure, then the bad cert
** hook will be called. The bad cert handler is NEVER called if the
** authenticate certificate hook returns SECWouldBlock. If the application
** needs to handle and/or override a bad cert, it should do so before it
** calls SSL_AuthCertificateComplete (modifying the error it passes to
** SSL_AuthCertificateComplete as needed).
**
** See the documentation for SSL_AuthCertificateComplete for more information
** about the asynchronous behavior that occurs when the authenticate
** certificate hook returns SECWouldBlock.
**
** RFC 6066 says that clients should send the bad_certificate_status_response
** alert when they encounter an error processing the stapled OCSP response.
** libssl does not provide a way for the authenticate certificate hook to
** indicate that an OCSP error (SEC_ERROR_OCSP_*) that it returns is an error
** in the stapled OCSP response or an error in some other OCSP response.
** Further, NSS does not provide a convenient way to control or determine
** which OCSP response(s) were used to validate a certificate chain.
** Consequently, the current version of libssl does not ever send the
** bad_certificate_status_response alert. This may change in future releases.
*/
typedef SECStatus (PR_CALLBACK *SSLAuthCertificate)(void *arg, PRFileDesc *fd, 
                                                    PRBool checkSig,
                                                    PRBool isServer);

SSL_IMPORT SECStatus SSL_AuthCertificateHook(PRFileDesc *fd, 
					     SSLAuthCertificate f,
				             void *arg);

/* An implementation of the certificate authentication hook */
SSL_IMPORT SECStatus SSL_AuthCertificate(void *arg, PRFileDesc *fd, 
					 PRBool checkSig, PRBool isServer);

/*
 * Prototype for SSL callback to get client auth data from the application.
 *	arg - application passed argument
 *	caNames - pointer to distinguished names of CAs that the server likes
 *	pRetCert - pointer to pointer to cert, for return of cert
 *	pRetKey - pointer to key pointer, for return of key
 */
typedef SECStatus (PR_CALLBACK *SSLGetClientAuthData)(void *arg,
                                PRFileDesc *fd,
                                CERTDistNames *caNames,
                                CERTCertificate **pRetCert,/*return */
                                SECKEYPrivateKey **pRetKey);/* return */

/*
 * Set the client side callback for SSL to retrieve user's private key
 * and certificate.
 *	fd - the file descriptor for the connection in question
 *	f - the application's callback that delivers the key and cert
 *	a - application specific data
 */
SSL_IMPORT SECStatus SSL_GetClientAuthDataHook(PRFileDesc *fd, 
			                       SSLGetClientAuthData f, void *a);

/*
 * Prototype for SSL callback to get client auth data from the application,
 * optionally using the underlying platform's cryptographic primitives.
 * To use the platform cryptographic primitives, caNames and pRetCerts
 * should be set.  To use NSS, pRetNSSCert and pRetNSSKey should be set.
 * Returning SECFailure will cause the socket to send no client certificate.
 *	arg - application passed argument
 *	caNames - pointer to distinguished names of CAs that the server likes
 *	pRetCerts - pointer to pointer to list of certs, with the first being
 *		    the client cert, and any following being used for chain
 *		    building
 *	pRetKey - pointer to native key pointer, for return of key
 *          - Windows: A pointer to a PCERT_KEY_CONTEXT that was allocated
 *                     via PORT_Alloc(). Ownership of the PCERT_KEY_CONTEXT
 *                     is transferred to NSS, which will free via
 *                     PORT_Free().
 *          - Mac OS X: A pointer to a SecKeyRef. Ownership is
 *                      transferred to NSS, which will free via CFRelease().
 *	pRetNSSCert - pointer to pointer to NSS cert, for return of cert.
 *	pRetNSSKey - pointer to NSS key pointer, for return of key.
 */
typedef SECStatus (PR_CALLBACK *SSLGetPlatformClientAuthData)(void *arg,
                                PRFileDesc *fd,
                                CERTDistNames *caNames,
                                CERTCertList **pRetCerts,/*return */
                                void **pRetKey,/* return */
                                CERTCertificate **pRetNSSCert,/*return */
                                SECKEYPrivateKey **pRetNSSKey);/* return */

/*
 * Set the client side callback for SSL to retrieve user's private key
 * and certificate.
 * Note: If a platform client auth callback is set, the callback configured by
 * SSL_GetClientAuthDataHook, if any, will not be called.
 *
 *	fd - the file descriptor for the connection in question
 *	f - the application's callback that delivers the key and cert
 *	a - application specific data
 */
SSL_IMPORT SECStatus
SSL_GetPlatformClientAuthDataHook(PRFileDesc *fd,
                                  SSLGetPlatformClientAuthData f, void *a);

/*
** SNI extension processing callback function.
** It is called when SSL socket receives SNI extension in ClientHello message.
** Upon this callback invocation, application is responsible to reconfigure the
** socket with the data for a particular server name.
** There are three potential outcomes of this function invocation:
**    * application does not recognize the name or the type and wants the
**    "unrecognized_name" alert be sent to the client. In this case the callback
**    function must return SSL_SNI_SEND_ALERT status.
**    * application does not recognize  the name, but wants to continue with
**    the handshake using the current socket configuration. In this case,
**    no socket reconfiguration is needed and the function should return
**    SSL_SNI_CURRENT_CONFIG_IS_USED.
**    * application recognizes the name and reconfigures the socket with
**    appropriate certs, key, etc. There are many ways to reconfigure. NSS
**    provides SSL_ReconfigFD function that can be used to update the socket
**    data from model socket. To continue with the rest of the handshake, the
**    implementation function should return an index of a name it has chosen.
** LibSSL will ignore any SNI extension received in a ClientHello message
** if application does not register a SSLSNISocketConfig callback.
** Each type field of SECItem indicates the name type.
** NOTE: currently RFC3546 defines only one name type: sni_host_name.
** Client is allowed to send only one name per known type. LibSSL will
** send an "unrecognized_name" alert if SNI extension name list contains more
** then one name of a type.
*/
typedef PRInt32 (PR_CALLBACK *SSLSNISocketConfig)(PRFileDesc *fd,
                                            const SECItem *srvNameArr,
                                                  PRUint32 srvNameArrSize,
                                                  void *arg);

/*
** SSLSNISocketConfig should return an index within 0 and srvNameArrSize-1
** when it has reconfigured the socket fd to use certs and keys, etc
** for a specific name. There are two other allowed return values. One
** tells libSSL to use the default cert and key.  The other tells libSSL
** to send the "unrecognized_name" alert.  These values are:
**/
#define SSL_SNI_CURRENT_CONFIG_IS_USED           -1
#define SSL_SNI_SEND_ALERT                       -2

/*
** Set application implemented SNISocketConfig callback.
*/
SSL_IMPORT SECStatus SSL_SNISocketConfigHook(PRFileDesc *fd, 
                                             SSLSNISocketConfig f,
                                             void *arg);

/*
** Reconfigure fd SSL socket with model socket parameters. Sets
** server certs and keys, list of trust anchor, socket options
** and all SSL socket call backs and parameters.
*/
SSL_IMPORT PRFileDesc *SSL_ReconfigFD(PRFileDesc *model, PRFileDesc *fd);

/*
 * Set the client side argument for SSL to retrieve PKCS #11 pin.
 *	fd - the file descriptor for the connection in question
 *	a - pkcs11 application specific data
 */
SSL_IMPORT SECStatus SSL_SetPKCS11PinArg(PRFileDesc *fd, void *a);

/*
** This is a callback for dealing with server certs that are not authenticated
** by the client.  The client app can decide that it actually likes the
** cert by some external means and restart the connection.
**
** The bad cert hook must return SECSuccess to override the result of the
** authenticate certificate hook, SECFailure if the certificate should still be
** considered invalid, or SECWouldBlock if the application will authenticate
** the certificate asynchronously. SECWouldBlock is only supported for
** non-blocking sockets.
**
** See the documentation for SSL_AuthCertificateComplete for more information
** about the asynchronous behavior that occurs when the bad cert hook returns
** SECWouldBlock.
*/
typedef SECStatus (PR_CALLBACK *SSLBadCertHandler)(void *arg, PRFileDesc *fd);
SSL_IMPORT SECStatus SSL_BadCertHook(PRFileDesc *fd, SSLBadCertHandler f, 
				     void *arg);

/*
** Configure SSL socket for running a secure server. Needs the
** certificate for the server and the servers private key. The arguments
** are copied.
*/
SSL_IMPORT SECStatus SSL_ConfigSecureServer(
				PRFileDesc *fd, CERTCertificate *cert,
				SECKEYPrivateKey *key, SSLKEAType kea);

/*
** Allows SSL socket configuration with caller-supplied certificate chain.
** If certChainOpt is NULL, tries to find one.
*/
SSL_IMPORT SECStatus
SSL_ConfigSecureServerWithCertChain(PRFileDesc *fd, CERTCertificate *cert,
                                    const CERTCertificateList *certChainOpt,
                                    SECKEYPrivateKey *key, SSLKEAType kea);

/*
** Configure a secure server's session-id cache. Define the maximum number
** of entries in the cache, the longevity of the entires, and the directory
** where the cache files will be placed.  These values can be zero, and 
** if so, the implementation will choose defaults.
** This version of the function is for use in applications that have only one 
** process that uses the cache (even if that process has multiple threads).
*/
SSL_IMPORT SECStatus SSL_ConfigServerSessionIDCache(int      maxCacheEntries,
					            PRUint32 timeout,
					            PRUint32 ssl3_timeout,
				              const char *   directory);

/* Configure a secure server's session-id cache. Depends on value of
 * enableMPCache, configures malti-proc or single proc cache. */
SSL_IMPORT SECStatus SSL_ConfigServerSessionIDCacheWithOpt(
                                                           PRUint32 timeout,
                                                       PRUint32 ssl3_timeout,
                                                     const char *   directory,
                                                          int maxCacheEntries,
                                                      int maxCertCacheEntries,
                                                    int maxSrvNameCacheEntries,
                                                           PRBool enableMPCache);

/*
** Like SSL_ConfigServerSessionIDCache, with one important difference.
** If the application will run multiple processes (as opposed to, or in 
** addition to multiple threads), then it must call this function, instead
** of calling SSL_ConfigServerSessionIDCache().
** This has nothing to do with the number of processORs, only processEs.
** This function sets up a Server Session ID (SID) cache that is safe for
** access by multiple processes on the same system.
*/
SSL_IMPORT SECStatus SSL_ConfigMPServerSIDCache(int      maxCacheEntries, 
				                PRUint32 timeout,
			       	                PRUint32 ssl3_timeout, 
		                          const char *   directory);

/* Get and set the configured maximum number of mutexes used for the 
** server's store of SSL sessions.  This value is used by the server 
** session ID cache initialization functions shown above.  Note that on 
** some platforms, these mutexes are actually implemented with POSIX 
** semaphores, or with unnamed pipes.  The default value varies by platform.
** An attempt to set a too-low maximum will return an error and the 
** configured value will not be changed.
*/
SSL_IMPORT PRUint32  SSL_GetMaxServerCacheLocks(void);
SSL_IMPORT SECStatus SSL_SetMaxServerCacheLocks(PRUint32 maxLocks);

/* environment variable set by SSL_ConfigMPServerSIDCache, and queried by
 * SSL_InheritMPServerSIDCache when envString is NULL.
 */
#define SSL_ENV_VAR_NAME            "SSL_INHERITANCE"

/* called in child to inherit SID Cache variables. 
 * If envString is NULL, this function will use the value of the environment
 * variable "SSL_INHERITANCE", otherwise the string value passed in will be 
 * used.
 */
SSL_IMPORT SECStatus SSL_InheritMPServerSIDCache(const char * envString);

/*
** Set the callback that gets called when a TLS handshake is complete. The
** handshake callback is called after verifying the peer's Finished message and
** before processing incoming application data.
**
** For the initial handshake: If the handshake false started (see
** SSL_ENABLE_FALSE_START), then application data may already have been sent
** before the handshake callback is called. If we did not false start then the
** callback will get called before any application data is sent.
*/
typedef void (PR_CALLBACK *SSLHandshakeCallback)(PRFileDesc *fd,
                                                 void *client_data);
SSL_IMPORT SECStatus SSL_HandshakeCallback(PRFileDesc *fd, 
			          SSLHandshakeCallback cb, void *client_data);

/* Applications that wish to enable TLS false start must set this callback
** function. NSS will invoke the functon to determine if a particular
** connection should use false start or not. SECSuccess indicates that the
** callback completed successfully, and if so *canFalseStart indicates if false
** start can be used. If the callback does not return SECSuccess then the
** handshake will be canceled. NSS's recommended criteria can be evaluated by
** calling SSL_RecommendedCanFalseStart.
**
** If no false start callback is registered then false start will never be
** done, even if the SSL_ENABLE_FALSE_START option is enabled.
**/
typedef SECStatus (PR_CALLBACK *SSLCanFalseStartCallback)(
    PRFileDesc *fd, void *arg, PRBool *canFalseStart);

SSL_IMPORT SECStatus SSL_SetCanFalseStartCallback(
    PRFileDesc *fd, SSLCanFalseStartCallback callback, void *arg);

/* This function sets *canFalseStart according to the recommended criteria for
** false start. These criteria may change from release to release and may depend
** on which handshake features have been negotiated and/or properties of the
** certifciates/keys used on the connection.
*/
SSL_IMPORT SECStatus SSL_RecommendedCanFalseStart(PRFileDesc *fd,
                                                  PRBool *canFalseStart);

/*
** For the server, request a new handshake.  For the client, begin a new
** handshake.  If flushCache is non-zero, the SSL3 cache entry will be 
** flushed first, ensuring that a full SSL handshake will be done.
** If flushCache is zero, and an SSL connection is established, it will 
** do the much faster session restart handshake.  This will change the 
** session keys without doing another private key operation.
*/
SSL_IMPORT SECStatus SSL_ReHandshake(PRFileDesc *fd, PRBool flushCache);

/*
** Same as above, but with an I/O timeout.
 */
SSL_IMPORT SECStatus SSL_ReHandshakeWithTimeout(PRFileDesc *fd,
                                                PRBool flushCache,
                                                PRIntervalTime timeout);

/* Returns a SECItem containing the certificate_types field of the
** CertificateRequest message.  Each byte of the data is a TLS
** ClientCertificateType value, and they are ordered from most preferred to
** least.  This function should only be called from the
** SSL_GetClientAuthDataHook callback, and will return NULL if called at any
** other time.  The returned value is valid only until the callback returns, and
** should not be freed.
*/
SSL_IMPORT const SECItem *
SSL_GetRequestedClientCertificateTypes(PRFileDesc *fd);

#ifdef SSL_DEPRECATED_FUNCTION 
/* deprecated!
** For the server, request a new handshake.  For the client, begin a new
** handshake.  Flushes SSL3 session cache entry first, ensuring that a 
** full handshake will be done.  
** This call is equivalent to SSL_ReHandshake(fd, PR_TRUE)
*/
SSL_IMPORT SECStatus SSL_RedoHandshake(PRFileDesc *fd);
#endif

/*
 * Allow the application to pass a URL or hostname into the SSL library.
 */
SSL_IMPORT SECStatus SSL_SetURL(PRFileDesc *fd, const char *url);

/*
 * Allow an application to define a set of trust anchors for peer
 * cert validation.
 */
SSL_IMPORT SECStatus SSL_SetTrustAnchors(PRFileDesc *fd, CERTCertList *list);

/*
** Return the number of bytes that SSL has waiting in internal buffers.
** Return 0 if security is not enabled.
*/
SSL_IMPORT int SSL_DataPending(PRFileDesc *fd);

/*
** Invalidate the SSL session associated with fd.
*/
SSL_IMPORT SECStatus SSL_InvalidateSession(PRFileDesc *fd);

/*
** Cache the SSL session associated with fd, if it has not already been cached.
*/
SSL_IMPORT SECStatus SSL_CacheSession(PRFileDesc *fd);

/*
** Cache the SSL session associated with fd, if it has not already been cached.
** This function may only be called when processing within a callback assigned
** via SSL_HandshakeCallback
*/
SSL_IMPORT SECStatus SSL_CacheSessionUnlocked(PRFileDesc *fd);

/*
** Return a SECItem containing the SSL session ID associated with the fd.
*/
SSL_IMPORT SECItem *SSL_GetSessionID(PRFileDesc *fd);

/*
** Clear out the client's SSL session cache, not the server's session cache.
*/
SSL_IMPORT void SSL_ClearSessionCache(void);

/*
** Close the server's SSL session cache.
*/
SSL_IMPORT SECStatus SSL_ShutdownServerSessionIDCache(void);

/*
** Set peer information so we can correctly look up SSL session later.
** You only have to do this if you're tunneling through a proxy.
*/
SSL_IMPORT SECStatus SSL_SetSockPeerID(PRFileDesc *fd, const char *peerID);

/*
** Reveal the security information for the peer. 
*/
SSL_IMPORT CERTCertificate * SSL_RevealCert(PRFileDesc * socket);
SSL_IMPORT void * SSL_RevealPinArg(PRFileDesc * socket);
SSL_IMPORT char * SSL_RevealURL(PRFileDesc * socket);

/* This callback may be passed to the SSL library via a call to
 * SSL_GetClientAuthDataHook() for each SSL client socket.
 * It will be invoked when SSL needs to know what certificate and private key
 * (if any) to use to respond to a request for client authentication.
 * If arg is non-NULL, it is a pointer to a NULL-terminated string containing
 * the nickname of the cert/key pair to use.
 * If arg is NULL, this function will search the cert and key databases for 
 * a suitable match and send it if one is found.
 */
SSL_IMPORT SECStatus
NSS_GetClientAuthData(void *                       arg,
                      PRFileDesc *                 socket,
                      struct CERTDistNamesStr *    caNames,
                      struct CERTCertificateStr ** pRetCert,
                      struct SECKEYPrivateKeyStr **pRetKey);

/*
** Configure DTLS-SRTP (RFC 5764) cipher suite preferences.
** Input is a list of ciphers in descending preference order and a length
** of the list. As a side effect, this causes the use_srtp extension to be
** negotiated.
**
** Invalid or unimplemented cipher suites in |ciphers| are ignored. If at
** least one cipher suite in |ciphers| is implemented, returns SECSuccess.
** Otherwise returns SECFailure.
*/
SSL_IMPORT SECStatus SSL_SetSRTPCiphers(PRFileDesc *fd,
					const PRUint16 *ciphers,
					unsigned int numCiphers);

/*
** Get the selected DTLS-SRTP cipher suite (if any).
** To be called after the handshake completes.
** Returns SECFailure if not negotiated.
*/
SSL_IMPORT SECStatus SSL_GetSRTPCipher(PRFileDesc *fd,
				       PRUint16 *cipher);

/*
 * Look to see if any of the signers in the cert chain for "cert" are found
 * in the list of caNames.  
 * Returns SECSuccess if so, SECFailure if not.
 * Used by NSS_GetClientAuthData.  May be used by other callback functions.
 */
SSL_IMPORT SECStatus NSS_CmpCertChainWCANames(CERTCertificate *cert, 
                                          CERTDistNames *caNames);

/* 
 * Returns key exchange type of the keys in an SSL server certificate.
 */
SSL_IMPORT SSLKEAType NSS_FindCertKEAType(CERTCertificate * cert);

/* Set cipher policies to a predefined Domestic (U.S.A.) policy.
 * This essentially allows all supported ciphers.
 */
SSL_IMPORT SECStatus NSS_SetDomesticPolicy(void);

/* Set cipher policies to a predefined Policy that is exportable from the USA
 *   according to present U.S. policies as we understand them.
 * It is the same as NSS_SetDomesticPolicy now.
 */
SSL_IMPORT SECStatus NSS_SetExportPolicy(void);

/* Set cipher policies to a predefined Policy that is exportable from the USA
 *   according to present U.S. policies as we understand them, and that the 
 *   nation of France will permit to be imported into their country.
 * It is the same as NSS_SetDomesticPolicy now.
 */
SSL_IMPORT SECStatus NSS_SetFrancePolicy(void);

SSL_IMPORT SSL3Statistics * SSL_GetStatistics(void);

/* Report more information than SSL_SecurityStatus.
** Caller supplies the info struct.  Function fills it in.
*/
SSL_IMPORT SECStatus SSL_GetChannelInfo(PRFileDesc *fd, SSLChannelInfo *info,
                                        PRUintn len);
SSL_IMPORT SECStatus SSL_GetCipherSuiteInfo(PRUint16 cipherSuite, 
                                        SSLCipherSuiteInfo *info, PRUintn len);

/* Returnes negotiated through SNI host info. */
SSL_IMPORT SECItem *SSL_GetNegotiatedHostInfo(PRFileDesc *fd);

/* Export keying material according to RFC 5705.
** fd must correspond to a TLS 1.0 or higher socket and out must
** already be allocated. If hasContext is false, it uses the no-context
** construction from the RFC and ignores the context and contextLen
** arguments.
*/
SSL_IMPORT SECStatus SSL_ExportKeyingMaterial(PRFileDesc *fd,
                                              const char *label,
                                              unsigned int labelLen,
                                              PRBool hasContext,
                                              const unsigned char *context,
                                              unsigned int contextLen,
                                              unsigned char *out,
                                              unsigned int outLen);

/*
** Return a new reference to the certificate that was most recently sent
** to the peer on this SSL/TLS connection, or NULL if none has been sent.
*/
SSL_IMPORT CERTCertificate * SSL_LocalCertificate(PRFileDesc *fd);

/* Test an SSL configuration to see if  SSL_BYPASS_PKCS11 can be turned on.
** Check the key exchange algorithm for each cipher in the list to see if
** a master secret key can be extracted after being derived with the mechanism
** required by the protocolmask argument. If the KEA will use keys from the
** specified cert make sure the extract operation is attempted from the slot
** where the private key resides.
** If MS can be extracted for all ciphers, (*pcanbypass) is set to TRUE and
** SECSuccess is returned. In all other cases but one (*pcanbypass) is
** set to FALSE and SECFailure is returned.
** In that last case Derive() has been called successfully but the MS is null,
** CanBypass sets (*pcanbypass) to FALSE and returns SECSuccess indicating the
** arguments were all valid but the slot cannot be bypassed.
**
** Note: A TRUE return code from CanBypass means "Your configuration will perform
** NO WORSE with the bypass enabled than without"; it does NOT mean that every
** cipher suite listed will work properly with the selected protocols.
**
** Caveat: If export cipher suites are included in the argument list Canbypass
** will return FALSE.
**/

/* protocol mask bits */
#define SSL_CBP_SSL3	0x0001	        /* test SSL v3 mechanisms */
#define SSL_CBP_TLS1_0	0x0002		/* test TLS v1.0 mechanisms */

SSL_IMPORT SECStatus SSL_CanBypass(CERTCertificate *cert,
                                   SECKEYPrivateKey *privKey,
				   PRUint32 protocolmask,
				   PRUint16 *ciphers, int nciphers,
                                   PRBool *pcanbypass, void *pwArg);

/*
** Did the handshake with the peer negotiate the given extension?
** Output parameter valid only if function returns SECSuccess
*/
SSL_IMPORT SECStatus SSL_HandshakeNegotiatedExtension(PRFileDesc * socket,
                                                      SSLExtensionType extId,
                                                      PRBool *yes);

SSL_IMPORT SECStatus SSL_HandshakeResumedSession(PRFileDesc *fd,
                                                 PRBool *last_handshake_resumed);

/* See SSL_SetClientChannelIDCallback for usage. If the callback returns
 * SECWouldBlock then SSL_RestartHandshakeAfterChannelIDReq should be called in
 * the future to restart the handshake.  On SECSuccess, the callback must have
 * written a P-256, EC key pair to |*out_public_key| and |*out_private_key|. */
typedef SECStatus (PR_CALLBACK *SSLClientChannelIDCallback)(
    void *arg,
    PRFileDesc *fd,
    SECKEYPublicKey **out_public_key,
    SECKEYPrivateKey **out_private_key);

/* SSL_RestartHandshakeAfterChannelIDReq attempts to restart the handshake
 * after a ChannelID callback returned SECWouldBlock.
 *
 * This function takes ownership of |channelIDPub| and |channelID|. */
SSL_IMPORT SECStatus SSL_RestartHandshakeAfterChannelIDReq(
    PRFileDesc *fd,
    SECKEYPublicKey *channelIDPub,
    SECKEYPrivateKey *channelID);

/* SSL_SetClientChannelIDCallback sets a callback function that will be called
 * once the server's ServerHello has been processed. This is only applicable to
 * a client socket and setting this callback causes the TLS Channel ID
 * extension to be advertised. */
SSL_IMPORT SECStatus SSL_SetClientChannelIDCallback(
    PRFileDesc *fd,
    SSLClientChannelIDCallback callback,
    void *arg);

/*
** How long should we wait before retransmitting the next flight of
** the DTLS handshake? Returns SECFailure if not DTLS or not in a
** handshake.
*/
SSL_IMPORT SECStatus DTLS_GetHandshakeTimeout(PRFileDesc *socket,
                                              PRIntervalTime *timeout);

/*
 * Return a boolean that indicates whether the underlying library
 * will perform as the caller expects.
 *
 * The only argument is a string, which should be the version
 * identifier of the NSS library. That string will be compared
 * against a string that represents the actual build version of
 * the SSL library.
 */
extern PRBool NSSSSL_VersionCheck(const char *importedVersion);

/*
 * Returns a const string of the SSL library version.
 */
extern const char *NSSSSL_GetVersion(void);

/* Restart an SSL connection that was paused to do asynchronous certificate
 * chain validation (when the auth certificate hook or bad cert handler
 * returned SECWouldBlock).
 *
 * This function only works for non-blocking sockets; Do not use it for
 * blocking sockets. Currently, this function works only for the client role of
 * a connection; it does not work for the server role.
 *
 * The application must call SSL_AuthCertificateComplete with 0 as the value of
 * the error parameter after it has successfully validated the peer's
 * certificate, in order to continue the SSL handshake.
 *
 * The application may call SSL_AuthCertificateComplete with a non-zero value
 * for error (e.g. SEC_ERROR_REVOKED_CERTIFICATE) when certificate validation
 * fails, before it closes the connection. If the application does so, an
 * alert corresponding to the error (e.g. certificate_revoked) will be sent to
 * the peer. See the source code of the internal function
 * ssl3_SendAlertForCertError for the current mapping of error to alert. This
 * mapping may change in future versions of libssl.
 *
 * This function will not complete the entire handshake. The application must
 * call SSL_ForceHandshake, PR_Recv, PR_Send, etc. after calling this function
 * to force the handshake to complete.
 *
 * On the first handshake of a connection, libssl will wait for the peer's
 * certificate to be authenticated before calling the handshake callback,
 * sending a client certificate, sending any application data, or returning
 * any application data to the application. On subsequent (renegotiation)
 * handshakes, libssl will block the handshake unconditionally while the
 * certificate is being validated.
 *
 * libssl may send and receive handshake messages while waiting for the
 * application to call SSL_AuthCertificateComplete, and it may call other
 * callbacks (e.g, the client auth data hook) before
 * SSL_AuthCertificateComplete has been called.
 *
 * An application that uses this asynchronous mechanism will usually have lower
 * handshake latency if it has to do public key operations on the certificate
 * chain and/or CRL/OCSP/cert fetching during the authentication, especially if
 * it does so in parallel on another thread. However, if the application can
 * authenticate the peer's certificate quickly then it may be more efficient
 * to use the synchronous mechanism (i.e. returning SECFailure/SECSuccess
 * instead of SECWouldBlock from the authenticate certificate hook).
 *
 * Be careful about converting an application from synchronous cert validation
 * to asynchronous certificate validation. A naive conversion is likely to
 * result in deadlocks; e.g. the application will wait in PR_Poll for network
 * I/O on the connection while all network I/O on the connection is blocked
 * waiting for this function to be called.
 *
 * Returns SECFailure on failure, SECSuccess on success. Never returns
 * SECWouldBlock. Note that SSL_AuthCertificateComplete will (usually) return
 * SECSuccess; do not interpret the return value of SSL_AuthCertificateComplete
 * as an indicator of whether it is OK to continue using the connection. For
 * example, SSL_AuthCertificateComplete(fd, SEC_ERROR_REVOKED_CERTIFICATE) will
 * return SECSuccess (normally), but that does not mean that the application
 * should continue using the connection. If the application passes a non-zero
 * value for second argument (error), or if SSL_AuthCertificateComplete returns
 * anything other than SECSuccess, then the application should close the
 * connection.
 */
SSL_IMPORT SECStatus SSL_AuthCertificateComplete(PRFileDesc *fd,
						 PRErrorCode error);
SEC_END_PROTOS

#endif /* __ssl_h_ */
