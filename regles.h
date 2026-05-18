#ifndef REGLES_H
#define REGLES_H

#include "plateau.h"

// La fonction magique demandée
int verifier_et_jouer_coup(Board *b, int x1, int y1, int x2, int y2);

int est_en_echec(Board *b, Color c);

#endif