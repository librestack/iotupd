# SPDX-License-Identifier: GPL-2.0-only

PROGRAM:=iotup
CLIENT:=$(PROGRAM)c
SERVER:=$(PROGRAM)d
OBJECTS:=$(SERVER).o err.o log.o misc.o
LIBS:=-llibrecast

.PHONY:		all clean

all:		$(SERVER)

$(SERVER):	$(OBJECTS)
		$(CC) $(LDFLAGS) -o $(SERVER) $(OBJECTS) $(LIBS)

$(SERVER).o:	$(SERVER).c $(SERVER).h

err.o:		err.c err.h

log.o:		log.c log.h

misc.o:		misc.c misc.h

clean:
		rm -f *.o $(SERVER)
