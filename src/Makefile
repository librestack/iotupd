# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019-2021 Brett Sheffield <brett@librecast.net>

PROGRAM:=iotup
CLIENT:=$(PROGRAM)c
SERVER:=$(PROGRAM)d
OBJECTS:=err.o job.o log.o misc.o mld.o chan.o vec.o
LIBS:=-llibrecast -pthread

.PHONY:		all clean realclean

all:		$(CLIENT) $(SERVER)

$(CLIENT):	$(OBJECTS) $(CLIENT).o $(OBJECTS)
		$(CC) $(LDFLAGS) -o $(CLIENT) $(CLIENT).o $(OBJECTS) $(LIBS)

$(SERVER):	$(OBJECTS) $(SERVER).o $(OBJECTS)
		$(CC) $(LDFLAGS) -o $(SERVER) $(SERVER).o $(OBJECTS) $(LIBS)

$(CLIENT).o:	$(CLIENT).c $(OBJECTS)

$(SERVER).o:	$(SERVER).c $(OBJECTS)

err.o:		err.c err.h

log.o:		log.c log.h

job.o:		job.c job.h

mld.o:		mld.c mld.h mld_pvt.h

misc.o:		misc.c misc.h

chan.o:		chan.c chan.h

vec.o:		vec.c vec.h

clean:
		rm -f *.o

realclean:
		rm -f *.o $(CLIENT) $(SERVER)

install:	all
		install -d $(BIN_PATH)
		install $(CLIENT) $(SERVER) $(BIN_PATH)/
