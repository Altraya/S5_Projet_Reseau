CC = gcc

LFLAGS = -g -W -Wall -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wbad-function-cast -Wcast-qual -o

CFLAGS = -g -W -Wall -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wbad-function-cast -Wcast-qual

LIBS = -lm

SRC=client.c serveur.c
EXEC=$(SRC:.c=)

all : $(EXEC)

% : %.c 
	$(CC) $(LFLAGS) $@ $^ $(LIBS)

clean:
	rm -f *.o

clear: clean
	rm -f $(EXEC)
