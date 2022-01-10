/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2020-2022 Brett Sheffield <bacs@librecast.net> */

#ifndef _MLD_H
#define _MLD_H 1

#include <librecast.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <immintrin.h>
#include "vec.h"

#define BLOOM_SZ 16777216
#define BLOOM_VECTORS BLOOM_SZ / VECTOR_BITS
#define BLOOM_HASHES 8 /* optimal = LOG2(BLOOM_SZ / ENTRIES) */
#define MLD_TIMEOUT 125 /* seconds before MLD record expires */
#define MLD_TIMER_INTERVAL 1 /* length of timer tick in seconds */
#define IPV6_BYTES 16

/* See RFC 3810 */

/* MALI = Multicast Address Listening Interval */
/* LLQT = Last Listener Query Time */

#define MLD2_ROBUSTNESS 2               /* 9.14.1.  Robustness Variable */
#define MLD2_CAPABLE_ROUTERS "ff02::16" /* all MLDv2-capable routers */
#define MLD2_LISTEN_REPORT 143          /* Multicast Listener Report messages */
#define MLD2_QI MLD_TIMEOUT		/* Query Interval */
#define MLD2_QRI 10000			/* Query Response Interval */

/* Current State Record */
#define MODE_IS_INCLUDE 1
#define MODE_IS_EXCLUDE 2

/* Filter Mode Change Record */
#define CHANGE_TO_INCLUDE_MODE 3
#define CHANGE_TO_EXCLUDE_MODE 4

/* Source List Change Record */
#define ALLOW_NEW_SOURCES 5
#define BLOCK_OLD_SOURCES 6

/* 9.14.1.  Robustness Variable */
#define MLD2_ROBUSTNESS 2

/* flags */
enum {
	MLD_DONTWAIT = 1
};

/* Event Types */
typedef enum {
	MLD_EVENT_JOIN = 1,
	MLD_EVENT_PART = 2,
	MLD_EVENT_MAX
} mld_event_type_t;
#define MLD_EVENT_ALL ((MLD_EVENT_MAX - 1) << 1) - 1

/* port (or service) to use for MLD event notifications */
#define MLD_EVENT_SERV 4242

#define aitoin6(ai) &(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr)

typedef struct mld_s mld_t;
typedef struct mld_addr_rec_s mld_addr_rec_t;
typedef struct mld_filter_s mld_filter_t;
typedef struct mld_timerjob_s mld_timerjob_t;
typedef struct mld_watch_s mld_watch_t;

struct mld_watch_s {
	mld_t *mld;
	pthread_t thread;
	lc_socket_t *sock;
	lc_channel_t *chan;
	void (*f)(mld_watch_t *, mld_watch_t *);
	void *arg;
	struct in6_addr *grp;
	unsigned int ifx;
	int events;
	int flags;
};

/* initialize / free state machine */
mld_t *mld_init(int ifaces);

/* free MLD objects */
void mld_free(mld_t *mld);

/* start MLD snooping. will stop when *cont zero (pass NULL to ignore) */
mld_t *mld_start(volatile int *cont, unsigned int iface);

/* stop MLD snooping */
void mld_stop(mld_t *mld);

/* return Librecast context for mld */
lc_ctx_t *mld_lctx(mld_t *mld);

/* return number of interfaces */
unsigned int mld_ifaces(mld_t *mld);

/* convert network index to mld filter array index */
unsigned int mld_idx_iface(mld_t *mld, unsigned int idx);

/* create notification channel */
lc_channel_t *mld_notification_channel(mld_t *mld, struct in6_addr *addr, int events);

/* decrement all the counters. */
void mld_timer_tick(mld_t *mld, unsigned int iface, size_t idx, uint8_t val);

/* set specific timer to val */
void mld_timer_set(mld_t *mld, unsigned int iface, size_t idx, uint8_t val);

/* reset specific timer to MLD_TIMEOUT */
void mld_timer_refresh(mld_t *mld, unsigned int iface, size_t idx, uint8_t val);

/* get/set timer for group address */
int mld_filter_timer_get(mld_t *mld, unsigned int iface, struct in6_addr *saddr);
int mld_filter_timer_set(mld_t *mld, unsigned int iface, struct in6_addr *saddr, uint8_t val);

/* add group address to interface bloom filter */
int mld_filter_grp_add(mld_t *mld, unsigned int iface, struct in6_addr *addr);
int mld_filter_grp_add_ai(mld_t *mld, unsigned int iface, struct addrinfo *ai);

/* return true (-1) if filter contains addr, false (0) if not */
int mld_filter_grp_cmp(mld_t *mld, unsigned int iface, struct in6_addr *addr);

/* remove group address from interface bloom filter */
int mld_filter_grp_del(mld_t *mld, unsigned int iface, struct in6_addr *addr);
int mld_filter_grp_del_ai(mld_t *mld, unsigned int iface, struct addrinfo *ai);

/* return 0 if addr is assigned to a local interface, 1 if not, -1 on error */
int mld_thatsme(struct in6_addr *addr);

/* handle MLD2 router msgs */
void mld_address_record(mld_t *mld, unsigned int iface, mld_addr_rec_t *rec);
void mld_listen_report(mld_t *mld, struct msghdr *msg);
void mld_msg_handle(mld_t *mld, struct msghdr *msg);

/* start mld listener. return -1 on error, 0 success */
int mld_listen(mld_t *mld);

/* query state */

/* block until notification received for addr on interface index ifx. If ifx is
 * zero, all interfaces are watched.  Returns 0 on success, or -1 on error and
 * errno is set to indicate the error.
 *
 * The flags argument is formed by ORing one or more of the following values:
 *
 *  MLD_DONTWAIT
 *      Enables  nonblocking  operation; if the operation would block, the call
 *      fails with the error EWOULDBLOCK.*/
int mld_wait(mld_t *mld, unsigned int ifx, struct in6_addr *addr, int flags);

/* Allocate new watch on specifed interface ifx (0 = all interfaces), grp and
 * events.  Free with mld_watch_free()
 * f is a function pointer to a function with two watch args.  Unless NULL, the
 * first will be a pointer to a watch populated with the event, the second will
 * be the original watch */
mld_watch_t *mld_watch_init(mld_t *mld, unsigned int ifx, struct in6_addr *grp, int events,
	void (*f)(mld_watch_t *, mld_watch_t *), void *arg, int flags);

/* start watch */
int mld_watch_start(mld_watch_t *watch);

/* mld_watch_init() + mld_watch_start() */
mld_watch_t *mld_watch(mld_t *mld, unsigned int ifx, struct in6_addr *grp, int events,
	void (*f)(mld_watch_t *, mld_watch_t *), void *arg, int flags);

/* stop watch. watch is still valid, can be started again with mld_watch_start() */
int mld_watch_stop(mld_watch_t *watch);

/* shorthand for mld_watch_stop() + mld_watch_free() */
int mld_watch_cancel(mld_watch_t *watch);

/* free watch */
void mld_watch_free(mld_watch_t *watch);

#endif /* _MLD_H */
