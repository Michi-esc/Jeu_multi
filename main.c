#include <stdio.h>
#include <SDL2/SDL.h>
#include "plateau.h"
#include "regles.h"
#include "affichage.h"
#include "moteur_thread.h"
#include "menu.h"
#include "ia.h"

// passer_au_tour_suivant est défini dans regles.c mais non exporté dans regles.h
// On la déclare ici pour la promotion de pion (seul cas où on en a besoin en dehors de regles.c)
extern void passer_au_tour_suivant(Board *b);

// Vérifie si le pion à (x, y) doit être promu (a atteint le bord opposé)
static int doit_promouvoir(Board *b, int x, int y) {
    Piece p = b->grid[y][x];
    if (p.type != PAWN) return 0;
    if (p.color == RED    && y == 0)             return 1;
    if (p.color == YELLOW && y == BOARD_SIZE - 1) return 1;
    if (p.color == BLUE   && x == BOARD_SIZE - 1) return 1;
    if (p.color == GREEN  && x == 0)             return 1;
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    // -------------------------------------------------------
    // 1. Initialisation SDL (fenêtre + renderer)
    // -------------------------------------------------------
    SDL_Window   *window   = NULL;
    SDL_Renderer *renderer = NULL;
    if (!init_sdl(&window, &renderer)) return -1;

    // -------------------------------------------------------
    // 2. Menu de configuration
    // -------------------------------------------------------
    GameConfig config;
    if (!afficher_menu(renderer, &config)) {
        nettoyer_sdl(window, renderer);
        return 0;
    }

    // -------------------------------------------------------
    // 3. Initialisation du plateau + système de threads
    // -------------------------------------------------------
    Board b;
    init_board(&b);
    init_systeme_securite();

    // Appliquer la configuration : désactiver les joueurs inactifs
    for (int i = 1; i <= 4; i++) {
        b.player_active[i] = config.joueur_actif[i];
    }
    // Si RED (premier joueur) est inactif, avancer jusqu'au premier joueur actif
    while (!b.player_active[b.turn]) {
        b.turn = (b.turn % 4) + 1;
    }

    // -------------------------------------------------------
    // 4. Variables de la boucle principale
    // -------------------------------------------------------
    int running   = 1;
    SDL_Event event;

    int x1 = -1, y1 = -1; // case source sélectionnée (clic 1)
    int x2 = -1, y2 = -1; // case destination           (clic 2)
    // États de jeu :
    //   0 = attente du 1er clic (sélection source)
    //   1 = attente du 2e clic  (sélection destination)
    //   2 = attente du choix de promotion (pion arrivé au bout)
    int etat_jeu  = 0;

    // Drapeaux IA
    int ia_lancee = 0; // 1 = un thread IA est en cours de calcul pour ce tour

    // -------------------------------------------------------
    // 5. Boucle principale (~60 FPS)
    // -------------------------------------------------------
    while (running) {
        Uint32 frame_start = SDL_GetTicks();

        // --- 5a. Gestion du tour IA ---
        // Si c'est le tour d'un joueur IA et que le calcul n'a pas encore été lancé
        if (config.joueur_ia[b.turn] && !ia_lancee && etat_jeu != 2) {
            ia_lancer(&b, b.turn);
            ia_lancee = 1;
        }

        // Quand le thread IA a terminé son calcul, appliquer le coup
        if (ia_lancee) {
            pthread_mutex_lock(&mutex_ia);
            int calcul_fini = !ia_en_cours;
            pthread_mutex_unlock(&mutex_ia);

            if (calcul_fini) {
                Coup coup = ia_recuperer_coup();
                if (coup.valide) {
                    // Utiliser l'accès sécurisé fourni par le Rôle 3
                    // modifier_plateau_securise appelle verifier_et_jouer_coup,
                    // qui avance déjà le tour en interne.
                    modifier_plateau_securise(&b, coup.x1, coup.y1, coup.x2, coup.y2);
                } else {
                    // Aucun coup légal disponible pour l'IA → passer le tour
                    passer_au_tour_suivant(&b);
                }
                ia_lancee = 0;
                etat_jeu  = 0;
            }
        }

        // --- 5b. Gestion des événements SDL ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }

            // Les clics souris ne sont traités que lors d'un tour humain
            if (!config.joueur_ia[b.turn] &&
                event.type == SDL_MOUSEBUTTONDOWN &&
                etat_jeu != 2) {

                if (event.button.button == SDL_BUTTON_LEFT) {
                    int gx = event.button.x / TILE_SIZE;
                    int gy = event.button.y / TILE_SIZE;

                    if (etat_jeu == 0) {
                        // Clic 1 : sélectionner une pièce du joueur courant
                        if (is_valid_cell(gx, gy) &&
                            b.grid[gy][gx].type != EMPTY &&
                            b.grid[gy][gx].color == b.turn) {
                            x1 = gx; y1 = gy;
                            etat_jeu = 1;
                        }
                    } else { // etat_jeu == 1
                        x2 = gx; y2 = gy;

                        // Re-sélection d'une autre pièce du même camp
                        if (is_valid_cell(x2, y2) &&
                            b.grid[y2][x2].color == b.turn) {
                            x1 = x2; y1 = y2;
                        } else {
                            // Tentative de coup via l'accès sécurisé (Rôle 3)
                            // Note : modifier_plateau_securise → verifier_et_jouer_coup
                            //        qui avance déjà le tour → NE PAS rappeler
                            //        passer_au_tour_suivant ici.
                            if (modifier_plateau_securise(&b, x1, y1, x2, y2)) {
                                // Vérifier si le pion doit être promu
                                // (le tour a déjà avancé, mais la pièce est encore en place)
                                if (doit_promouvoir(&b, x2, y2)) {
                                    etat_jeu = 2; // attendre le choix de promotion
                                } else {
                                    etat_jeu = 0;
                                }
                            }
                            // Si coup invalide : on garde la sélection, le joueur réessaie
                        }
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    // Clic droit : annuler la sélection en cours
                    etat_jeu = 0;
                    x1 = y1 = -1;
                }
            }

            // Sélection de la pièce de promotion (appui clavier)
            if (event.type == SDL_KEYDOWN && etat_jeu == 2) {
                PieceType choix = EMPTY;
                if      (event.key.keysym.sym == SDLK_1) choix = QUEEN;
                else if (event.key.keysym.sym == SDLK_2) choix = ROOK;
                else if (event.key.keysym.sym == SDLK_3) choix = BISHOP;
                else if (event.key.keysym.sym == SDLK_4) choix = KNIGHT;

                if (choix != EMPTY) {
                    b.grid[y2][x2].type = choix;
                    // Le tour a déjà été avancé par verifier_et_jouer_coup → rien à faire
                    etat_jeu = 0;
                }
            }
        }

        // --- 5c. Rendu ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int sel_x = (etat_jeu == 1 || etat_jeu == 2) ? x1 : -1;
        int sel_y = (etat_jeu == 1 || etat_jeu == 2) ? y1 : -1;
        dessiner_plateau(renderer, &b, sel_x, sel_y);
        dessiner_interface(renderer, &b);

        SDL_RenderPresent(renderer);

        // Limitation à ~60 FPS
        Uint32 ft = SDL_GetTicks() - frame_start;
        if (ft < 16) SDL_Delay(16 - ft);
    }

    // -------------------------------------------------------
    // 6. Nettoyage
    // -------------------------------------------------------
    fermer_systeme_securite();
    nettoyer_sdl(window, renderer);
    return 0;
}
