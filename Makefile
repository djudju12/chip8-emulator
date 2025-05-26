CC=gcc
CFLAGS=-Wall -Wextra -ggdb

LIBS=-lraylib -lm

BIN=bin
SRC=src

DEFINES=DEBUG

$(BIN)/chip8: $(SRC)/main.c $(BIN)
	$(CC) $(addprefix -D, $(DEFINES)) $< $(CFLAGS) -o $@ $(LIBS)

$(BIN):
	mkdir -p $(BIN)