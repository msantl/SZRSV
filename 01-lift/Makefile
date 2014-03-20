CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

HEADERS = udp.h message.h list.h constants.h parse.h

POMOCNI_C = udp.c message.c list.c parse.c
POMOCNI_O = ${POMOCNI_C:.c=.o} 

LIFT_C = lift.c
LIFT_O = ${LIFT_C:.c=.o}

UPR_C = upr.c
UPR_O = ${UPR_C:.c=.o}

TIPKE_C = tipke.c
TIPKE_O = ${TIPKE_C:.c=.o}

all: lift tipke upr

lift: $(LIFT_O) $(POMOCNI_O)
	$(CC) $(CFLAGS) $(LIFT_O) $(POMOCNI_O) -o $@

tipke: $(TIPKE_O) $(POMOCNI_O)
	$(CC) $(CFLAGS) $(TIPKE_O) $(POMOCNI_O) -o $@

upr: $(UPR_O) $(POMOCNI_O)
	$(CC) $(CFLAGS) $(UPR_O) $(POMOCNI_O) -o $@

$(TIPKE_O): $(HEADERS)
$(LIFT_O): $(HEADERS)
$(UPR_O): $(HEADERS)
$(POMOCNI_O): $(HEADERS)

clean:
	-rm -f lift tipke upr *.o
