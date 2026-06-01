#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include "plateau.h"

// ================================================================
//  Configuration retournée par le menu
// ================================================================
typedef struct {
    // --- Mode solo : config de chaque joueur ---
    int joueur_actif[5]; // [1..4] : 1 = participe, 0 = éliminé au départ
    int joueur_ia[5];    // [1..4] : 1 = IA, 0 = humain

    // --- Mode réseau ---
    // 0 = solo | 1 = héberger (serveur) | 2 = rejoindre (client)
    int  mode_reseau;
    char ip_serveur[64]; // IP saisie par l'utilisateur (mode rejoindre)
} GameConfig;

// ================================================================
//  Affiche le menu principal et tous les sous-menus jusqu'à ce
//  que l'utilisateur soit prêt à jouer.
//  Retourne 1 → lancer la partie, 0 → fermeture de la fenêtre.
// ================================================================
int afficher_menu(SDL_Renderer *renderer, GameConfig *config);

#endif
