#include "menu.h"
#include <string.h>
#include <stdint.h>
#include "affichage.h"

// Dessine un bouton rectangle avec un label centré.
// actif=1 → fond coloré + texte blanc | actif=0 → fond sombre + texte teinté
static void dessiner_bouton(SDL_Renderer *r, SDL_Rect rect, const char *label,
                             int actif, uint8_t br, uint8_t bg, uint8_t bb) {
    // Fond
    if (actif) {
        SDL_SetRenderDrawColor(r, br, bg, bb, 255);
    } else {
        SDL_SetRenderDrawColor(r, 38, 38, 50, 255);
    }
    SDL_RenderFillRect(r, &rect);

    // Bordure (couleur du joueur)
    SDL_SetRenderDrawColor(r, br, bg, bb, actif ? 255 : 110);
    SDL_RenderDrawRect(r, &rect);

    // Texte centré (scale 2 = 10px de haut)
    int scale = 2;
    int tw = largeur_texte(label, scale);
    int tx = rect.x + (rect.w - tw) / 2;
    int ty = rect.y + (rect.h - 5 * scale) / 2;
    uint8_t tr = actif ? 255 : br;
    uint8_t tg = actif ? 255 : bg;
    uint8_t tb = actif ? 255 : bb;
    dessiner_texte(r, label, tx, ty, scale, tr, tg, tb);
}

// ============================================================
//  Données constantes du menu
// ============================================================

// Couleurs RGB associées à chaque joueur (indice = Color enum : 1=RED..4=GREEN)
static const uint8_t COULEURS[5][3] = {
    {  0,   0,   0}, // NONE
    {210,  55,  55}, // RED   — Rouge
    { 55, 100, 215}, // BLUE  — Bleu
    {210, 195,  45}, // YELLOW— Jaune
    { 50, 195,  65}, // GREEN — Vert
};

static const char *NOMS_JOUEURS[5] = { "", "ROUGE", "BLEU", "JAUNE", "VERT" };

// ============================================================
//  Dimensions et positions du menu (pour WINDOW_SIZE = 480)
// ============================================================
#define MENU_TITRE_H   42   // hauteur de la bande titre
#define MENU_ROW_H     85   // hauteur d'une ligne joueur
#define MENU_ROW_Y0    55   // y du début de la première ligne joueur
#define MENU_JOUER_Y  408   // y du bouton JOUER
#define MENU_JOUER_H   48   // hauteur du bouton JOUER

// ============================================================
//  Rendu du menu complet
// ============================================================
static void dessiner_menu(SDL_Renderer *r, GameConfig *cfg, int survol_jouer) {
    // --- Fond général ---
    SDL_SetRenderDrawColor(r, 16, 16, 26, 255);
    SDL_RenderClear(r);

    // --- Bande titre ---
    SDL_Rect bande = { 0, 0, WINDOW_SIZE, MENU_TITRE_H };
    SDL_SetRenderDrawColor(r, 26, 26, 40, 255);
    SDL_RenderFillRect(r, &bande);

    // Titre centré, scale 3 → "ECHECS 4 JOUEURS" = 16 chars × 18px = 288px
    const char *titre = "ECHECS 4 JOUEURS";
    int tx = (WINDOW_SIZE - largeur_texte(titre, 3)) / 2;
    dessiner_texte(r, titre, tx, (MENU_TITRE_H - 15) / 2, 3, 255, 195, 45);

    // Ligne de séparation dorée
    SDL_SetRenderDrawColor(r, 255, 195, 45, 255);
    SDL_RenderDrawLine(r, 0, MENU_TITRE_H, WINDOW_SIZE, MENU_TITRE_H);

    // --- En-têtes colonnes ---
    dessiner_texte(r, "JOUEUR",  8,  46, 1, 100, 100, 120);
    dessiner_texte(r, "STATUT",  92, 46, 1, 100, 100, 120);
    dessiner_texte(r, "MODE",   245, 46, 1, 100, 100, 120);

    // --- Lignes joueurs ---
    for (int i = 1; i <= 4; i++) {
        int ry = MENU_ROW_Y0 + (i - 1) * MENU_ROW_H;
        uint8_t cr = COULEURS[i][0], cg = COULEURS[i][1], cb = COULEURS[i][2];

        // 1. Carré couleur (80×80) — clic = toggle actif/inactif
        SDL_Rect sq = { 5, ry + 3, 80, 78 };
        if (cfg->joueur_actif[i]) {
            SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
        } else {
            SDL_SetRenderDrawColor(r, 35, 35, 45, 255);
        }
        SDL_RenderFillRect(r, &sq);

        // Bordure du carré (toujours colorée)
        SDL_SetRenderDrawColor(r, cr, cg, cb, 200);
        SDL_RenderDrawRect(r, &sq);

        // Nom du joueur dans le carré (centré)
        int nw = largeur_texte(NOMS_JOUEURS[i], 1);
        int nx = sq.x + (sq.w - nw) / 2;
        int ny = sq.y + (sq.h - 5) / 2;
        uint8_t nalpha = cfg->joueur_actif[i] ? 240 : 80;
        dessiner_texte(r, NOMS_JOUEURS[i], nx, ny, 1, nalpha, nalpha, nalpha);

        // 2. Statut textuel + hint clic
        const char *statut = cfg->joueur_actif[i] ? "ACTIF" : "INACTIF";
        uint8_t sr = cfg->joueur_actif[i] ? 60  : 180;
        uint8_t sg = cfg->joueur_actif[i] ? 195 : 70;
        uint8_t sb = cfg->joueur_actif[i] ? 60  : 60;
        dessiner_texte(r, statut,  92, ry + 10, 1, sr, sg, sb);
        dessiner_texte(r, "CLIQUER", 92, ry + 22, 1, 60, 60, 75);
        dessiner_texte(r, "POUR", 92, ry + 30, 1, 60, 60, 75);
        dessiner_texte(r, "BASCULER", 92, ry + 38, 1, 60, 60, 75);

        // 3. Boutons HUMAIN / IA (visibles seulement si actif)
        if (cfg->joueur_actif[i]) {
            SDL_Rect btn_h  = {  92, ry + 48, 150, 30 };
            SDL_Rect btn_ia = { 248, ry + 48,  80, 30 };
            int is_human = (cfg->joueur_ia[i] == 0);
            dessiner_bouton(r, btn_h,  "HUMAIN", is_human,  cr, cg, cb);
            dessiner_bouton(r, btn_ia, "IA",    !is_human,  cr, cg, cb);
        } else {
            SDL_Rect zone_grise = { 92, ry + 48, 236, 30 };
            SDL_SetRenderDrawColor(r, 28, 28, 36, 255);
            SDL_RenderFillRect(r, &zone_grise);
            dessiner_texte(r, "JOUEUR DESACTIVE", 96, ry + 56, 1, 55, 55, 65);
        }

        // Séparateur entre joueurs
        SDL_SetRenderDrawColor(r, 35, 35, 50, 255);
        SDL_RenderDrawLine(r, 0, ry + MENU_ROW_H - 1, WINDOW_SIZE, ry + MENU_ROW_H - 1);
    }

    // --- Bouton JOUER ---
    SDL_Rect btn_jouer = { (WINDOW_SIZE - 200) / 2, MENU_JOUER_Y, 200, MENU_JOUER_H };
    uint8_t jbr = survol_jouer ? 80  : 50;
    uint8_t jbg = survol_jouer ? 240 : 200;
    uint8_t jbb = survol_jouer ? 80  : 50;
    dessiner_bouton(r, btn_jouer, "JOUER", 1, jbr, jbg, jbb);

    // Note d'information sous le bouton
    const char *note = "MIN 2 JOUEURS ACTIFS";
    dessiner_texte(r, note, (WINDOW_SIZE - largeur_texte(note, 1)) / 2,
                   MENU_JOUER_Y + MENU_JOUER_H + 6, 1, 70, 70, 85);

    SDL_RenderPresent(r);
}

// ============================================================
//  Comptage des joueurs actifs
// ============================================================
static int compter_actifs(GameConfig *cfg) {
    int n = 0;
    for (int i = 1; i <= 4; i++) n += cfg->joueur_actif[i];
    return n;
}

// ============================================================
//  Point d'entrée public
// ============================================================
int afficher_menu(SDL_Renderer *renderer, GameConfig *config) {
    // Configuration par défaut : 4 joueurs actifs, tous humains
    for (int i = 1; i <= 4; i++) {
        config->joueur_actif[i] = 1;
        config->joueur_ia[i]    = 0;
    }

    int survol_jouer = 0;
    SDL_Event event;

    while (1) {
        dessiner_menu(renderer, config, survol_jouer);

        while (SDL_PollEvent(&event)) {

            // Fermeture de la fenêtre → annuler
            if (event.type == SDL_QUIT) return 0;

            // Suivi du curseur pour l'effet de survol sur "JOUER"
            if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x, my = event.motion.y;
                survol_jouer = (mx >= (WINDOW_SIZE - 200) / 2 &&
                                mx <= (WINDOW_SIZE + 200) / 2 &&
                                my >= MENU_JOUER_Y &&
                                my <= MENU_JOUER_Y + MENU_JOUER_H);
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;

                // --- Bouton JOUER ---
                if (mx >= (WINDOW_SIZE - 200) / 2 && mx <= (WINDOW_SIZE + 200) / 2 &&
                    my >= MENU_JOUER_Y && my <= MENU_JOUER_Y + MENU_JOUER_H) {
                    if (compter_actifs(config) >= 2) return 1;
                }

                // --- Lignes joueurs ---
                for (int i = 1; i <= 4; i++) {
                    int ry = MENU_ROW_Y0 + (i - 1) * MENU_ROW_H;

                    // Carré couleur → toggle actif (minimum 2 joueurs)
                    if (mx >= 5 && mx <= 85 && my >= ry + 3 && my <= ry + 81) {
                        if (config->joueur_actif[i] && compter_actifs(config) > 2) {
                            config->joueur_actif[i] = 0;
                        } else if (!config->joueur_actif[i]) {
                            config->joueur_actif[i] = 1;
                        }
                    }

                    // Boutons HUMAIN / IA (seulement si le joueur est actif)
                    if (config->joueur_actif[i]) {
                        int btn_y = ry + 48;
                        // HUMAIN
                        if (mx >= 92 && mx <= 242 && my >= btn_y && my <= btn_y + 30) {
                            config->joueur_ia[i] = 0;
                        }
                        // IA
                        if (mx >= 248 && mx <= 328 && my >= btn_y && my <= btn_y + 30) {
                            config->joueur_ia[i] = 1;
                        }
                    }
                }
            }
        }

        SDL_Delay(16); // ~60 FPS
    }
}
