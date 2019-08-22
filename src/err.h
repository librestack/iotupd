/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2019 Brett Sheffield <brett@gladserv.com> */

#ifndef __IOTD_ERR_H__
#define __IOTD_ERR_H__ 1

#include <errno.h>

#define IOTD_ERROR_CODES(X) \
	X(IOTD_ERROR_SUCCESS,             "Success") \
	X(IOTD_ERROR_FAILURE,             "Failure") \
	X(IOTD_ERROR_PID_OPEN,            "Failed to open pidfile") \
	X(IOTD_ERROR_PID_READFAIL,        "Failed to read pidfile") \
	X(IOTD_ERROR_PID_INVALID,         "Invalid pid") \
	X(IOTD_ERROR_ALREADY_RUNNING,     "Daemon already running") \
	X(IOTD_ERROR_PID_WRITEFAIL,       "Failed to write to pidfile") \
	X(IOTD_ERROR_DAEMON_FAILURE,      "Failed to daemonize") \
	X(IOTD_ERROR_CONFIG_NOTNUMERIC,   "Numeric config value not numeric") \
	X(IOTD_ERROR_CONFIG_BOUNDS,       "Numeric config value out of bounds") \
	X(IOTD_ERROR_CONFIG_BOOLEAN,     "Invalid boolean config value") \
	X(IOTD_ERROR_CONFIG_READFAIL,    "Unable to read config file") \
	X(IOTD_ERROR_CONFIG_INVALID,     "Error in config file") \
	X(IOTD_ERROR_MALLOC,             "Memory allocation error") \
	X(IOTD_ERROR_INVALID_ARGS,       "Invalid command line options") \
	X(IOTD_ERROR_DAEMON_STOPPED,     "Daemon not running") \
	X(IOTD_ERROR_NET_SEND,           "Error sending data") \
	X(IOTD_ERROR_NET_RECV,           "Error receiving data") \
	X(IOTD_ERROR_NET_SOCKOPT,        "Error setting socket options") \
	X(IOTD_ERROR_CMD_INVALID,        "Invalid Command received") \
	X(IOTD_ERROR_SOCKET_CREATE,      "Unable to create unix socket") \
	X(IOTD_ERROR_SOCKET_CONNECT,     "Unable to connect to unix socket") \
	X(IOTD_ERROR_SOCKET_BIND,        "Unable to bind to unix socket") \
	X(IOTD_ERROR_MCAST_JOIN,         "Multicast join failed") \
	X(IOTD_ERROR_MCAST_LEAVE,        "Multicast leave failed") \
	X(IOTD_ERROR_SOCKET_LISTENING,   "Socket already listening") \
	X(IOTD_ERROR_BRIDGE_INIT,        "Unable to setup bridge control") \
	X(IOTD_ERROR_BRIDGE_EXISTS,      "Bridge already exists") \
	X(IOTD_ERROR_BRIDGE_ADD_FAIL,    "Bridge creation failed") \
	X(IOTD_ERROR_TAP_ADD_FAIL,       "TAP creation failed") \
	X(IOTD_ERROR_BRIDGE_NODEV,       "Bridge does not exist") \
	X(IOTD_ERROR_IF_NODEV,           "Interface does not exist") \
	X(IOTD_ERROR_IF_BUSY,            "Interface already bridged") \
	X(IOTD_ERROR_IF_LOOP,            "Interface is a bridge") \
	X(IOTD_ERROR_IF_OPNOTSUPP,       "Interface does not support bridging") \
	X(IOTD_ERROR_IF_BRIDGE_FAIL,     "Unable to bridge interface") \
	X(IOTD_ERROR_SOCK_IOCTL,         "Unable to create ioctl socket") \
	X(IOTD_ERROR_IF_UP_FAIL,         "Unable to bring up interface") \
	X(IOTD_ERROR_CTX_REQUIRED,       "context required for this operation") \
	X(IOTD_ERROR_INVALID_BASEADDR,   "Invalid hashgroup baseaddr") \
	X(IOTD_ERROR_RANDOM_OPEN,        "Unable to open random source") \
	X(IOTD_ERROR_RANDOM_READ,        "Unable to read random source") \
	X(IOTD_ERROR_HASH_INIT,          "Unable to initialize hash") \
	X(IOTD_ERROR_HASH_UPDATE,        "Unable to hash data") \
	X(IOTD_ERROR_HASH_FINAL,         "Unable to finalize hash") \
	X(IOTD_ERROR_DB_OPEN,            "Unable to open database") \
	X(IOTD_ERROR_DB_EXEC,            "Error executing database operation") \
	X(IOTD_ERROR_DB_REQUIRED,        "Database required") \
	X(IOTD_ERROR_DB_KEYNOTFOUND,     "Requested key not found in database") \
	X(IOTD_ERROR_SOCKET_REQUIRED,    "socket required for this operation") \
	X(IOTD_ERROR_CHANNEL_REQUIRED,   "channel required for this operation") \
	X(IOTD_ERROR_MESSAGE_REQUIRED,   "message required for this operation") \
	X(IOTD_ERROR_MESSAGE_EMPTY,      "message has no payload") \
	X(IOTD_ERROR_INVALID_PARAMS,     "Invalid arguments to function") \
	X(IOTD_ERROR_MSG_ATTR_UNKNOWN,   "Unknown message attribute") \
	X(IOTD_ERROR_THREAD_CANCEL,      "Failed to cancel thread") \
	X(IOTD_ERROR_THREAD_JOIN,        "Failed to join thread") \
	X(IOTD_ERROR_INVALID_OPCODE,     "Invalid opcode") \
	X(IOTD_ERROR_FILE_OPEN_FAIL,     "Unable to open file") \
	X(IOTD_ERROR_FILE_STAT_FAIL,     "Unable to open file") \
	X(IOTD_ERROR_MMAP_FAIL,          "Unable to map file")
#undef X

#define IOTD_ERROR_MSG(name, msg) case name: return msg;
#define IOTD_ERROR_ENUM(name, msg) name,
enum {
	IOTD_ERROR_CODES(IOTD_ERROR_ENUM)
};

/* log message and return code */
int err_log(int level, int e);

/* return human readable error message for e */
char *err_msg(int e);

/* print human readable error, using errsv (errno) or progam defined (e) code */
void err_print(int e, int errsv, char *errstr);

#endif /* __IOTD_ERR_H__ */
