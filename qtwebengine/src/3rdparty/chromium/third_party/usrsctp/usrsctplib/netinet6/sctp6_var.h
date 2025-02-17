/*-
 * Copyright (c) 2001-2007, by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2008-2012, by Randall Stewart. All rights reserved.
 * Copyright (c) 2008-2012, by Michael Tuexen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet6/sctp6_var.h 243186 2012-11-17 20:04:04Z tuexen $");
#endif

#ifndef _NETINET6_SCTP6_VAR_H_
#define _NETINET6_SCTP6_VAR_H_

#if defined(__Userspace__)
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *sin6);
#endif
#if defined(_KERNEL)

#if defined(__FreeBSD__) || (__APPLE__) || defined(__Windows__)
SYSCTL_DECL(_net_inet6_sctp6);
extern struct pr_usrreqs sctp6_usrreqs;
#else
int sctp6_usrreq(struct socket *, int, struct mbuf *, struct mbuf *, struct mbuf *);
#endif

#if defined(__APPLE__)
int sctp6_input(struct mbuf **, int *);
int sctp6_input_with_port(struct mbuf **, int *, uint16_t);
#elif defined(__Panda__)
int sctp6_input (pakhandle_type *);
#elif defined(__FreeBSD__) && __FreeBSD_version < 902000
int sctp6_input __P((struct mbuf **, int *, int));
int sctp6_input_with_port __P((struct mbuf **, int *, uint16_t));
#else
int sctp6_input(struct mbuf **, int *, int);
int sctp6_input_with_port(struct mbuf **, int *, uint16_t);
#endif
#if defined(__FreeBSD__) && __FreeBSD_version < 902000
int sctp6_output
__P((struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
     struct mbuf *, struct proc *));
void sctp6_ctlinput __P((int, struct sockaddr *, void *));
#else
int sctp6_output(struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
                 struct mbuf *, struct proc *);
void sctp6_ctlinput(int, struct sockaddr *, void *);
#endif
#if !(defined(__FreeBSD__) || defined(__APPLE__))
extern void in6_sin_2_v4mapsin6(struct sockaddr_in *sin,
				struct sockaddr_in6 *sin6);
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *sin6);
extern void in6_sin6_2_sin_in_sock(struct sockaddr *nam);
#endif
extern void sctp6_notify(struct sctp_inpcb *, struct icmp6_hdr *,
                         struct sctphdr *, struct sockaddr *,
                         struct sctp_tcb *, struct sctp_nets *);
#endif
#endif
