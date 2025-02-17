/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SEC_ERR_H_
#define __SEC_ERR_H_

#include "utilrename.h"

#define SEC_ERROR_BASE                          (-0x2000)
#define SEC_ERROR_LIMIT                         (SEC_ERROR_BASE + 1000)

#define IS_SEC_ERROR(code) \
    (((code) >= SEC_ERROR_BASE) && ((code) < SEC_ERROR_LIMIT))

#ifndef NO_SECURITY_ERROR_ENUM
typedef enum {
SEC_ERROR_IO                                =   SEC_ERROR_BASE + 0,
SEC_ERROR_LIBRARY_FAILURE                   =   SEC_ERROR_BASE + 1,
SEC_ERROR_BAD_DATA                          =   SEC_ERROR_BASE + 2,
SEC_ERROR_OUTPUT_LEN                        =   SEC_ERROR_BASE + 3,
SEC_ERROR_INPUT_LEN                         =   SEC_ERROR_BASE + 4,
SEC_ERROR_INVALID_ARGS                      =   SEC_ERROR_BASE + 5,
SEC_ERROR_INVALID_ALGORITHM                 =   SEC_ERROR_BASE + 6,
SEC_ERROR_INVALID_AVA                       =   SEC_ERROR_BASE + 7,
SEC_ERROR_INVALID_TIME                      =   SEC_ERROR_BASE + 8,
SEC_ERROR_BAD_DER                           =   SEC_ERROR_BASE + 9,
SEC_ERROR_BAD_SIGNATURE                     =   SEC_ERROR_BASE + 10,
SEC_ERROR_EXPIRED_CERTIFICATE               =   SEC_ERROR_BASE + 11,
SEC_ERROR_REVOKED_CERTIFICATE               =   SEC_ERROR_BASE + 12,
SEC_ERROR_UNKNOWN_ISSUER                    =   SEC_ERROR_BASE + 13,
SEC_ERROR_BAD_KEY                           =   SEC_ERROR_BASE + 14,
SEC_ERROR_BAD_PASSWORD                      =   SEC_ERROR_BASE + 15,
SEC_ERROR_RETRY_PASSWORD                    =   SEC_ERROR_BASE + 16,
SEC_ERROR_NO_NODELOCK                       =   SEC_ERROR_BASE + 17,
SEC_ERROR_BAD_DATABASE                      =   SEC_ERROR_BASE + 18,
SEC_ERROR_NO_MEMORY                         =   SEC_ERROR_BASE + 19,
SEC_ERROR_UNTRUSTED_ISSUER                  =   SEC_ERROR_BASE + 20,
SEC_ERROR_UNTRUSTED_CERT                    =   SEC_ERROR_BASE + 21,
SEC_ERROR_DUPLICATE_CERT                    =   (SEC_ERROR_BASE + 22),
SEC_ERROR_DUPLICATE_CERT_NAME               =   (SEC_ERROR_BASE + 23),
SEC_ERROR_ADDING_CERT                       =   (SEC_ERROR_BASE + 24),
SEC_ERROR_FILING_KEY                        =   (SEC_ERROR_BASE + 25),
SEC_ERROR_NO_KEY                            =   (SEC_ERROR_BASE + 26),
SEC_ERROR_CERT_VALID                        =   (SEC_ERROR_BASE + 27),
SEC_ERROR_CERT_NOT_VALID                    =   (SEC_ERROR_BASE + 28),
SEC_ERROR_CERT_NO_RESPONSE                  =   (SEC_ERROR_BASE + 29),
SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE        =   (SEC_ERROR_BASE + 30),
SEC_ERROR_CRL_EXPIRED                       =   (SEC_ERROR_BASE + 31),
SEC_ERROR_CRL_BAD_SIGNATURE                 =   (SEC_ERROR_BASE + 32),
SEC_ERROR_CRL_INVALID                       =   (SEC_ERROR_BASE + 33),
SEC_ERROR_EXTENSION_VALUE_INVALID           =   (SEC_ERROR_BASE + 34),
SEC_ERROR_EXTENSION_NOT_FOUND               =   (SEC_ERROR_BASE + 35),
SEC_ERROR_CA_CERT_INVALID                   =   (SEC_ERROR_BASE + 36),
SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID       =   (SEC_ERROR_BASE + 37),
SEC_ERROR_CERT_USAGES_INVALID               =   (SEC_ERROR_BASE + 38),
SEC_INTERNAL_ONLY                           =   (SEC_ERROR_BASE + 39),
SEC_ERROR_INVALID_KEY                       =   (SEC_ERROR_BASE + 40),
SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION        =   (SEC_ERROR_BASE + 41),
SEC_ERROR_OLD_CRL                           =   (SEC_ERROR_BASE + 42),
SEC_ERROR_NO_EMAIL_CERT                     =   (SEC_ERROR_BASE + 43),
SEC_ERROR_NO_RECIPIENT_CERTS_QUERY          =   (SEC_ERROR_BASE + 44),
SEC_ERROR_NOT_A_RECIPIENT                   =   (SEC_ERROR_BASE + 45),
SEC_ERROR_PKCS7_KEYALG_MISMATCH             =   (SEC_ERROR_BASE + 46),
SEC_ERROR_PKCS7_BAD_SIGNATURE               =   (SEC_ERROR_BASE + 47),
SEC_ERROR_UNSUPPORTED_KEYALG                =   (SEC_ERROR_BASE + 48),
SEC_ERROR_DECRYPTION_DISALLOWED             =   (SEC_ERROR_BASE + 49),
/* Fortezza Alerts */
XP_SEC_FORTEZZA_BAD_CARD                    =   (SEC_ERROR_BASE + 50),
XP_SEC_FORTEZZA_NO_CARD                     =   (SEC_ERROR_BASE + 51),
XP_SEC_FORTEZZA_NONE_SELECTED               =   (SEC_ERROR_BASE + 52),
XP_SEC_FORTEZZA_MORE_INFO                   =   (SEC_ERROR_BASE + 53),
XP_SEC_FORTEZZA_PERSON_NOT_FOUND            =   (SEC_ERROR_BASE + 54),
XP_SEC_FORTEZZA_NO_MORE_INFO                =   (SEC_ERROR_BASE + 55),
XP_SEC_FORTEZZA_BAD_PIN                     =   (SEC_ERROR_BASE + 56),
XP_SEC_FORTEZZA_PERSON_ERROR                =   (SEC_ERROR_BASE + 57),
SEC_ERROR_NO_KRL                            =   (SEC_ERROR_BASE + 58),
SEC_ERROR_KRL_EXPIRED                       =   (SEC_ERROR_BASE + 59),
SEC_ERROR_KRL_BAD_SIGNATURE                 =   (SEC_ERROR_BASE + 60),
SEC_ERROR_REVOKED_KEY                       =   (SEC_ERROR_BASE + 61),
SEC_ERROR_KRL_INVALID                       =   (SEC_ERROR_BASE + 62),
SEC_ERROR_NEED_RANDOM                       =   (SEC_ERROR_BASE + 63),
SEC_ERROR_NO_MODULE                         =   (SEC_ERROR_BASE + 64),
SEC_ERROR_NO_TOKEN                          =   (SEC_ERROR_BASE + 65),
SEC_ERROR_READ_ONLY                         =   (SEC_ERROR_BASE + 66),
SEC_ERROR_NO_SLOT_SELECTED                  =   (SEC_ERROR_BASE + 67),
SEC_ERROR_CERT_NICKNAME_COLLISION           =   (SEC_ERROR_BASE + 68),
SEC_ERROR_KEY_NICKNAME_COLLISION            =   (SEC_ERROR_BASE + 69),
SEC_ERROR_SAFE_NOT_CREATED                  =   (SEC_ERROR_BASE + 70),
SEC_ERROR_BAGGAGE_NOT_CREATED               =   (SEC_ERROR_BASE + 71),
XP_JAVA_REMOVE_PRINCIPAL_ERROR              =   (SEC_ERROR_BASE + 72),
XP_JAVA_DELETE_PRIVILEGE_ERROR              =   (SEC_ERROR_BASE + 73),
XP_JAVA_CERT_NOT_EXISTS_ERROR               =   (SEC_ERROR_BASE + 74),
SEC_ERROR_BAD_EXPORT_ALGORITHM              =   (SEC_ERROR_BASE + 75),
SEC_ERROR_EXPORTING_CERTIFICATES            =   (SEC_ERROR_BASE + 76),
SEC_ERROR_IMPORTING_CERTIFICATES            =   (SEC_ERROR_BASE + 77),
SEC_ERROR_PKCS12_DECODING_PFX               =   (SEC_ERROR_BASE + 78),
SEC_ERROR_PKCS12_INVALID_MAC                =   (SEC_ERROR_BASE + 79),
SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM  =   (SEC_ERROR_BASE + 80),
SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE =   (SEC_ERROR_BASE + 81),
SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE      =   (SEC_ERROR_BASE + 82),
SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM  =   (SEC_ERROR_BASE + 83),
SEC_ERROR_PKCS12_UNSUPPORTED_VERSION        =   (SEC_ERROR_BASE + 84),
SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT =   (SEC_ERROR_BASE + 85),
SEC_ERROR_PKCS12_CERT_COLLISION             =   (SEC_ERROR_BASE + 86),
SEC_ERROR_USER_CANCELLED                    =   (SEC_ERROR_BASE + 87),
SEC_ERROR_PKCS12_DUPLICATE_DATA             =   (SEC_ERROR_BASE + 88),
SEC_ERROR_MESSAGE_SEND_ABORTED              =   (SEC_ERROR_BASE + 89),
SEC_ERROR_INADEQUATE_KEY_USAGE              =   (SEC_ERROR_BASE + 90),
SEC_ERROR_INADEQUATE_CERT_TYPE              =   (SEC_ERROR_BASE + 91),
SEC_ERROR_CERT_ADDR_MISMATCH                =   (SEC_ERROR_BASE + 92),
SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY       =   (SEC_ERROR_BASE + 93),
SEC_ERROR_PKCS12_IMPORTING_CERT_CHAIN       =   (SEC_ERROR_BASE + 94),
SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME = (SEC_ERROR_BASE + 95),
SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY       =   (SEC_ERROR_BASE + 96),
SEC_ERROR_PKCS12_UNABLE_TO_WRITE            =   (SEC_ERROR_BASE + 97),
SEC_ERROR_PKCS12_UNABLE_TO_READ             =   (SEC_ERROR_BASE + 98),
SEC_ERROR_PKCS12_KEY_DATABASE_NOT_INITIALIZED = (SEC_ERROR_BASE + 99),
SEC_ERROR_KEYGEN_FAIL                       =   (SEC_ERROR_BASE + 100),
SEC_ERROR_INVALID_PASSWORD                  =   (SEC_ERROR_BASE + 101),
SEC_ERROR_RETRY_OLD_PASSWORD                =   (SEC_ERROR_BASE + 102),
SEC_ERROR_BAD_NICKNAME                      =   (SEC_ERROR_BASE + 103),
SEC_ERROR_NOT_FORTEZZA_ISSUER               =   (SEC_ERROR_BASE + 104),
SEC_ERROR_CANNOT_MOVE_SENSITIVE_KEY         =   (SEC_ERROR_BASE + 105),
SEC_ERROR_JS_INVALID_MODULE_NAME            =   (SEC_ERROR_BASE + 106),
SEC_ERROR_JS_INVALID_DLL                    =   (SEC_ERROR_BASE + 107),
SEC_ERROR_JS_ADD_MOD_FAILURE                =   (SEC_ERROR_BASE + 108),
SEC_ERROR_JS_DEL_MOD_FAILURE                =   (SEC_ERROR_BASE + 109),
SEC_ERROR_OLD_KRL                           =   (SEC_ERROR_BASE + 110),
SEC_ERROR_CKL_CONFLICT                      =   (SEC_ERROR_BASE + 111),
SEC_ERROR_CERT_NOT_IN_NAME_SPACE            =   (SEC_ERROR_BASE + 112),
SEC_ERROR_KRL_NOT_YET_VALID                 =   (SEC_ERROR_BASE + 113),
SEC_ERROR_CRL_NOT_YET_VALID                 =   (SEC_ERROR_BASE + 114),
SEC_ERROR_UNKNOWN_CERT                      =   (SEC_ERROR_BASE + 115),
SEC_ERROR_UNKNOWN_SIGNER                    =   (SEC_ERROR_BASE + 116),
SEC_ERROR_CERT_BAD_ACCESS_LOCATION          =   (SEC_ERROR_BASE + 117),
SEC_ERROR_OCSP_UNKNOWN_RESPONSE_TYPE        =   (SEC_ERROR_BASE + 118),
SEC_ERROR_OCSP_BAD_HTTP_RESPONSE            =   (SEC_ERROR_BASE + 119),
SEC_ERROR_OCSP_MALFORMED_REQUEST            =   (SEC_ERROR_BASE + 120),
SEC_ERROR_OCSP_SERVER_ERROR                 =   (SEC_ERROR_BASE + 121),
SEC_ERROR_OCSP_TRY_SERVER_LATER             =   (SEC_ERROR_BASE + 122),
SEC_ERROR_OCSP_REQUEST_NEEDS_SIG            =   (SEC_ERROR_BASE + 123),
SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST         =   (SEC_ERROR_BASE + 124),
SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS      =   (SEC_ERROR_BASE + 125),
SEC_ERROR_OCSP_UNKNOWN_CERT                 =   (SEC_ERROR_BASE + 126),
SEC_ERROR_OCSP_NOT_ENABLED                  =   (SEC_ERROR_BASE + 127),
SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER         =   (SEC_ERROR_BASE + 128),
SEC_ERROR_OCSP_MALFORMED_RESPONSE           =   (SEC_ERROR_BASE + 129),
SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE        =   (SEC_ERROR_BASE + 130),
SEC_ERROR_OCSP_FUTURE_RESPONSE              =   (SEC_ERROR_BASE + 131),
SEC_ERROR_OCSP_OLD_RESPONSE                 =   (SEC_ERROR_BASE + 132),
/* smime stuff */
SEC_ERROR_DIGEST_NOT_FOUND                  =   (SEC_ERROR_BASE + 133),
SEC_ERROR_UNSUPPORTED_MESSAGE_TYPE          =   (SEC_ERROR_BASE + 134),
SEC_ERROR_MODULE_STUCK                      =   (SEC_ERROR_BASE + 135),
SEC_ERROR_BAD_TEMPLATE                      =   (SEC_ERROR_BASE + 136),
SEC_ERROR_CRL_NOT_FOUND                     =   (SEC_ERROR_BASE + 137),
SEC_ERROR_REUSED_ISSUER_AND_SERIAL          =   (SEC_ERROR_BASE + 138),
SEC_ERROR_BUSY                              =   (SEC_ERROR_BASE + 139),
SEC_ERROR_EXTRA_INPUT                       =   (SEC_ERROR_BASE + 140),
/* error codes used by elliptic curve code */
SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE        =   (SEC_ERROR_BASE + 141),
SEC_ERROR_UNSUPPORTED_EC_POINT_FORM         =   (SEC_ERROR_BASE + 142),
SEC_ERROR_UNRECOGNIZED_OID                  =   (SEC_ERROR_BASE + 143),
SEC_ERROR_OCSP_INVALID_SIGNING_CERT         =   (SEC_ERROR_BASE + 144),
/* new revocation errors */
SEC_ERROR_REVOKED_CERTIFICATE_CRL           =   (SEC_ERROR_BASE + 145),
SEC_ERROR_REVOKED_CERTIFICATE_OCSP          =   (SEC_ERROR_BASE + 146),
SEC_ERROR_CRL_INVALID_VERSION               =   (SEC_ERROR_BASE + 147),
SEC_ERROR_CRL_V1_CRITICAL_EXTENSION         =   (SEC_ERROR_BASE + 148),
SEC_ERROR_CRL_UNKNOWN_CRITICAL_EXTENSION    =   (SEC_ERROR_BASE + 149),
SEC_ERROR_UNKNOWN_OBJECT_TYPE               =   (SEC_ERROR_BASE + 150),
SEC_ERROR_INCOMPATIBLE_PKCS11               =   (SEC_ERROR_BASE + 151),
SEC_ERROR_NO_EVENT                          =   (SEC_ERROR_BASE + 152),
SEC_ERROR_CRL_ALREADY_EXISTS                =   (SEC_ERROR_BASE + 153),
SEC_ERROR_NOT_INITIALIZED                   =   (SEC_ERROR_BASE + 154),
SEC_ERROR_TOKEN_NOT_LOGGED_IN               =   (SEC_ERROR_BASE + 155),
SEC_ERROR_OCSP_RESPONDER_CERT_INVALID       =   (SEC_ERROR_BASE + 156),
SEC_ERROR_OCSP_BAD_SIGNATURE                =   (SEC_ERROR_BASE + 157),

SEC_ERROR_OUT_OF_SEARCH_LIMITS              =   (SEC_ERROR_BASE + 158),
SEC_ERROR_INVALID_POLICY_MAPPING            =   (SEC_ERROR_BASE + 159),
SEC_ERROR_POLICY_VALIDATION_FAILED          =   (SEC_ERROR_BASE + 160),
/* No longer used.  Unknown AIA location types are now silently ignored. */
SEC_ERROR_UNKNOWN_AIA_LOCATION_TYPE         =   (SEC_ERROR_BASE + 161),
SEC_ERROR_BAD_HTTP_RESPONSE                 =   (SEC_ERROR_BASE + 162),
SEC_ERROR_BAD_LDAP_RESPONSE                 =   (SEC_ERROR_BASE + 163),
SEC_ERROR_FAILED_TO_ENCODE_DATA             =   (SEC_ERROR_BASE + 164),
SEC_ERROR_BAD_INFO_ACCESS_LOCATION          =   (SEC_ERROR_BASE + 165),

SEC_ERROR_LIBPKIX_INTERNAL                  =   (SEC_ERROR_BASE + 166),

SEC_ERROR_PKCS11_GENERAL_ERROR              =   (SEC_ERROR_BASE + 167),
SEC_ERROR_PKCS11_FUNCTION_FAILED            =   (SEC_ERROR_BASE + 168),
SEC_ERROR_PKCS11_DEVICE_ERROR               =   (SEC_ERROR_BASE + 169),

SEC_ERROR_BAD_INFO_ACCESS_METHOD            =   (SEC_ERROR_BASE + 170),
SEC_ERROR_CRL_IMPORT_FAILED                 =   (SEC_ERROR_BASE + 171),

SEC_ERROR_EXPIRED_PASSWORD                  =   (SEC_ERROR_BASE + 172),
SEC_ERROR_LOCKED_PASSWORD                   =   (SEC_ERROR_BASE + 173),

SEC_ERROR_UNKNOWN_PKCS11_ERROR              =   (SEC_ERROR_BASE + 174),

SEC_ERROR_BAD_CRL_DP_URL                    =   (SEC_ERROR_BASE + 175),

SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED =   (SEC_ERROR_BASE + 176),

SEC_ERROR_LEGACY_DATABASE                   =   (SEC_ERROR_BASE + 177),

SEC_ERROR_APPLICATION_CALLBACK_ERROR        =   (SEC_ERROR_BASE + 178),

/* Add new error codes above here. */
SEC_ERROR_END_OF_LIST
} SECErrorCodes;
#endif /* NO_SECURITY_ERROR_ENUM */

#endif /* __SEC_ERR_H_ */
