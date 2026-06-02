#include <stdio.h>
#include <SDL2/SDL.h>
#include "plateau.h"
#include "regles.h"
#include "affichage.h"
#include "moteur_thread.h"
#include "menu.h"
#include "ia.h"
#include "reseau.h"

extern void passer_au_tour_suivant(Board *b);

// Log terminal d'un coup (ajouté par Rôle 2)
static void afficher_historique(Board *b, int x1, int y1, int x2, int y2, int is_ia) {
    const char *noms_couleurs[] = {"", "BLANC", "NOIR", "ROUGE", "BLEU"};
    const char *noms_pieces[]   = {"", "Pion", "Tour", "Cavalier", "Fou", "Reine", "Roi"};
    Color     c  = b->grid[y2][x2].color;
    PieceType pt = b->grid[y2][x2].type;
    if (c >= WHITE && c <= BLUE && pt >= PAWN && pt <= KING) {
        printf("[%s] %s joue : %s (%d,%d) -> (%d,%d)\n",
               is_ia ? "IA" : "HUMAIN",
               noms_couleurs[c], noms_pieces[pt],
               x1, y1, x2, y2);
    }
}

#undef main
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    // -------------------------------------------------------
    // 1. Initialisation SDL
    // -------------------------------------------------------
    SDL_Window   *window   = NULL;
    SDL_Renderer *renderer = NULL;
    if (!init_sdl(&window, &renderer)) return -1;

    // -------------------------------------------------------
    // 2. Menu (gère aussi la connexion réseau en interne)
    // -------------------------------------------------------
    GameConfig config;
    if (!afficher_menu(renderer, &config)) {
        nettoyer_sdl(window, renderer);
        reseau_fermer();
        return 0;
    }

    // -------------------------------------------------------
    // 3. Initialisation du plateau
    // -------------------------------------------------------
    Board b;
    init_board(&b);
    init_systeme_securite();

    // Réassigner le vrai plateau aux threads réseau (qui utilisaient un board temporaire)
    if (config.mode_reseau != 0)
        reseau_set_board(&b);

    if (config.mode_reseau == 0) {
        // --- SOLO : appliquer la config manuelle ---
        for (int i = 1; i <= 4; i++)
            b.player_active[i] = config.joueur_actif[i];
        while (!b.player_active[b.turn])
            b.turn = (b.turn % 4) + 1;
    } else {
        // --- RÉSEAU : chaque PC contrôle sa couleur, pas d'IA ---
        for (int i = 1; i <= 4; i++) config.joueur_ia[i] = 0;

        if (config.mode_reseau == 1) {
            // Serveur = RED + clients connectés
            b.player_active[WHITE]  = 1;
            b.player_active[BLACK]  = (net_info.nb_connectes >= 1) ? 1 : 0;
            b.player_active[RED]    = (net_info.nb_connectes >= 2) ? 1 : 0;
            b.player_active[BLUE]   = (net_info.nb_connectes >= 3) ? 1 : 0;
        } else {
            // Client : tous actifs (le serveur arbitre les coups)
            for (int i = 1; i <= 4; i++) b.player_active[i] = 1;
        }
        while (!b.player_active[b.turn])
            b.turn = (b.turn % 4) + 1;
    }

    // -------------------------------------------------------
    // 4. Boucle principale (~60 FPS)
    // -------------------------------------------------------
    int running   = 1;
    SDL_Event event;
    int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
    int etat_jeu  = 0; // 0=attente src | 1=attente dest | 2=promotion | 3=fin
    int ia_lancee = 0;

    // Pour la bannière de victoire (ajouté par Rôle 2)
    int nb_actifs  = 0;
    int id_gagnant = 0;

    while (running) {
        Uint32 frame_start = SDL_GetTicks();

        // ---- Détection fin de partie (Rôle 2) ----
        nb_actifs  = 0;
        id_gagnant = 0;
        for (int i = 1; i <= 4; i++) {
            if (b.player_active[i]) { nb_actifs++; id_gagnant = i; }
        }
        if (nb_actifs <= 1 && etat_jeu != 3) etat_jeu = 3;

        // ---- Tour IA (solo uniquement) ----
        if (config.mode_reseau == 0 && config.joueur_ia[b.turn] &&
            !ia_lancee && etat_jeu != 2 && etat_jeu != 3) {
            ia_lancer(&b, b.turn);
            ia_lancee = 1;
        }
        if (ia_lancee) {
            pthread_mutex_lock(&mutex_ia);
            int fini = !ia_en_cours;
            pthread_mutex_unlock(&mutex_ia);
            if (fini) {
                Coup coup = ia_recuperer_coup();
                if (coup.valide) {
                    if (modifier_plateau_securise(&b, coup.x1, coup.y1, coup.x2, coup.y2))
                        afficher_historique(&b, coup.x1, coup.y1, coup.x2, coup.y2, 1);
                } else {
                    passer_au_tour_suivant(&b);
                }
                ia_lancee = 0;
                etat_jeu  = 0;
            }
        }

        // ---- Gestion des événements SDL ----
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = 0; break; }

            // En réseau : n'autoriser les clics QUE quand c'est notre couleur
            int mon_tour = (config.mode_reseau == 0) ||
                           (b.turn == net_info.ma_couleur);

            if (mon_tour && !config.joueur_ia[b.turn] &&
                event.type == SDL_MOUSEBUTTONDOWN &&
                etat_jeu != 2 && etat_jeu != 3) {

                if (event.button.button == SDL_BUTTON_LEFT) {
                    int gx = event.button.x / TILE_SIZE;
                    int gy = event.button.y / TILE_SIZE;

                    if (etat_jeu == 0) {
                        if (is_valid_cell(gx, gy) &&
                            b.grid[gy][gx].type != EMPTY &&
                            b.grid[gy][gx].color == b.turn) {
                            x1 = gx; y1 = gy; etat_jeu = 1;
                        }
                    } else {
                        x2 = gx; y2 = gy;
                        if (is_valid_cell(x2, y2) &&
                            b.grid[y2][x2].color == b.turn) {
                            x1 = x2; y1 = y2; // re-sélection
                        } else {
                            if (modifier_plateau_securise(&b, x1, y1, x2, y2)) {
                                afficher_historique(&b, x1, y1, x2, y2, 0);
                                // Diffuser sur le réseau si actif
                                if (config.mode_reseau != 0)
                                    reseau_envoyer_coup(x1, y1, x2, y2,
                                                        net_info.ma_couleur);
                                etat_jeu = doit_promouvoir(&b, x2, y2) ? 2 : 0;
                            }
                        }
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    etat_jeu = 0; x1 = y1 = -1;
                }
            }

            if (event.type == SDL_KEYDOWN && etat_jeu == 2) {
                PieceType choix = EMPTY;
                if      (event.key.keysym.sym == SDLK_1) choix = QUEEN;
                else if (event.key.keysym.sym == SDLK_2) choix = ROOK;
                else if (event.key.keysym.sym == SDLK_3) choix = BISHOP;
                else if (event.key.keysym.sym == SDLK_4) choix = KNIGHT;
                if (choix != EMPTY) {
                    b.grid[y2][x2].type = choix;
                    if (config.mode_reseau != 0)
                        reseau_envoyer_coup(x2, y2, x2, y2, net_info.ma_couleur);
                    etat_jeu = 0;
                }
            }
        }

        // ---- Rendu ----
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int sel_x = (etat_jeu == 1 || etat_jeu == 2) ? x1 : -1;
        int sel_y = (etat_jeu == 1 || etat_jeu == 2) ? y1 : -1;
        dessiner_plateau(renderer, &b, sel_x, sel_y);
        dessiner_interface(renderer, &b);

        // Bannière "IA réfléchit..." (Rôle 2) — simple rectangle coloré
        if (ia_lancee) {
            SDL_Rect bg = { WINDOW_SIZE/2 - 60, WINDOW_SIZE/2 - 15, 120, 30 };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
            SDL_RenderFillRect(renderer, &bg);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
            SDL_RenderDrawRect(renderer, &bg);
        }

        // Bannière victoire (Rôle 2)
        if (etat_jeu == 3) {
            static const uint8_t VCOLS[5][3] = {
                {0,0,0}, {210,55,55}, {55,100,215}, {210,195,45}, {50,195,65}
            };
            SDL_Rect bg = { WINDOW_SIZE/2 - 80, WINDOW_SIZE/2 - 25, 160, 50 };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 210);
            SDL_RenderFillRect(renderer, &bg);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            if (nb_actifs == 1) {
                SDL_SetRenderDrawColor(renderer,
                    VCOLS[id_gagnant][0], VCOLS[id_gagnant][1], VCOLS[id_gagnant][2], 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            }
            SDL_RenderDrawRect(renderer, &bg);
        }

        SDL_RenderPresent(renderer);

        Uint32 ft = SDL_GetTicks() - frame_start;
        if (ft < 16) SDL_Delay(16 - ft);
    }

    // -------------------------------------------------------
    // 5. Nettoyage
    // -------------------------------------------------------
    reseau_fermer();
    fermer_systeme_securite();
    nettoyer_sdl(window, renderer);
    return 0;
}
