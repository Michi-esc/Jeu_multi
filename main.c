#include <stdio.h>
#include <SDL2/SDL.h>
#include "plateau.h"
#include "regles.h"
#include "affichage.h"
#include "moteur_thread.h"

int main(int argc, char* argv[]) {
    // 1. Initialisation du jeu
    Board b;
    init_board(&b);

    // Initialisation du système de sécurité (Mutex)
    init_systeme_securite();

    // 2. Initialisation de la SDL
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!init_sdl(&window, &renderer)) {
        return -1;
    }

    int running = 1;
    SDL_Event event;
    
    // Variables pour gérer la sélection (mécanique à 2 clics)
    int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
    int etat_jeu = 0; // 0 = attente source, 1 = attente destination, 2 = promotion

    // 3. Boucle principale
    while (running) {
        Uint32 frame_start = SDL_GetTicks();

        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } 
            else if (event.type == SDL_MOUSEBUTTONDOWN && etat_jeu != 2) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int grid_x = event.button.x / TILE_SIZE;
                    int grid_y = event.button.y / TILE_SIZE;

                    if (etat_jeu == 0) {
                        // Clic 1 : Sélection d'une case source valide contenant une pièce de la bonne couleur
                        if (is_valid_cell(grid_x, grid_y) && 
                            b.grid[grid_y][grid_x].type != EMPTY && 
                            b.grid[grid_y][grid_x].color == b.turn) {
                            x1 = grid_x;
                            y1 = grid_y;
                            etat_jeu = 1;
                        }
                    } else {
                        // Clic 2 : Choisir une destination
                        x2 = grid_x;
                        y2 = grid_y;
                        
                        // Si on clique sur une autre de ses propres pièces, on change de sélection
                        if (is_valid_cell(x2, y2) && b.grid[y2][x2].color == b.turn) {
                            x1 = x2;
                            y1 = y2;
                        } else {
                            // Sinon, on tente de jouer le coup
                            if (verifier_et_jouer_coup(&b, x1, y1, x2, y2)) {
                                if (doit_promouvoir(&b, x2, y2)) {
                                    etat_jeu = 2; // On attend le choix de promotion (clavier)
                                } else {
                                    passer_au_tour_suivant(&b);
                                    etat_jeu = 0;
                                }
                            }
                            // Si le coup est invalide, on garde la sélection pour réessayer
                        }
                    }
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    etat_jeu = 0; // Clic droit pour annuler la sélection
                }
            }
            else if (event.type == SDL_KEYDOWN && etat_jeu == 2) {
                PieceType choix = EMPTY;
                if (event.key.keysym.sym == SDLK_1) choix = QUEEN;
                else if (event.key.keysym.sym == SDLK_2) choix = ROOK;
                else if (event.key.keysym.sym == SDLK_3) choix = BISHOP;
                else if (event.key.keysym.sym == SDLK_4) choix = KNIGHT;

                if (choix != EMPTY) {
                    b.grid[y2][x2].type = choix;
                    passer_au_tour_suivant(&b);
                    etat_jeu = 0;
                }
            }
        }

        // Affichage
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // On passe les coordonnées de la sélection si une pièce est sélectionnée
        dessiner_plateau(renderer, &b, (etat_jeu == 1 || etat_jeu == 2) ? x1 : -1, (etat_jeu == 1 || etat_jeu == 2) ? y1 : -1);

        // Affichage de l'interface
        dessiner_interface(renderer, &b);

        SDL_RenderPresent(renderer);

        // Limitation des FPS (Capping à ~60 FPS soit ~16ms par frame)
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) SDL_Delay(16 - frame_time);
    }

    fermer_systeme_securite();
    nettoyer_sdl(window, renderer);
    return 0;
}