# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Brett Sheffield <brett@gladserv.com>

PREFIX = /usr/local
export PREFIX

CFLAGS += -O -Wall -g
export CFLAGS

BIN_PATH = $(PREFIX)/sbin
export BIN_PATH

.PHONY: all clean src payloads

all:	src

clean:
	@$(MAKE) -C src $@

realclean:
	@$(MAKE) -C src $@

install:
	@$(MAKE) -C src $@

src:
	@$(MAKE) -C src all

payloads:
	dd if=/dev/urandom of=/tmp/testfile.512 bs=1024 count=524288
	dd if=/dev/urandom of=/tmp/testfile.1G bs=1024 count=1048576
	dd if=/dev/urandom of=/tmp/testfile.2G bs=1024 count=2097152
	dd if=/dev/urandom of=/tmp/testfile.3G bs=1024 count=3145728

