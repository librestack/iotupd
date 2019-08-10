# SPDX-License-Identifier: GPL-2.0-or-later

PROGRAM:=iotup
CLIENT:=$(PROGRAM)c
SERVER:=$(PROGRAM)d
OBJECTS:=crc32.o err.o iot.o log.o misc.o
LIBS:=-llibrecast -lwolfssl

.PHONY:		all clean realclean

all:		$(CLIENT) $(SERVER)

$(CLIENT):	$(OBJECTS) $(CLIENT).o
		$(CC) $(LDFLAGS) -o $(CLIENT) $(CLIENT).o $(OBJECTS) $(LIBS)

$(SERVER):	$(OBJECTS) $(SERVER).o
		$(CC) $(LDFLAGS) -o $(SERVER) $(SERVER).o $(OBJECTS) $(LIBS)

$(CLIENT).o:	$(CLIENT).c $(CLIENT).h $(OBJECTS)

$(SERVER).o:	$(SERVER).c $(SERVER).h $(OBJECTS)

crc32.o:	crc32.c crc32.h

err.o:		err.c err.h

iot.o:		iot.c iot.h

log.o:		log.c log.h

misc.o:		misc.c misc.h

clean:
		rm -f *.o

realclean:
		rm -f *.o $(CLIENT) $(SERVER)

install:	all
		install -d $(BIN_PATH)
		install $(CLIENT) $(SERVER) $(BIN_PATH)/
