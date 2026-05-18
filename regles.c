#include "regles.h"
#include <stdlib.h>

int est_mouvement_legal(Board *b, int x1, int y1, int x2, int y2) {
    Piece p = b->grid[y1][x1];
    Piece cible = b->grid[y2][x2];

    if (!is_valid_cell(x1, y1) || !is_valid_cell(x2, y2)) return 0;
    if (p.type == EMPTY || p.color != b->turn) return 0;
    if (cible.type != EMPTY && cible.color == p.color) return 0;

    // Ici, on ajouterait un switch(p.type) pour chaque pièce
    // Exemple simplifié pour le Cavalier :
    if (p.type == KNIGHT) {
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
    }

    // TODO: Implémenter les autres pièces (Pion, Tour, Fou, Reine, Roi)
    return 1; // Temporaire pour test
}

void passer_au_tour_suivant(Board *b) {
    do {
        b->turn = (b->turn % 4) + 1;
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

    // Le coup est définitif
    passer_au_tour_suivant(b);
    return 1;
}

int est_en_echec(Board *b, Color c) {
    // Parcourir tout le plateau pour voir si une pièce ennemie peut manger le roi de couleur c
    return 0; // À implémenter
}