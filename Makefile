CC = gcc
CFLAGS = -Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs) -lpthread -lws2_32

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
	EXEC = jeu_multi.exe
	LIBS += -lws2_32
else
	EXEC = jeu_multi
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)