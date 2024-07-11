CC = gcc
CFLAGS = -Wall

# Detect platform
ifeq ($(OS),Windows_NT)
    LDFLAGS =
else
    LDFLAGS = -pthread -lc
endif

OBJ = chash.o hash_table.o

all: chash

chash: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o chash $(OBJ)

chash.o: chash.c hash_table.h
	$(CC) $(CFLAGS) -c chash.c

hash_table.o: hash_table.c hash_table.h
	$(CC) $(CFLAGS) -c hash_table.c

clean:
	rm -f *.o chash
