/*-
 * Copyright (c) 2009-2010 Brad Penoff
 * Copyright (c) 2009-2010 Humaira Kamal
 * Copyright (c) 2011-2012 Irene Ruengeler
 * Copyright (c) 2011-2012 Michael Tuexen
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _USER_ENVIRONMENT_H_
#define _USER_ENVIRONMENT_H_
/* __Userspace__ */
#include <sys/types.h>

#ifdef __Userspace_os_FreeBSD
#ifndef _SYS_MUTEX_H_
#include <sys/mutex.h>
#endif
#endif
#if defined (__Userspace_os_Windows)
#include "netinet/sctp_os_userspace.h"
#endif

/* maxsockets is used in SCTP_ZONE_INIT call. It refers to
 * kern.ipc.maxsockets kernel environment variable.
 */
extern int maxsockets;

/* int hz; is declared in sys/kern/subr_param.c and refers to kernel timer frequency.
 * See http://ivoras.sharanet.org/freebsd/vmware.html for additional info about kern.hz
 * hz is initialized in void init_param1(void) in that file.
 */
extern int hz;


/* The following two ints define a range of available ephermal ports. */
extern int ipport_firstauto, ipport_lastauto;

/* nmbclusters is used in sctp_usrreq.c (e.g., sctp_init). In the FreeBSD kernel,
 *  this is 1024 + maxusers * 64.
 */
extern int nmbclusters;

#if !defined (__Userspace_os_Windows)
#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

extern int read_random(void *buf, int count);

/* errno's may differ per OS.  errno.h now included in sctp_os_userspace.h */
/* Source: /usr/src/sys/sys/errno.h */
/* #define	ENOSPC		28 */		/* No space left on device */
/* #define	ENOBUFS		55 */		/* No buffer space available */
/* #define	ENOMEM		12 */		/* Cannot allocate memory */
/* #define	EACCES		13 */		/* Permission denied */
/* #define	EFAULT		14 */		/* Bad address */
/* #define	EHOSTDOWN	64 */		/* Host is down */
/* #define	EHOSTUNREACH	65 */		/* No route to host */

/* Source ip_output.c. extern'd in ip_var.h */
extern u_short ip_id;

#if defined(__Userspace_os_Linux)
#define IPV6_VERSION            0x60
#endif
#if defined(INVARIANTS)
#define panic(args...)            \
	do {                      \
		SCTP_PRINTF(args);\
		exit(1);          \
} while (0)
#endif

#if defined(INVARIANTS)
#define KASSERT(cond, args)          \
	do {                         \
		if (!(cond)) {       \
			printf args ;\
			exit(1);     \
		}                    \
	} while (0)
#else
#define KASSERT(cond, args)
#endif

/* necessary for sctp_pcb.c */
extern int ip_defttl;


/* dummy definitions used (temporarily?) for inpcb userspace port */

/* called in sctp_usrreq.c */
#define in6_sin_2_v4mapsin6(arg1, arg2) /* STUB */

#endif
