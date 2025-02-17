/*
 * chacha20.h - header file for ChaCha20 implementation.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef FREEBL_CHACHA20_H_
#define FREEBL_CHACHA20_H_

#include <stdint.h>

/* ChaCha20XOR encrypts |inLen| bytes from |in| with the given key and
 * nonce and writes the result to |out|, which may be equal to |in|. The
 * initial block counter is specified by |counter|. */
extern void ChaCha20XOR(unsigned char *out,
			const unsigned char *in, unsigned int inLen,
			const unsigned char key[32],
			const unsigned char nonce[8],
			uint64_t counter);

#endif  /* FREEBL_CHACHA20_H_ */
