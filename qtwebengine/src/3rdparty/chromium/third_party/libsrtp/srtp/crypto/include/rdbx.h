/*
 * rdbx.h
 *
 * replay database with extended packet indices, using a rollover counter
 *
 * David A. McGrew
 * Cisco Systems, Inc.
 *
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

#ifndef RDBX_H
#define RDBX_H

#include "datatypes.h"
#include "err.h"

/* #define ROC_TEST */  

#ifndef ROC_TEST

typedef uint16_t sequence_number_t;   /* 16 bit sequence number  */
typedef uint32_t rollover_counter_t;   /* 32 bit rollover counter */

#else  /* use small seq_num and roc datatypes for testing purposes */

typedef unsigned char sequence_number_t;         /* 8 bit sequence number   */
typedef uint16_t rollover_counter_t;   /* 16 bit rollover counter */

#endif

#define seq_num_median (1 << (8*sizeof(sequence_number_t) - 1))
#define seq_num_max    (1 << (8*sizeof(sequence_number_t)))

/*
 * An xtd_seq_num_t is a 64-bit unsigned integer used as an 'extended'
 * sequence number.  
 */

typedef uint64_t xtd_seq_num_t;


/*
 * An rdbx_t is a replay database with extended range; it uses an
 * xtd_seq_num_t and a bitmask of recently received indices.
 */

typedef struct {
  xtd_seq_num_t index;
  bitvector_t bitmask;
} rdbx_t;


/*
 * rdbx_init(rdbx_ptr, ws)
 *
 * initializes the rdbx pointed to by its argument with the window size ws,
 * setting the rollover counter and sequence number to zero
 */

err_status_t
rdbx_init(rdbx_t *rdbx, unsigned long ws);


/*
 * rdbx_dealloc(rdbx_ptr)
 *
 * frees memory associated with the rdbx
 */

err_status_t
rdbx_dealloc(rdbx_t *rdbx);


/*
 * rdbx_estimate_index(rdbx, guess, s)
 * 
 * given an rdbx and a sequence number s (from a newly arrived packet),
 * sets the contents of *guess to contain the best guess of the packet
 * index to which s corresponds, and returns the difference between
 * *guess and the locally stored synch info
 */

int
rdbx_estimate_index(const rdbx_t *rdbx,
		    xtd_seq_num_t *guess,
		    sequence_number_t s);

/*
 * rdbx_check(rdbx, delta);
 *
 * rdbx_check(&r, delta) checks to see if the xtd_seq_num_t
 * which is at rdbx->window_start + delta is in the rdb
 *
 */

err_status_t
rdbx_check(const rdbx_t *rdbx, int difference);

/*
 * replay_add_index(rdbx, delta)
 * 
 * adds the xtd_seq_num_t at rdbx->window_start + delta to replay_db
 * (and does *not* check if that xtd_seq_num_t appears in db)
 *
 * this function should be called *only* after replay_check has
 * indicated that the index does not appear in the rdbx, and a mutex
 * should protect the rdbx between these calls if necessary.
 */

err_status_t
rdbx_add_index(rdbx_t *rdbx, int delta);


/*
 * rdbx_set_roc(rdbx, roc) initalizes the rdbx_t at the location rdbx
 * to have the rollover counter value roc.  If that value is less than
 * the current rollover counter value, then the function returns
 * err_status_replay_old; otherwise, err_status_ok is returned.
 * 
 */

err_status_t
rdbx_set_roc(rdbx_t *rdbx, uint32_t roc);

/*
 * rdbx_get_roc(rdbx) returns the value of the rollover counter for
 * the rdbx_t pointed to by rdbx
 * 
 */

xtd_seq_num_t
rdbx_get_packet_index(const rdbx_t *rdbx);

/*
 * xtd_seq_num_t functions - these are *internal* functions of rdbx, and
 * shouldn't be used to manipulate rdbx internal values.  use the rdbx
 * api instead!
 */

/*
 * rdbx_get_ws(rdbx_ptr)
 *
 * gets the window size which was used to initialize the rdbx
 */

unsigned long
rdbx_get_window_size(const rdbx_t *rdbx);


/* index_init(&pi) initializes a packet index pi (sets it to zero) */

void
index_init(xtd_seq_num_t *pi);

/* index_advance(&pi, s) advances a xtd_seq_num_t forward by s */

void
index_advance(xtd_seq_num_t *pi, sequence_number_t s);


/*
 * index_guess(local, guess, s)
 * 
 * given a xtd_seq_num_t local (which represents the highest
 * known-to-be-good index) and a sequence number s (from a newly
 * arrived packet), sets the contents of *guess to contain the best
 * guess of the packet index to which s corresponds, and returns the
 * difference between *guess and *local
 */

int
index_guess(const xtd_seq_num_t *local,
		   xtd_seq_num_t *guess,
		   sequence_number_t s);


#endif /* RDBX_H */









