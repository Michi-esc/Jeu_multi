#include "regles.h"
#include <stdlib.h>

// Fonction utilitaire pour vérifier que la trajectoire d'une pièce est dégagée
int chemin_est_libre(Board *b, int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int stepX = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int stepY = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

    int x = x1 + stepX;
    int y = y1 + stepY;

    while (x != x2 || y != y2) {
        if (!is_valid_cell(x, y)) return 0; // Ne peut pas traverser le vide en diagonale
        if (b->grid[y][x].type != EMPTY) return 0; // Collision avec une pièce
        x += stepX;
        y += stepY;
    }
    return 1;
}

int est_mouvement_legal(Board *b, int x1, int y1, int x2, int y2) {
    Piece p = b->grid[y1][x1];
    Piece cible = b->grid[y2][x2];

    if (!is_valid_cell(x1, y1) || !is_valid_cell(x2, y2)) return 0;
    if (p.type == EMPTY || p.color != b->turn) return 0;
    if (cible.type != EMPTY && cible.color == p.color) return 0;

    int dx = x2 - x1;
    int dy = y2 - y1;
    int adx = abs(dx);
    int ady = abs(dy);

    switch (p.type) {
        case PAWN: {
            int dirX = 0, dirY = 0;
            int startX = -1, startY = -1;
            if (p.color == WHITE) { dirY = -1; startY = BOARD_SIZE - 2; }
            else if (p.color == BLACK) { dirY = 1; startY = 1; }
            else if (p.color == RED) { dirX = -1; startX = BOARD_SIZE - 2; }
            else if (p.color == BLUE) { dirX = 1; startX = 1; }

            if (cible.type == EMPTY) { // Mouvement simple
                if (p.color == WHITE || p.color == BLACK) {
                    if (dx == 0 && dy == dirY) return 1;
                    if (dx == 0 && dy == 2 * dirY && y1 == startY && b->grid[y1+dirY][x1].type == EMPTY) return 1;
                } else {
                    if (dy == 0 && dx == dirX) return 1;
                    if (dy == 0 && dx == 2 * dirX && x1 == startX && b->grid[y1][x1+dirX].type == EMPTY) return 1;
                }
            } else { // Prise en diagonale
                if (p.color == WHITE || p.color == BLACK) { return (adx == 1 && dy == dirY); }
                else { return (ady == 1 && dx == dirX); }
            }
            return 0;
        }
        case ROOK:   if (adx != 0 && ady != 0) return 0; return chemin_est_libre(b, x1, y1, x2, y2);
        case BISHOP: if (adx != ady) return 0;           return chemin_est_libre(b, x1, y1, x2, y2);
        case QUEEN:  if (adx != 0 && ady != 0 && adx != ady) return 0; return chemin_est_libre(b, x1, y1, x2, y2);
        case KNIGHT: return (adx == 1 && ady == 2) || (adx == 2 && ady == 1);
        case KING:   return (adx <= 1 && ady <= 1);
        default:     return 0;
    }
}

void passer_au_tour_suivant(Board *b) {
    int initial_turn = b->turn;
    do {
        b->turn = (b->turn % 4) + 1;
        if (b->turn == initial_turn) break; // Sécurité anti-boucle infinie
    } while (b->player_active[b->turn] == 0);
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

    // Promotion automatique du Pion en Reine s'il arrive au bout
    if (b->grid[y2][x2].type == PAWN) {
        if (b->grid[y2][x2].color == WHITE && y2 == 0) b->grid[y2][x2].type = QUEEN;
        if (b->grid[y2][x2].color == BLACK && y2 == BOARD_SIZE - 1) b->grid[y2][x2].type = QUEEN;
        if (b->grid[y2][x2].color == RED && x2 == 0) b->grid[y2][x2].type = QUEEN;
        if (b->grid[y2][x2].color == BLUE && x2 == BOARD_SIZE - 1) b->grid[y2][x2].type = QUEEN;
    }

    // Le coup est définitif
    passer_au_tour_suivant(b);
    return 1;
}

int est_en_echec(Board *b, Color c) {
    int rx = -1, ry = -1;
    // 1. Trouver le roi du joueur
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (b->grid[y][x].type == KING && b->grid[y][x].color == c) {
                rx = x; ry = y; break;
            }
        }
        if (rx != -1) break;
    }
    if (rx == -1) return 0; // Si plus de roi, pas d'échec technique

    Color current_turn = b->turn;
    int en_echec = 0;

    // 2. Vérifier si n'importe quelle pièce ennemie peut l'attaquer
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = b->grid[y][x];
            if (p.type != EMPTY && p.color != c && p.color != NONE) {
                b->turn = p.color; // On simule le tour de l'ennemi pour valider son coup
                if (est_mouvement_legal(b, x, y, rx, ry)) {
                    en_echec = 1; break;
                }
            }
        }
        if (en_echec) break;
    }

    b->turn = current_turn; // On restaure l'état du tour normal
    return en_echec;
}