#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include "plateau.h"

// Configuration choisie dans le menu avant le lancement de la partie
typedef struct {
    int joueur_actif[5]; // [1..4] : 1 = le joueur participe, 0 = éliminé dès le départ
    int joueur_ia[5];    // [1..4] : 1 = contrôlé par l'IA, 0 = contrôlé par un humain
} GameConfig;

// Affiche le menu de configuration et bloque jusqu'à ce que le joueur valide.
// Retourne 1 si la partie doit démarrer, 0 si l'utilisateur ferme la fenêtre.
// La fenêtre/renderer SDL doit déjà être initialisée avant l'appel.
int afficher_menu(SDL_Renderer *renderer, GameConfig *config);

#endif
