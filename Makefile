CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
SRC    = $(wildcard *.c)
OBJ    = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
    EXEC = jeu_multi.exe
    LIBS = $(shell sdl2-config --libs) -lpthread -lws2_32
else
    # macOS / Linux : pas de Winsock
    EXEC = jeu_multi
    LIBS = $(shell sdl2-config --libs) -lpthread
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC) jeu_multi.exe
