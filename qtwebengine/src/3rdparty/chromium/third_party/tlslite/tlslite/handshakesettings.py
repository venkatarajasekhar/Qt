# Authors: 
#   Trevor Perrin
#   Dave Baggett (Arcode Corporation) - cleanup handling of constants
#
# See the LICENSE file for legal information regarding use of this file.

"""Class for setting handshake parameters."""

from .constants import CertificateType
from .utils import cryptomath
from .utils import cipherfactory

# RC4 is preferred as faster in Python, works in SSL3, and immune to CBC
# issues such as timing attacks
CIPHER_NAMES = ["rc4", "aes256", "aes128", "3des"]
MAC_NAMES = ["sha"] # Don't allow "md5" by default.
ALL_MAC_NAMES = ["sha", "md5"]
KEY_EXCHANGE_NAMES = ["rsa", "dhe_rsa", "srp_sha", "srp_sha_rsa", "dh_anon"]
CIPHER_IMPLEMENTATIONS = ["openssl", "pycrypto", "python"]
CERTIFICATE_TYPES = ["x509"]

class HandshakeSettings(object):
    """This class encapsulates various parameters that can be used with
    a TLS handshake.
    @sort: minKeySize, maxKeySize, cipherNames, macNames, certificateTypes,
    minVersion, maxVersion

    @type minKeySize: int
    @ivar minKeySize: The minimum bit length for asymmetric keys.

    If the other party tries to use SRP, RSA, or Diffie-Hellman
    parameters smaller than this length, an alert will be
    signalled.  The default is 1023.

    @type maxKeySize: int
    @ivar maxKeySize: The maximum bit length for asymmetric keys.

    If the other party tries to use SRP, RSA, or Diffie-Hellman
    parameters larger than this length, an alert will be signalled.
    The default is 8193.

    @type cipherNames: list
    @ivar cipherNames: The allowed ciphers, in order of preference.

    The allowed values in this list are 'aes256', 'aes128', '3des', and
    'rc4'.  If these settings are used with a client handshake, they
    determine the order of the ciphersuites offered in the ClientHello
    message.

    If these settings are used with a server handshake, the server will
    choose whichever ciphersuite matches the earliest entry in this
    list.

    NOTE:  If '3des' is used in this list, but TLS Lite can't find an
    add-on library that supports 3DES, then '3des' will be silently
    removed.

    The default value is ['rc4', 'aes256', 'aes128', '3des'].

    @type macNames: list
    @ivar macNames: The allowed MAC algorithms.
    
    The allowed values in this list are 'sha' and 'md5'.
    
    The default value is ['sha'].


    @type certificateTypes: list
    @ivar certificateTypes: The allowed certificate types, in order of
    preference.

    The only allowed certificate type is 'x509'.  This list is only used with a
    client handshake.  The client will advertise to the server which certificate
    types are supported, and will check that the server uses one of the
    appropriate types.


    @type minVersion: tuple
    @ivar minVersion: The minimum allowed SSL/TLS version.

    This variable can be set to (3,0) for SSL 3.0, (3,1) for
    TLS 1.0, or (3,2) for TLS 1.1.  If the other party wishes to
    use a lower version, a protocol_version alert will be signalled.
    The default is (3,0).

    @type maxVersion: tuple
    @ivar maxVersion: The maximum allowed SSL/TLS version.

    This variable can be set to (3,0) for SSL 3.0, (3,1) for
    TLS 1.0, or (3,2) for TLS 1.1.  If the other party wishes to
    use a higher version, a protocol_version alert will be signalled.
    The default is (3,2).  (WARNING: Some servers may (improperly)
    reject clients which offer support for TLS 1.1.  In this case,
    try lowering maxVersion to (3,1)).
    
    @type useExperimentalTackExtension: bool
    @ivar useExperimentalTackExtension: Whether to enabled TACK support.
    
    Note that TACK support is not standardized by IETF and uses a temporary
    TLS Extension number, so should NOT be used in production software.
    """
    def __init__(self):
        self.minKeySize = 1023
        self.maxKeySize = 8193
        self.cipherNames = CIPHER_NAMES
        self.macNames = MAC_NAMES
        self.keyExchangeNames = KEY_EXCHANGE_NAMES
        self.cipherImplementations = CIPHER_IMPLEMENTATIONS
        self.certificateTypes = CERTIFICATE_TYPES
        self.minVersion = (3,0)
        self.maxVersion = (3,2)
        self.useExperimentalTackExtension = False

    # Validates the min/max fields, and certificateTypes
    # Filters out unsupported cipherNames and cipherImplementations
    def _filter(self):
        other = HandshakeSettings()
        other.minKeySize = self.minKeySize
        other.maxKeySize = self.maxKeySize
        other.cipherNames = self.cipherNames
        other.macNames = self.macNames
        other.keyExchangeNames = self.keyExchangeNames
        other.cipherImplementations = self.cipherImplementations
        other.certificateTypes = self.certificateTypes
        other.minVersion = self.minVersion
        other.maxVersion = self.maxVersion

        if not cipherfactory.tripleDESPresent:
            other.cipherNames = [e for e in self.cipherNames if e != "3des"]
        if len(other.cipherNames)==0:
            raise ValueError("No supported ciphers")
        if len(other.certificateTypes)==0:
            raise ValueError("No supported certificate types")

        if not cryptomath.m2cryptoLoaded:
            other.cipherImplementations = \
                [e for e in other.cipherImplementations if e != "openssl"]
        if not cryptomath.pycryptoLoaded:
            other.cipherImplementations = \
                [e for e in other.cipherImplementations if e != "pycrypto"]
        if len(other.cipherImplementations)==0:
            raise ValueError("No supported cipher implementations")

        if other.minKeySize<512:
            raise ValueError("minKeySize too small")
        if other.minKeySize>16384:
            raise ValueError("minKeySize too large")
        if other.maxKeySize<512:
            raise ValueError("maxKeySize too small")
        if other.maxKeySize>16384:
            raise ValueError("maxKeySize too large")
        for s in other.cipherNames:
            if s not in CIPHER_NAMES:
                raise ValueError("Unknown cipher name: '%s'" % s)
        for s in other.macNames:
            if s not in ALL_MAC_NAMES:
                raise ValueError("Unknown MAC name: '%s'" % s)
        for s in other.keyExchangeNames:
            if s not in KEY_EXCHANGE_NAMES:
                raise ValueError("Unknown key exchange name: '%s'" % s)
        for s in other.cipherImplementations:
            if s not in CIPHER_IMPLEMENTATIONS:
                raise ValueError("Unknown cipher implementation: '%s'" % s)
        for s in other.certificateTypes:
            if s not in CERTIFICATE_TYPES:
                raise ValueError("Unknown certificate type: '%s'" % s)

        if other.minVersion > other.maxVersion:
            raise ValueError("Versions set incorrectly")

        if not other.minVersion in ((3,0), (3,1), (3,2)):
            raise ValueError("minVersion set incorrectly")

        if not other.maxVersion in ((3,0), (3,1), (3,2)):
            raise ValueError("maxVersion set incorrectly")

        return other

    def _getCertificateTypes(self):
        l = []
        for ct in self.certificateTypes:
            if ct == "x509":
                l.append(CertificateType.x509)
            else:
                raise AssertionError()
        return l
