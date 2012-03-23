CC = gcc
CP = cp
CFLAGS = -g -Wall -W -Wno-unused-function -Wno-unused-parameter -Werror
RM = rm

SRCS = lex.c syn.c dynarray.c common.c
OBJS = $(SRCS:.c=.o)
BINARIES = client server 
SUBFOLDER = testserver

all: client server copy

rebuild: clean all

clean:
	-$(RM) -r -f *.o *.c~ *.h~ *.purify core* srv* $(BINARIES) \
	./$(SUBFOLDER)/*

#%.o: %.c
 #    $(CC) $(CFLAGS) -c $< -o $@

client: client.o $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $^ 

server: server.o $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $^ 

copy:   server.o $(OBJS)
	$(CP) server $(SUBFOLDER)

dynarray.o: dynarray.c dynarray.h
lex.o: lex.c lex.h dynarray.c dynarray.h
syn.o: syn.c syn.h dynarray.c dynarray.h
client.o: client.c client.h lex.c lex.h syn.c syn.h dynarray.c dynarray.h
server.o: server.c server.h lex.c lex.h syn.c syn.h dynarray.c dynarray.h
