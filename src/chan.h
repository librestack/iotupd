/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2021 Claudio Calvelli <clc@w42.org> */

#ifndef __IOTD_CHAN_H__
#define __IOTD_CHAN_H__ 1

#include <netinet/in.h>

/* get multicast group from strings; currently it only works if the port is
 * the librecast default, so we don't allow to specify a value */
int get_channel (const char * group, /* const char * port, */ struct sockaddr_in6 * addr);

#endif /* __IOTD_CHAN_H__ */
