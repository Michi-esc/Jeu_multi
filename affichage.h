#ifndef AFFICHAGE_H
#define AFFICHAGE_H

#include <SDL2/SDL.h>
#include "plateau.h"

// Paramètres d'affichage
#define TILE_SIZE 40
#define WINDOW_SIZE (BOARD_SIZE * TILE_SIZE)

// Initialise la SDL, crée la fenêtre et le renderer.
// Retourne 1 en cas de succès, 0 en cas d'erreur.
int init_sdl(SDL_Window **window, SDL_Renderer **renderer);

// Libère les ressources allouées par la SDL
void nettoyer_sdl(SDL_Window *window, SDL_Renderer *renderer);

// Parcourt la grille de la structure Board et dessine le plateau (damier, coins morts, pièces)
void dessiner_plateau(SDL_Renderer *renderer, Board *b, int sel_x, int sel_y);

// Dessine l'interface par-dessus le plateau
void dessiner_interface(SDL_Renderer *renderer, Board *b);

#endif