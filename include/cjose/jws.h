/*
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2014-2016 Cisco Systems, Inc.  All Rights Reserved.
 */

/**
 * \file  jws.h
 * \brief Functions and data structures for interacting with
 *        JSON Web Signature (JWS) objects.
 *
 */

#ifndef CJOSE_JWS_H
#define CJOSE_JWS_H

/** ES256 owning prepared-state API. */
#define CJOSE_JWS_EXTERNAL_SIGNING_API 1

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "header.h"
#include "jwk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An instance of a JWS object.
 */
typedef struct _cjose_jws_int cjose_jws_t;

/**
 * An owning, opaque ES256 external-signing state. Calls using the same state
 * must be serialized by the caller.
 */
typedef struct _cjose_jws_prepared_int cjose_jws_prepared_t;

/**
 * Prepares an ES256 JWS without a private key or a signing callback.
 *
 * The public JWK must be an EC P-256 key with no private component. The header
 * must be an object with alg exactly "ES256"; the b64 header is not supported.
 * The compact representation, including its terminator, must fit in INT_MAX
 * bytes. Header, payload and public key are copied, so the caller may
 * modify or release them after this call. No private key material is retained.
 *
 * \param public_jwk [in] the expected public-only verification key.
 * \param protected_header [in] protected header values to copy.
 * \param plaintext [in] payload bytes; must be non-NULL, even for an empty payload.
 * \param plaintext_len [in] number of payload bytes.
 * \param err [out] optional error details.
 * \returns an owning prepared state, or NULL on failure.
 */
cjose_jws_prepared_t *cjose_jws_prepare(const cjose_jwk_t *public_jwk,
                                     cjose_header_t *protected_header,
                                     const uint8_t *plaintext,
                                     size_t plaintext_len,
                                     cjose_err *err);

/**
 * Returns the exact signing input, BASE64URL(header) || '.' || BASE64URL(payload).
 * The length excludes any terminator. The read-only buffer belongs to prepared
 * and remains valid until cjose_jws_prepared_release(), including after complete.
 * Output arguments are not changed on failure.
 *
 * \param prepared [in] the prepared state.
 * \param input [out] borrowed signing-input bytes; do not modify or free them.
 * \param input_len [out] number of signing-input bytes.
 * \param err [out] optional error details.
 * \returns true on success.
 */
bool cjose_jws_prepared_get_signing_input(
    const cjose_jws_prepared_t *prepared, const uint8_t **input, size_t *input_len, cjose_err *err);

/**
 * Returns the 32-byte SHA-256 digest of the exact prepared signing input.
 * This is the digest for an external ES256 signer; do not hash it again when
 * using a digest-signing API. Buffer ownership, lifetime and output-argument
 * behavior are the same as cjose_jws_prepared_get_signing_input().
 *
 * \param prepared [in] the prepared state.
 * \param digest [out] borrowed digest bytes; do not modify or free them.
 * \param digest_len [out] number of digest bytes (32).
 * \param err [out] optional error details.
 * \returns true on success.
 */
bool cjose_jws_prepared_get_digest(
    const cjose_jws_prepared_t *prepared, const uint8_t **digest, size_t *digest_len, cjose_err *err);

/**
 * Verifies an external signature against the frozen public key and signing
 * input, then returns a normal JWS owned by the caller.
 *
 * The signature must be exactly 64 bytes in JOSE R || S format, not ASN.1 DER.
 * It is copied, not retained. On failure NULL is returned and prepared remains
 * unchanged and retryable, including after an allocation failure. On success
 * prepared is completed: subsequent calls fail with CJOSE_ERR_INVALID_STATE.
 * Accessors remain available. The caller must release both prepared and the
 * returned JWS independently, in either order.
 *
 * \param prepared [in,out] the prepared state.
 * \param signature [in] the external JOSE signature.
 * \param signature_len [in] number of signature bytes (64).
 * \param err [out] optional error details; invalid arguments use
 *        CJOSE_ERR_INVALID_ARG, a signature mismatch uses CJOSE_ERR_CRYPTO.
 * \returns a verified JWS, or NULL on failure. No partial JWS is published.
 */
cjose_jws_t *cjose_jws_complete(
    cjose_jws_prepared_t *prepared, const uint8_t *signature, size_t signature_len, cjose_err *err);

/**
 * Releases an unfinished or completed prepared state. NULL is a no-op.
 * Does not release a JWS previously returned by cjose_jws_complete().
 */
void cjose_jws_prepared_release(cjose_jws_prepared_t *prepared);

/**
 * Creates a new JWS by signing the given plaintext within the given header
 * and JWK.
 *
 * \param jwk [in] the key to use for signing the JWS.
 * \param protected_header [in] header values to include in the JWS header.
 * \param plaintext [in] the plaintext to be signed as the JWS payload.
 * \param plaintext_len [in] the length of the plaintext.
 * \param err [out] An optional error object which can be used to get additional
 *        information in the event of an error.
 * \returns a newly generated JWS with the given plaintext as the payload.
 */
cjose_jws_t *cjose_jws_sign(
    const cjose_jwk_t *jwk, cjose_header_t *protected_header, const uint8_t *plaintext, size_t plaintext_len, cjose_err *err);

/**
 * Creates a serialization of the given JWS object.
 *
 * Note the current implementation only supports serialization to the JWS
 * compact format.
 *
 * \param jws [in] the JWS object to be serialized.
 * \param ser [out] pointer to a compact serialization of this JWS.  Note
 *        the returned string pointer is owned by the JWS, the caller should
 *        not attempt to free it directly, and note that it will be freed
 *        automatically when the JWS itself is released.
 * \param err [out] An optional error object which can be used to get additional
 *        information in the event of an error.
 * \returns true if the serialization is successfully returned.
 */
bool cjose_jws_export(cjose_jws_t *jws, const char **ser, cjose_err *err);

/**
 * Creates a new JWS object from the given JWS compact serialization.
 *
 * Note the current implementation only recognizes the JWS compact serialization
 * format.
 *
 * \param compact [in] a JWS in serialized form.
 * \param compact_len [in] the length of the compact serialization.
 * \param err [out] An optional error object which can be used to get additional
 *        information in the event of an error.
 * \returns a newly generated JWS object from the given JWS serialization.
 */
cjose_jws_t *cjose_jws_import(const char *compact, size_t compact_len, cjose_err *err);

/**
 * Verifies the JWS object using the given JWK.
 *
 * \param jws [in] the JWS object to verify.
 * \param jwk [in] the key to use for verification.
 * \param err [out] An optional error object which can be used to get additional
 *        information in the event of an error.
 * \returns true if verification was sucecssful.
 */
bool cjose_jws_verify(cjose_jws_t *jws, const cjose_jwk_t *jwk, cjose_err *err);

/**
 * Returns the plaintext data of the JWS payload.
 *
 * \param jws [in] the JWS object for which the plaintext is requested.
 * \param plaintext [out] pointer to the plaintext of this JWS.  Note
 *        the returned buffer is owned by the JWS, the caller should
 *        not attempt to free it directly, and note that it will be freed
 *        automatically when the JWS itself is released.
 * \param plaintext_len [out] number of bytes of plaintext in the returned
 *        plaintext buffer.
 * \param err [out] An optional error object which can be used to get additional
 *        information in the event of an error.
 * \returns true if the plaintext is sucessfully returned.
 */
bool cjose_jws_get_plaintext(const cjose_jws_t *jws, uint8_t **plaintext, size_t *plaintext_len, cjose_err *err);

/**
 * Returns the protected header of the JWS payload.
 *
 * **NOTE:** The returned header is still owned by the JWS object. Users must
 * call `cjose_header_retain()` if it is expected to be valid after the
 * owning `cjose_jws_t` is released.
 *
 * \param jws [in] the JWS object for which the protected header is requested.
 * \returns the (parsed) protected header
 */
cjose_header_t *cjose_jws_get_protected(cjose_jws_t *jws);

/**
 * Releases the given JWS object.
 *
 * \param jws the JWS to be released.  If null, this is a no-op.
 */
void cjose_jws_release(cjose_jws_t *jws);

#ifdef __cplusplus
}
#endif

#endif // CJOSE_JWS_H
