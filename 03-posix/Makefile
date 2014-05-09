PROJECT = sched 
SOURCE = main.c task.c parser.c
HEADERS = task.h parser.h

CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread -lrt
OBJECTS = ${SOURCE:.c=.o}

$(PROJECT): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(PROJECT) $(LDFLAGS)

$(OBJECTS): $(HEADERS)

clean:
	-rm -f $(PROJECT) $(OBJECTS) *.core
