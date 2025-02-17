/*
 * aes_icm.c
 *
 * AES Integer Counter Mode
 *
 * David A. McGrew
 * Cisco Systems, Inc.
 */

/*
 *	
 * Copyright (c) 2001-2006, Cisco Systems, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * 
 *   Neither the name of the Cisco Systems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#define ALIGN_32 0

#include "aes_icm.h"
#include "alloc.h"


debug_module_t mod_aes_icm = {
  0,                 /* debugging is off by default */
  "aes icm"          /* printable module name       */
};

/*
 * integer counter mode works as follows:
 *
 * 16 bits
 * <----->
 * +------+------+------+------+------+------+------+------+ 
 * |           nonce           |    pakcet index    |  ctr |---+
 * +------+------+------+------+------+------+------+------+   |
 *                                                             |
 * +------+------+------+------+------+------+------+------+   v
 * |                      salt                      |000000|->(+)
 * +------+------+------+------+------+------+------+------+   |
 *                                                             |
 *                                                        +---------+
 *							  | encrypt |
 *							  +---------+
 *							       | 
 * +------+------+------+------+------+------+------+------+   |
 * |                    keystream block                    |<--+ 
 * +------+------+------+------+------+------+------+------+   
 *
 * All fields are big-endian
 *
 * ctr is the block counter, which increments from zero for
 * each packet (16 bits wide)
 * 
 * packet index is distinct for each packet (48 bits wide)
 *
 * nonce can be distinct across many uses of the same key, or
 * can be a fixed value per key, or can be per-packet randomness
 * (64 bits)
 *
 */

err_status_t
aes_icm_alloc_ismacryp(cipher_t **c, int key_len, int forIsmacryp) {
  extern cipher_type_t aes_icm;
  uint8_t *pointer;
  int tmp;

  debug_print(mod_aes_icm, 
            "allocating cipher with key length %d", key_len);

  /*
   * Ismacryp, for example, uses 16 byte key + 8 byte 
   * salt  so this function is called with key_len = 24.
   * The check for key_len = 30/38/46 does not apply. Our usage
   * of aes functions with key_len = values other than 30
   * has not broken anything. Don't know what would be the
   * effect of skipping this check for srtp in general.
   */
  if (!(forIsmacryp && key_len > 16 && key_len < 30) &&
      key_len != 30 && key_len != 38 && key_len != 46)
    return err_status_bad_param;

  /* allocate memory a cipher of type aes_icm */
  tmp = (sizeof(aes_icm_ctx_t) + sizeof(cipher_t));
  pointer = (uint8_t*)crypto_alloc(tmp);
  if (pointer == NULL) 
    return err_status_alloc_fail;

  /* set pointers */
  *c = (cipher_t *)pointer;
  (*c)->type = &aes_icm;
  (*c)->state = pointer + sizeof(cipher_t);

  /* increment ref_count */
  aes_icm.ref_count++;

  /* set key size        */
  (*c)->key_len = key_len;

  return err_status_ok;  
}

err_status_t aes_icm_alloc(cipher_t **c, int key_len, int forIsmacryp) {
  return aes_icm_alloc_ismacryp(c, key_len, 0);
}

err_status_t
aes_icm_dealloc(cipher_t *c) {
  extern cipher_type_t aes_icm;

  /* zeroize entire state*/
  octet_string_set_to_zero((uint8_t *)c, 
			   sizeof(aes_icm_ctx_t) + sizeof(cipher_t));

  /* free memory */
  crypto_free(c);

  /* decrement ref_count */
  aes_icm.ref_count--;
  
  return err_status_ok;  
}


/*
 * aes_icm_context_init(...) initializes the aes_icm_context
 * using the value in key[].
 *
 * the key is the secret key 
 *
 * the salt is unpredictable (but not necessarily secret) data which
 * randomizes the starting point in the keystream
 */

err_status_t
aes_icm_context_init(aes_icm_ctx_t *c, const uint8_t *key, int key_len) {
  err_status_t status;
  int base_key_len, copy_len;

  if (key_len > 16 && key_len < 30) /* Ismacryp */
    base_key_len = 16;
  else if (key_len == 30 || key_len == 38 || key_len == 46)
    base_key_len = key_len - 14;
  else
    return err_status_bad_param;

  /* 
   * set counter and initial values to 'offset' value, being careful not to
   * go past the end of the key buffer.
   */
  v128_set_to_zero(&c->counter);
  v128_set_to_zero(&c->offset);

  /* force last two octets of the offset to be left zero 
   * (for srtp compatibility) */
  copy_len = key_len - base_key_len;
  
  memcpy(&c->counter, key + base_key_len, copy_len);
  memcpy(&c->offset, key + base_key_len, copy_len);

  debug_print(mod_aes_icm, 
	      "key:  %s", octet_string_hex_string(key, base_key_len)); 
  debug_print(mod_aes_icm, 
	      "offset: %s", v128_hex_string(&c->offset)); 

  /* expand key */
  status = aes_expand_encryption_key(key, base_key_len, &c->expanded_key);
  if (status) {
    v128_set_to_zero(&c->counter);
    v128_set_to_zero(&c->offset);
    return status;
  }

  /* indicate that the keystream_buffer is empty */
  c->bytes_in_buffer = 0;

  return err_status_ok;
}

/*
 * aes_icm_set_octet(c, i) sets the counter of the context which it is
 * passed so that the next octet of keystream that will be generated
 * is the ith octet
 */

err_status_t
aes_icm_set_octet(aes_icm_ctx_t *c,
		  uint64_t octet_num) {

#ifdef NO_64BIT_MATH
  int tail_num       = low32(octet_num) & 0x0f;
  /* 64-bit right-shift 4 */
  uint64_t block_num = make64(high32(octet_num) >> 4,
							  ((high32(octet_num) & 0x0f)<<(32-4)) |
							   (low32(octet_num) >> 4));
#else
  int tail_num       = (int)(octet_num % 16);
  uint64_t block_num = octet_num / 16;
#endif
  

  /* set counter value */
  /* FIX - There's no way this is correct */
  c->counter.v64[0] = c->offset.v64[0];
#ifdef NO_64BIT_MATH
  c->counter.v64[0] = make64(high32(c->offset.v64[0]) ^ high32(block_num),
							 low32(c->offset.v64[0])  ^ low32(block_num));
#else
  c->counter.v64[0] = c->offset.v64[0] ^ block_num;
#endif

  debug_print(mod_aes_icm, 
	      "set_octet: %s", v128_hex_string(&c->counter)); 

  /* fill keystream buffer, if needed */
  if (tail_num) {
    v128_copy(&c->keystream_buffer, &c->counter);
    aes_encrypt(&c->keystream_buffer, &c->expanded_key);
    c->bytes_in_buffer = sizeof(v128_t);

    debug_print(mod_aes_icm, "counter:    %s", 
	      v128_hex_string(&c->counter));
    debug_print(mod_aes_icm, "ciphertext: %s", 
	      v128_hex_string(&c->keystream_buffer));    
    
    /*  indicate number of bytes in keystream_buffer  */
    c->bytes_in_buffer = sizeof(v128_t) - tail_num;
  
  } else {
    
    /* indicate that keystream_buffer is empty */
    c->bytes_in_buffer = 0;
  }

  return err_status_ok;
}

/*
 * aes_icm_set_iv(c, iv) sets the counter value to the exor of iv with
 * the offset
 */

err_status_t
aes_icm_set_iv(aes_icm_ctx_t *c, void *iv) {
  v128_t *nonce = (v128_t *) iv;

  debug_print(mod_aes_icm, 
	      "setting iv: %s", v128_hex_string(nonce)); 
 
  v128_xor(&c->counter, &c->offset, nonce);
  
  debug_print(mod_aes_icm, 
	      "set_counter: %s", v128_hex_string(&c->counter)); 

  /* indicate that the keystream_buffer is empty */
  c->bytes_in_buffer = 0;

  return err_status_ok;
}



/*
 * aes_icm_advance(...) refills the keystream_buffer and
 * advances the block index of the sicm_context forward by one
 *
 * this is an internal, hopefully inlined function
 */
  
static INLINE void
aes_icm_advance_ismacryp(aes_icm_ctx_t *c, uint8_t forIsmacryp) {
  /* fill buffer with new keystream */
  v128_copy(&c->keystream_buffer, &c->counter);
  aes_encrypt(&c->keystream_buffer, &c->expanded_key);
  c->bytes_in_buffer = sizeof(v128_t);

  debug_print(mod_aes_icm, "counter:    %s", 
	      v128_hex_string(&c->counter));
  debug_print(mod_aes_icm, "ciphertext: %s", 
	      v128_hex_string(&c->keystream_buffer));    
  
  /* clock counter forward */

  if (forIsmacryp) {
    uint32_t temp;    
    //alex's clock counter forward
    temp = ntohl(c->counter.v32[3]);
    c->counter.v32[3] = htonl(++temp);
  } else {
    if (!++(c->counter.v8[15])) 
      ++(c->counter.v8[14]);
  }
}

static INLINE void aes_icm_advance(aes_icm_ctx_t *c) {
  aes_icm_advance_ismacryp(c, 0);
}


/*e
 * icm_encrypt deals with the following cases:
 *
 * bytes_to_encr < bytes_in_buffer
 *  - add keystream into data
 *
 * bytes_to_encr > bytes_in_buffer
 *  - add keystream into data until keystream_buffer is depleted
 *  - loop over blocks, filling keystream_buffer and then
 *    adding keystream into data
 *  - fill buffer then add in remaining (< 16) bytes of keystream 
 */

err_status_t
aes_icm_encrypt_ismacryp(aes_icm_ctx_t *c,
              unsigned char *buf, unsigned int *enc_len, 
              int forIsmacryp) {
  unsigned int bytes_to_encr = *enc_len;
  unsigned int i;
  uint32_t *b;

  /* check that there's enough segment left but not for ismacryp*/
  if (!forIsmacryp && (bytes_to_encr + htons(c->counter.v16[7])) > 0xffff)
    return err_status_terminus;

 debug_print(mod_aes_icm, "block index: %d", 
           htons(c->counter.v16[7]));
  if (bytes_to_encr <= (unsigned int)c->bytes_in_buffer) {
    
    /* deal with odd case of small bytes_to_encr */
    for (i = (sizeof(v128_t) - c->bytes_in_buffer);
		 i < (sizeof(v128_t) - c->bytes_in_buffer + bytes_to_encr); i++) 
	{
      *buf++ ^= c->keystream_buffer.v8[i];
	}

    c->bytes_in_buffer -= bytes_to_encr;

    /* return now to avoid the main loop */
    return err_status_ok;

  } else {
    
    /* encrypt bytes until the remaining data is 16-byte aligned */    
    for (i=(sizeof(v128_t) - c->bytes_in_buffer); i < sizeof(v128_t); i++) 
      *buf++ ^= c->keystream_buffer.v8[i];

    bytes_to_encr -= c->bytes_in_buffer;
    c->bytes_in_buffer = 0;

  }
  
  /* now loop over entire 16-byte blocks of keystream */
  for (i=0; i < (bytes_to_encr/sizeof(v128_t)); i++) {

    /* fill buffer with new keystream */
    aes_icm_advance_ismacryp(c, forIsmacryp);

    /*
     * add keystream into the data buffer (this would be a lot faster
     * if we could assume 32-bit alignment!)
     */

#if ALIGN_32
    b = (uint32_t *)buf;
    *b++ ^= c->keystream_buffer.v32[0];
    *b++ ^= c->keystream_buffer.v32[1];
    *b++ ^= c->keystream_buffer.v32[2];
    *b++ ^= c->keystream_buffer.v32[3];
    buf = (uint8_t *)b;
#else    
    if ((((unsigned long) buf) & 0x03) != 0) {
      *buf++ ^= c->keystream_buffer.v8[0];
      *buf++ ^= c->keystream_buffer.v8[1];
      *buf++ ^= c->keystream_buffer.v8[2];
      *buf++ ^= c->keystream_buffer.v8[3];
      *buf++ ^= c->keystream_buffer.v8[4];
      *buf++ ^= c->keystream_buffer.v8[5];
      *buf++ ^= c->keystream_buffer.v8[6];
      *buf++ ^= c->keystream_buffer.v8[7];
      *buf++ ^= c->keystream_buffer.v8[8];
      *buf++ ^= c->keystream_buffer.v8[9];
      *buf++ ^= c->keystream_buffer.v8[10];
      *buf++ ^= c->keystream_buffer.v8[11];
      *buf++ ^= c->keystream_buffer.v8[12];
      *buf++ ^= c->keystream_buffer.v8[13];
      *buf++ ^= c->keystream_buffer.v8[14];
      *buf++ ^= c->keystream_buffer.v8[15];
    } else {
      b = (uint32_t *)buf;
      *b++ ^= c->keystream_buffer.v32[0];
      *b++ ^= c->keystream_buffer.v32[1];
      *b++ ^= c->keystream_buffer.v32[2];
      *b++ ^= c->keystream_buffer.v32[3];
      buf = (uint8_t *)b;
    }
#endif /* #if ALIGN_32 */

  }
  
  /* if there is a tail end of the data, process it */
  if ((bytes_to_encr & 0xf) != 0) {
    
    /* fill buffer with new keystream */
    aes_icm_advance_ismacryp(c, forIsmacryp);
    
    for (i=0; i < (bytes_to_encr & 0xf); i++)
      *buf++ ^= c->keystream_buffer.v8[i];
    
    /* reset the keystream buffer size to right value */
    c->bytes_in_buffer = sizeof(v128_t) - i;  
  } else {

    /* no tail, so just reset the keystream buffer size to zero */
    c->bytes_in_buffer = 0;

  }

  return err_status_ok;
}

err_status_t
aes_icm_encrypt(aes_icm_ctx_t *c, unsigned char *buf, unsigned int *enc_len) {
  return aes_icm_encrypt_ismacryp(c, buf, enc_len, 0);
}

err_status_t
aes_icm_output(aes_icm_ctx_t *c, uint8_t *buffer, int num_octets_to_output) {
  unsigned int len = num_octets_to_output;
  
  /* zeroize the buffer */
  octet_string_set_to_zero(buffer, num_octets_to_output);
  
  /* exor keystream into buffer */
  return aes_icm_encrypt(c, buffer, &len);
}


char 
aes_icm_description[] = "aes integer counter mode";

uint8_t aes_icm_test_case_0_key[30] = {
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd
};

uint8_t aes_icm_test_case_0_nonce[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t aes_icm_test_case_0_plaintext[32] =  {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

uint8_t aes_icm_test_case_0_ciphertext[32] = {
  0xe0, 0x3e, 0xad, 0x09, 0x35, 0xc9, 0x5e, 0x80,
  0xe1, 0x66, 0xb1, 0x6d, 0xd9, 0x2b, 0x4e, 0xb4,
  0xd2, 0x35, 0x13, 0x16, 0x2b, 0x02, 0xd0, 0xf7,
  0x2a, 0x43, 0xa2, 0xfe, 0x4a, 0x5f, 0x97, 0xab
};

cipher_test_case_t aes_icm_test_case_0 = {
  30,                                    /* octets in key            */
  aes_icm_test_case_0_key,               /* key                      */
  aes_icm_test_case_0_nonce,             /* packet index             */
  32,                                    /* octets in plaintext      */
  aes_icm_test_case_0_plaintext,         /* plaintext                */
  32,                                    /* octets in ciphertext     */
  aes_icm_test_case_0_ciphertext,        /* ciphertext               */
  NULL                                   /* pointer to next testcase */
};

uint8_t aes_icm_test_case_1_key[46] = {
  0x57, 0xf8, 0x2f, 0xe3, 0x61, 0x3f, 0xd1, 0x70,
  0xa8, 0x5e, 0xc9, 0x3c, 0x40, 0xb1, 0xf0, 0x92,
  0x2e, 0xc4, 0xcb, 0x0d, 0xc0, 0x25, 0xb5, 0x82,
  0x72, 0x14, 0x7c, 0xc4, 0x38, 0x94, 0x4a, 0x98,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd
};

uint8_t aes_icm_test_case_1_nonce[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t aes_icm_test_case_1_plaintext[32] =  {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

uint8_t aes_icm_test_case_1_ciphertext[32] = {
  0x92, 0xbd, 0xd2, 0x8a, 0x93, 0xc3, 0xf5, 0x25,
  0x11, 0xc6, 0x77, 0xd0, 0x8b, 0x55, 0x15, 0xa4,
  0x9d, 0xa7, 0x1b, 0x23, 0x78, 0xa8, 0x54, 0xf6,
  0x70, 0x50, 0x75, 0x6d, 0xed, 0x16, 0x5b, 0xac
};

cipher_test_case_t aes_icm_test_case_1 = {
  46,                                    /* octets in key            */
  aes_icm_test_case_1_key,               /* key                      */
  aes_icm_test_case_1_nonce,             /* packet index             */
  32,                                    /* octets in plaintext      */
  aes_icm_test_case_1_plaintext,         /* plaintext                */
  32,                                    /* octets in ciphertext     */
  aes_icm_test_case_1_ciphertext,        /* ciphertext               */
  &aes_icm_test_case_0                   /* pointer to next testcase */
};



/*
 * note: the encrypt function is identical to the decrypt function
 */

cipher_type_t aes_icm = {
  (cipher_alloc_func_t)          aes_icm_alloc,
  (cipher_dealloc_func_t)        aes_icm_dealloc,  
  (cipher_init_func_t)           aes_icm_context_init,
  (cipher_encrypt_func_t)        aes_icm_encrypt,
  (cipher_decrypt_func_t)        aes_icm_encrypt,
  (cipher_set_iv_func_t)         aes_icm_set_iv,
  (char *)                       aes_icm_description,
  (int)                          0,   /* instance count */
  (cipher_test_case_t *)        &aes_icm_test_case_1,
  (debug_module_t *)            &mod_aes_icm,
  (cipher_type_id_t)             AES_ICM
};

