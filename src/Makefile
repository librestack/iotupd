# SPDX-License-Identifier: GPL-2.0-only

PROGRAM:=iotup
CLIENT:=$(PROGRAM)c
SERVER:=$(PROGRAM)d
OBJECTS:=err.o log.o misc.o
LIBS:=-llibrecast

.PHONY:		all clean

all:		$(CLIENT) $(SERVER)

$(CLIENT):	$(OBJECTS) $(CLIENT).o
		$(CC) $(LDFLAGS) -o $(CLIENT) $(CLIENT).o $(OBJECTS) $(LIBS)

$(SERVER):	$(OBJECTS) $(SERVER).o
		$(CC) $(LDFLAGS) -o $(SERVER) $(SERVER).o $(OBJECTS) $(LIBS)

$(CLIENT).o:	$(CLIENT).c $(CLIENT).h iot.h

$(SERVER).o:	$(SERVER).c $(SERVER).h iot.h

err.o:		err.c err.h

log.o:		log.c log.h

misc.o:		misc.c misc.h

clean:
		rm -f *.o $(SERVER)
