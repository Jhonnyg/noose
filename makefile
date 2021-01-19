
CC = clang++
CFLAGS = -g -Wall -o0
OUT = bin
BIN = $(OUT)/noose

SRC = main.cpp noose.cpp

noose_cpu.o: noose_cpu.cpp
	$(CC) $(CFLAGS) -c noose_cpu.cpp -o $(OUT)/noose_cpu.o

noose.o: noose.cpp noose.h
	$(CC) $(CFLAGS) -c noose.cpp -o $(OUT)/noose.o

main.o: main.cpp noose.h
	$(CC) $(CFLAGS) -c main.cpp -o $(OUT)/main.o

all: main.o noose.o noose_cpu.o
	$(CC) $(CFLAGS) -o $(BIN) $(OUT)/main.o $(OUT)/noose.o $(OUT)/noose_cpu.o

clean:
	$(RM) $(BIN)

$(shell mkdir -p $(OUT))
