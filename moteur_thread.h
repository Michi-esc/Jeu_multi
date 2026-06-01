#ifndef MOTEUR_THREAD_H
#define MOTEUR_THREAD_H

#include "plateau.h"

// Initialisation et destruction du système de protection (Mutex)
void init_systeme_securite(void);
void fermer_systeme_securite(void);

// Accès sécurisé pour le Rôle 2 (Affichage)
// Permet de copier l'état actuel du plateau sans risque de modification pendant la lecture
void recuperer_plateau_securise(Board *dest, Board *source);

// Accès sécurisé pour les Rôles 1, 2 et 4 (Logique / Joueur / IA)
// Encapsule la "fonction magique" du Rôle 1 dans un verrou Mutex
int modifier_plateau_securise(Board *b, int x1, int y1, int x2, int y2);

#endif