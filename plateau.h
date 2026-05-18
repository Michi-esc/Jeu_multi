#ifndef PLATEAU_H
#define PLATEAU_H

#define BOARD_SIZE 12

typedef enum {
    EMPTY = 0, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, OUT_OF_BOUNDS
} PieceType;

typedef enum {
    NONE = 0, RED, BLUE, YELLOW, GREEN
} Color;

typedef struct {
    PieceType type;
    Color color;
} Piece;

typedef struct {
    Piece grid[BOARD_SIZE][BOARD_SIZE];
    Color turn;
    int player_active[5]; // 1 si le joueur est encore en lice, 0 sinon
} Board;

// Initialisation du plateau
void init_board(Board *b);

// Vérifie si une case est dans les coins morts (3x3)
int is_valid_cell(int x, int y);

#endif