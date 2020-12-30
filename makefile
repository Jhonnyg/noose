
CC = clang++
CFLAGS = -g -Wall -o0
OUT = bin
BIN = $(OUT)/noose

SRC = main.cpp noose.cpp

noose.o: noose.cpp noose.h
	$(CC) $(CFLAGS) -c noose.cpp -o $(OUT)/noose.o

main.o: main.cpp noose.h
	$(CC) $(CFLAGS) -c main.cpp -o $(OUT)/main.o

all: main.o noose.o
	$(CC) $(CFLAGS) -o $(BIN) $(OUT)/main.o $(OUT)/noose.o

clean:
	$(RM) $(BIN)

$(shell mkdir -p $(OUT))
