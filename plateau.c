#include "plateau.h"
#include <stdio.h>

int is_valid_cell(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return 0;
    // Coins morts 3x3 pour laisser 8 cases libres au centre sur un plateau de 14
    // Les indices 0, 1, 2 et 11, 12, 13 sont les zones de coins
    if ((x < 3 || x >= (BOARD_SIZE - 3)) && (y < 3 || y >= (BOARD_SIZE - 3))) return 0;
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
    // Pour un BOARD_SIZE de 14, (14 - 8) / 2 = 3. 
    // Les pièces occupent les colonnes/lignes de 3 à 10 (soit 8 cases).
    int start_pos_8_pieces = (BOARD_SIZE - 8) / 2; 
    setup_player_pieces(b, WHITE, BOARD_SIZE - 1, start_pos_8_pieces, -1); // Sud (ligne 11, pions sur 10, colonnes 2-9)
    setup_player_pieces(b, BLACK, 0, start_pos_8_pieces, 1);   // Nord (ligne 0, pions sur 1, colonnes 2-9)
    setup_side_player_pieces(b, RED, BOARD_SIZE - 1, start_pos_8_pieces, -1); // Est (colonne 11, pions sur 10, lignes 2-9)
    setup_side_player_pieces(b, BLUE, 0, start_pos_8_pieces, 1);  // Ouest (colonne 0, pions sur 1, lignes 2-9)

    b->turn = WHITE;
    for(int i=1; i<=4; i++) b->player_active[i] = 1;
}