# SPDX-License-Identifier: GPL-2.0-only

PREFIX = /usr/local
export PREFIX

CFLAGS += -O -Wall -Werror -g
export CFLAGS

.PHONY: all clean src

all:	src

clean:
	@$(MAKE) -C src $@

install:
	@$(MAKE) -C src $@

src:
	@$(MAKE) -C src all

