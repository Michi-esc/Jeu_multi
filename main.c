#include <stdio.h>
#include <SDL2/SDL.h>
#include "plateau.h"
#include "regles.h"
#include "affichage.h"

int main(int argc, char* argv[]) {
    // 1. Initialisation du jeu
    Board b;
    init_board(&b);

    // 2. Initialisation de la SDL
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!init_sdl(&window, &renderer)) {
        return -1;
    }

    int running = 1;
    SDL_Event event;
    
    // Variables pour gérer la sélection (mécanique à 2 clics)
    int x1 = -1, y1 = -1;
    int piece_selectionnee = 0; // 0 = attente source, 1 = attente destination

    // 3. Boucle principale
    while (running) {
        Uint32 frame_start = SDL_GetTicks();

        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } 
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int grid_x = event.button.x / TILE_SIZE;
                    int grid_y = event.button.y / TILE_SIZE;

                    if (!piece_selectionnee) {
                        // Clic 1 : Sélection d'une case source valide contenant une pièce de la bonne couleur
                        if (is_valid_cell(grid_x, grid_y) && 
                            b.grid[grid_y][grid_x].type != EMPTY && 
                            b.grid[grid_y][grid_x].color == b.turn) {
                            x1 = grid_x;
                            y1 = grid_y;
                            piece_selectionnee = 1;
                        }
                    } else {
                        // Clic 2 : Choisir une destination
                        int x2 = grid_x;
                        int y2 = grid_y;
                        
                        // Si on clique sur une autre de ses propres pièces, on change de sélection
                        if (is_valid_cell(x2, y2) && b.grid[y2][x2].color == b.turn) {
                            x1 = x2;
                            y1 = y2;
                        } else {
                            // Sinon, on tente de jouer le coup
                            verifier_et_jouer_coup(&b, x1, y1, x2, y2);
                            piece_selectionnee = 0; // On réinitialise l'état
                        }
                    }
                }
            }
        }

        // Affichage
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // On passe les coordonnées de la sélection si une pièce est sélectionnée
        dessiner_plateau(renderer, &b, piece_selectionnee ? x1 : -1, piece_selectionnee ? y1 : -1);

        // Affichage de l'interface
        dessiner_interface(renderer, &b);

        SDL_RenderPresent(renderer);

        // Limitation des FPS (Capping à ~60 FPS soit ~16ms par frame)
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) SDL_Delay(16 - frame_time);
    }

    nettoyer_sdl(window, renderer);
    return 0;
}