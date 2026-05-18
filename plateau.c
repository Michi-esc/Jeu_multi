#include "plateau.h"
#include <stdio.h>

int is_valid_cell(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return 0;
    
    // Coins morts 4x4. 
    // On autorise cependant les lignes/colonnes 0, 1, 10, 11 si elles sont au centre (indices 2 à 9)
    // pour permettre le placement des pièces.
    if ((x < 2 || x >= 10) && (y < 2 || y >= 10)) return 0;
    
    return 1;
}

void setup_player_pieces(Board *b, Color color, int row, int start_col, int direction) {
    int pawn_row = row + direction;
    PieceType order[] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    
    for (int i = 0; i < 8; i++) {
        b->grid[row][start_col + i] = (Piece){order[i], color};
        b->grid[pawn_row][start_col + i] = (Piece){PAWN, color};
    }
}

// Pour les joueurs Est/Ouest, on adapte la logique pour remplir en colonnes
void setup_side_player_pieces(Board *b, Color color, int col, int start_row, int direction) {
    int pawn_col = col + direction;
    PieceType order[] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    
    for (int i = 0; i < 8; i++) {
        b->grid[start_row + i][col] = (Piece){order[i], color};
        b->grid[start_row + i][pawn_col] = (Piece){PAWN, color};
    }
}

void init_board(Board *b) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            b->grid[y][x] = (Piece){EMPTY, NONE};
        }
    }

    // Placement des joueurs
    // On centre les 8 pièces sur les 12 cases (donc de l'indice 2 à 9)
    int start_idx = 2; 
    
    setup_player_pieces(b, RED, BOARD_SIZE - 1, start_idx, -1);    // Sud (Rouge)
    setup_player_pieces(b, YELLOW, 0, start_idx, 1);             // Nord (Jaune)
    setup_side_player_pieces(b, GREEN, BOARD_SIZE - 1, start_idx, -1); // Est (Vert)
    setup_side_player_pieces(b, BLUE, 0, start_idx, 1);           // Ouest (Bleu)

    b->turn = RED; // Le joueur Rouge commence
    for(int i=1; i<=4; i++) b->player_active[i] = 1;
}