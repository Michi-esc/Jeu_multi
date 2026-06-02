CC = gcc
CFLAGS = -Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs) -lpthread

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = jeu_multi

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)