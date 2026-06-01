CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lmingw32 -lSDL2main -lSDL2 -lpthread

SRC = main.c plateau.c regles.c affichage.c moteur_thread.c synchronisation.c
OBJ = $(SRC:.c=.o)
EXEC = echecs4.exe

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBS) -mwindows

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	if exist *.o del /f *.o
	if exist $(EXEC) del /f $(EXEC)