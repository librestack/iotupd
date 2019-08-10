# SPDX-License-Identifier: GPL-2.0-or-later

PREFIX = /usr/local
export PREFIX

CFLAGS += -O -Wall -Werror -g
export CFLAGS

BIN_PATH = $(PREFIX)/sbin
export BIN_PATH

.PHONY: all clean src

all:	src

clean:
	@$(MAKE) -C src $@

realclean:
	@$(MAKE) -C src $@

install:
	@$(MAKE) -C src $@

src:
	@$(MAKE) -C src all

