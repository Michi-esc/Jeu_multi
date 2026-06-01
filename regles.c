#include "regles.h"
#include <stdlib.h>

// Vérifie si le chemin est libre pour les pièces à longue portée (Tour, Fou, Reine)
static int est_chemin_libre(Board *b, int x1, int y1, int x2, int y2) {
    int dx = (x2 > x1) ? 1 : (x2 < x1 ? -1 : 0);
    int dy = (y2 > y1) ? 1 : (y2 < y1 ? -1 : 0);
    int x = x1 + dx;
    int y = y1 + dy;

    while (x != x2 || y != y2) {
        if (!is_valid_cell(x, y)) return 0; // Ne peut pas traverser le vide
        if (b->grid[y][x].type != EMPTY) return 0;
        x += dx;
        y += dy;
    }
    return 1;
}

// Logique brute de déplacement sans vérification de l'état du Roi (pour éviter la récursion)
static int mouvement_theorique_legal(Board *b, int x1, int y1, int x2, int y2) {
    Piece p = b->grid[y1][x1];
    Piece cible = b->grid[y2][x2];
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    if (!is_valid_cell(x1, y1) || !is_valid_cell(x2, y2)) return 0;
    if (p.type == EMPTY) return 0;
    if (cible.type != EMPTY && cible.color == p.color) return 0;

    switch (p.type) {
        case PAWN: {
            int dirX = 0, dirY = 0;
            int startRow = -1, startCol = -1;
            
            if (p.color == WHITE) { dirY = -1; startRow = BOARD_SIZE - 2; }
            if (p.color == BLACK) { dirY = 1;  startRow = 1;  }
            if (p.color == BLUE)  { dirX = 1;  startCol = 1;  }
            if (p.color == RED)   { dirX = -1; startCol = BOARD_SIZE - 2; }

            // Avance d'une case
            if (x2 == x1 + dirX && y2 == y1 + dirY && cible.type == EMPTY) return 1;
            
            // Premier double pas
            if (cible.type == EMPTY && b->grid[y1+dirY][x1+dirX].type == EMPTY) {
                if (dirY != 0 && y1 == startRow && y2 == y1 + 2 * dirY && x1 == x2) return 1;
                if (dirX != 0 && x1 == startCol && x2 == x1 + 2 * dirX && y1 == y2) return 1;
            }

            // Capture diagonale
            if (cible.type != EMPTY && cible.color != p.color) {
                if (dirY != 0 && y2 == y1 + dirY && (x2 == x1 + 1 || x2 == x1 - 1)) return 1;
                if (dirX != 0 && x2 == x1 + dirX && (y2 == y1 + 1 || y2 == y1 - 1)) return 1;
            }
            return 0;
        }
        case ROOK:
            if (x1 != x2 && y1 != y2) return 0;
            return est_chemin_libre(b, x1, y1, x2, y2);

        case KNIGHT:
            return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);

        case BISHOP:
            if (dx != dy) return 0;
            return est_chemin_libre(b, x1, y1, x2, y2);

        case QUEEN:
            if (dx != dy && x1 != x2 && y1 != y2) return 0;
            return est_chemin_libre(b, x1, y1, x2, y2);

        case KING:
            return (dx <= 1 && dy <= 1);

        default: return 0;
    }
}

int est_mouvement_legal(Board *b, int x1, int y1, int x2, int y2) {
    if (b->grid[y1][x1].color != b->turn) return 0;
    return mouvement_theorique_legal(b, x1, y1, x2, y2);
}

void passer_au_tour_suivant(Board *b) {
    int initial_turn = b->turn;
    do {
        b->turn = (b->turn % 4) + 1;
        if (b->turn == initial_turn) break; // Sécurité anti-boucle infinie
    } while (b->player_active[b->turn] == 0);
}

int est_en_echec(Board *b, Color c) {
    int kx = -1, ky = -1;
    // 1. Trouver le roi
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (b->grid[y][x].type == KING && b->grid[y][x].color == c) {
                kx = x; ky = y; break;
            }
        }
    }
    if (kx == -1) return 0; // Roi mangé ou absent

    // 2. Vérifier si une pièce adverse peut l'atteindre
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = b->grid[y][x];
            if (p.color != NONE && p.color != c) {
                if (mouvement_theorique_legal(b, x, y, kx, ky)) return 1;
            }
        }
    }
    return 0;
}

int verifier_et_jouer_coup(Board *b, int x1, int y1, int x2, int y2) {
    if (!est_mouvement_legal(b, x1, y1, x2, y2)) {
        return 0; // Coup invalide
    }

    // Simuler le coup pour vérifier l'échec au roi
    Piece temp = b->grid[y2][x2];
    b->grid[y2][x2] = b->grid[y1][x1];
    b->grid[y1][x1] = (Piece){EMPTY, NONE};

    if (est_en_echec(b, b->turn)) {
        // Annuler le coup, on ne peut pas se mettre en échec
        b->grid[y1][x1] = b->grid[y2][x2];
        b->grid[y2][x2] = temp;
        return 0;
    }

    // Élimination d'un joueur si son Roi est capturé
    if (temp.type == KING) {
        b->player_active[temp.color] = 0; // Le joueur ne jouera plus
        // Transformer toutes ses pièces restantes en blocs gris neutres
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                if (b->grid[y][x].color == temp.color) {
                    b->grid[y][x].color = NONE; 
                }
            }
        }
    }

    return 1;
}

int doit_promouvoir(Board *b, int x, int y) {
    Piece p = b->grid[y][x];
    if (p.type != PAWN) return 0;
    if (p.color == WHITE && y == 0) return 1;
    if (p.color == BLACK && y == BOARD_SIZE - 1) return 1;
    if (p.color == RED && x == 0) return 1;
    if (p.color == BLUE && x == BOARD_SIZE - 1) return 1;
    return 0;
}