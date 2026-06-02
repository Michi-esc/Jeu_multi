#include "menu.h"
#include "reseau.h"
#include <string.h>
#include <stdint.h>
#include "affichage.h"
#include <stdio.h>

// ================================================================
//  Police de caractères 5×5 pixel art (sans SDL_ttf)
//  Index : 0=' ', 1-10='0'-'9', 11-36='A'-'Z'
//  Chaque octet = une ligne de 5 pixels (bit 4 = col gauche)
// ================================================================
static const uint8_t FONT[37][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x0E,0x13,0x15,0x19,0x0E}, // '0'
    {0x04,0x0C,0x04,0x04,0x0E}, // '1'
    {0x0E,0x11,0x06,0x08,0x1F}, // '2'
    {0x1E,0x01,0x0E,0x01,0x1E}, // '3'
    {0x12,0x12,0x1E,0x02,0x02}, // '4'
    {0x1F,0x10,0x1E,0x01,0x1E}, // '5'
    {0x0E,0x10,0x1E,0x11,0x0E}, // '6'
    {0x1F,0x02,0x04,0x08,0x08}, // '7'
    {0x0E,0x11,0x0E,0x11,0x0E}, // '8'
    {0x0E,0x11,0x0F,0x01,0x0E}, // '9'
    {0x0E,0x11,0x1F,0x11,0x11}, // 'A'
    {0x1E,0x11,0x1E,0x11,0x1E}, // 'B'
    {0x0E,0x10,0x10,0x10,0x0E}, // 'C'
    {0x1E,0x11,0x11,0x11,0x1E}, // 'D'
    {0x1F,0x10,0x1E,0x10,0x1F}, // 'E'
    {0x1F,0x10,0x1E,0x10,0x10}, // 'F'
    {0x0E,0x10,0x16,0x11,0x0F}, // 'G'
    {0x11,0x11,0x1F,0x11,0x11}, // 'H'
    {0x0E,0x04,0x04,0x04,0x0E}, // 'I'
    {0x03,0x01,0x01,0x11,0x0E}, // 'J'
    {0x11,0x12,0x1C,0x12,0x11}, // 'K'
    {0x10,0x10,0x10,0x10,0x1F}, // 'L'
    {0x11,0x1B,0x15,0x11,0x11}, // 'M'
    {0x11,0x19,0x15,0x13,0x11}, // 'N'
    {0x0E,0x11,0x11,0x11,0x0E}, // 'O'
    {0x1E,0x11,0x1E,0x10,0x10}, // 'P'
    {0x0E,0x11,0x11,0x16,0x0D}, // 'Q'
    {0x1E,0x11,0x1E,0x12,0x11}, // 'R'
    {0x0E,0x10,0x0E,0x01,0x1E}, // 'S'
    {0x1F,0x04,0x04,0x04,0x04}, // 'T'
    {0x11,0x11,0x11,0x11,0x0E}, // 'U'
    {0x11,0x11,0x11,0x0A,0x04}, // 'V'
    {0x11,0x11,0x15,0x1B,0x11}, // 'W'
    {0x11,0x0A,0x04,0x0A,0x11}, // 'X'
    {0x11,0x0A,0x04,0x04,0x04}, // 'Y'
    {0x1F,0x02,0x04,0x08,0x1F}, // 'Z'
};

// ================================================================
//  Primitives de dessin
// ================================================================

static void draw_char(SDL_Renderer *r, char c, int x, int y, int sc,
                      uint8_t cr, uint8_t cg, uint8_t cb) {
    int idx;
    if      (c == ' ')              idx = 0;
    else if (c >= '0' && c <= '9') idx = 1  + (c - '0');
    else if (c >= 'A' && c <= 'Z') idx = 11 + (c - 'A');
    else if (c >= 'a' && c <= 'z') idx = 11 + (c - 'a');
    else if (c == '.')             idx = 0; // point = espace (acceptable pour IP)
    else return;

    // Le point '.' est dessiné manuellement (pixel en bas à droite)
    if (c == '.') {
        SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
        SDL_Rect dot = { x + 2*sc, y + 4*sc, sc, sc };
        SDL_RenderFillRect(r, &dot);
        return;
    }

    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (FONT[idx][row] & (0x10 >> col)) {
                SDL_Rect px = { x + col*sc, y + row*sc, sc, sc };
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

static void draw_text(SDL_Renderer *r, const char *s, int x, int y, int sc,
                      uint8_t cr, uint8_t cg, uint8_t cb) {
    int pas = (5 + 1) * sc;
    for (int i = 0; s[i]; i++)
        draw_char(r, s[i], x + i * pas, y, sc, cr, cg, cb);
}

static int text_width(const char *s, int sc) {
    return (int)strlen(s) * (5 + 1) * sc;
}

// Bouton rectangle avec label centré
// actif=1 → fond coloré + texte blanc ; actif=0 → fond sombre + texte teinté
static void draw_btn(SDL_Renderer *r, SDL_Rect rect, const char *label,
                     int actif, uint8_t br, uint8_t bg, uint8_t bb) {
    SDL_SetRenderDrawColor(r, actif ? br : 38, actif ? bg : 38, actif ? bb : 50, 255);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, br, bg, bb, actif ? 255 : 110);
    SDL_RenderDrawRect(r, &rect);
    int sc = 2;
    int tw = text_width(label, sc);
    int tx = rect.x + (rect.w - tw) / 2;
    int ty = rect.y + (rect.h - 5*sc) / 2;
    draw_text(r, label, tx, ty, sc,
              actif ? 255 : br, actif ? 255 : bg, actif ? 255 : bb);
}

// Bande titre commune
static void draw_header(SDL_Renderer *r, const char *titre) {
    SDL_Rect band = {0, 0, WINDOW_SIZE, 42};
    SDL_SetRenderDrawColor(r, 24, 24, 38, 255);
    SDL_RenderFillRect(r, &band);
    int tx = (WINDOW_SIZE - text_width(titre, 3)) / 2;
    draw_text(r, titre, tx, 12, 3, 255, 195, 45);
    SDL_SetRenderDrawColor(r, 255, 195, 45, 255);
    SDL_RenderDrawLine(r, 0, 42, WINDOW_SIZE, 42);
}

// ================================================================
//  Données des joueurs
// ================================================================
static const uint8_t JCOLS[5][3] = {
    {0,0,0}, {240,240,240}, {40,40,40}, {210,55,55}, {55,100,215}
};
static const char *JNOMS[5] = { "", "BLANC", "NOIR", "ROUGE", "BLEU" };

// ================================================================
//  ÉCRAN 1 — Choix du mode : SOLO / RESEAU
// ================================================================
static int ecran_mode(SDL_Renderer *r) {
    // Retourne : 0=SOLO, 1=RESEAU, -1=quitter
    SDL_Event e;
    while (1) {
        SDL_SetRenderDrawColor(r, 16, 16, 26, 255);
        SDL_RenderClear(r);
        draw_header(r, "ECHECS 4 JOUEURS");

        draw_text(r, "CHOISISSEZ UN MODE",
                  (WINDOW_SIZE - text_width("CHOISISSEZ UN MODE", 2)) / 2,
                  80, 2, 160, 160, 180);

        SDL_Rect b_solo   = {(WINDOW_SIZE/2 - 220), 160, 200, 60};
        SDL_Rect b_reseau = {(WINDOW_SIZE/2 + 20),  160, 200, 60};
        draw_btn(r, b_solo,   "SOLO",   1, 60, 160, 255);
        draw_btn(r, b_reseau, "RESEAU", 1, 255, 160, 40);

        // Description
        draw_text(r, "1 A 4 JOUEURS",
                  b_solo.x + (b_solo.w - text_width("1 A 4 JOUEURS",1))/2,
                  b_solo.y + 68, 1, 100, 130, 180);
        draw_text(r, "VIA SWITCH ETHERNET",
                  b_reseau.x + (b_reseau.w - text_width("VIA SWITCH ETHERNET",1))/2,
                  b_reseau.y + 68, 1, 180, 120, 60);

        SDL_RenderPresent(r);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return -1;
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                if (mx >= b_solo.x && mx <= b_solo.x+b_solo.w &&
                    my >= b_solo.y && my <= b_solo.y+b_solo.h) return 0;
                if (mx >= b_reseau.x && mx <= b_reseau.x+b_reseau.w &&
                    my >= b_reseau.y && my <= b_reseau.y+b_reseau.h) return 1;
            }
        }
        SDL_Delay(16);
    }
}

// ================================================================
//  ÉCRAN 2 — Config joueurs solo (identique à l'implémentation précédente)
// ================================================================
static int compter_actifs(GameConfig *c) {
    int n = 0; for (int i=1;i<=4;i++) n += c->joueur_actif[i]; return n;
}

static int ecran_config_solo(SDL_Renderer *r, GameConfig *cfg) {
    SDL_Event e;
    int survol = 0;
    while (1) {
        SDL_SetRenderDrawColor(r, 16, 16, 26, 255);
        SDL_RenderClear(r);
        draw_header(r, "CONFIGURATION");

        // Colonnes
        draw_text(r, "JOUEUR", 8, 48, 1, 100,100,120);
        draw_text(r, "STATUT", 92, 48, 1, 100,100,120);
        draw_text(r, "MODE",  245, 48, 1, 100,100,120);

        for (int i=1; i<=4; i++) {
            int ry = 58 + (i-1)*88;
            uint8_t cr=JCOLS[i][0], cg=JCOLS[i][1], cb=JCOLS[i][2];

            // Carré couleur (toggle actif)
            SDL_Rect sq = {5, ry+2, 80, 78};
            SDL_SetRenderDrawColor(r, cfg->joueur_actif[i] ? cr:35,
                                      cfg->joueur_actif[i] ? cg:35,
                                      cfg->joueur_actif[i] ? cb:45, 255);
            SDL_RenderFillRect(r, &sq);
            SDL_SetRenderDrawColor(r, cr, cg, cb, 200);
            SDL_RenderDrawRect(r, &sq);
            int nw = text_width(JNOMS[i], 1);
            draw_text(r, JNOMS[i], sq.x+(sq.w-nw)/2, sq.y+(sq.h-5)/2, 1,
                      cfg->joueur_actif[i]?240:80,
                      cfg->joueur_actif[i]?240:80,
                      cfg->joueur_actif[i]?240:80);

            // Statut + hint
            draw_text(r, cfg->joueur_actif[i]?"ACTIF":"INACTIF", 92, ry+10, 1,
                      cfg->joueur_actif[i]?60:180,
                      cfg->joueur_actif[i]?200:70,
                      cfg->joueur_actif[i]?60:60);
            draw_text(r, "CLIC POUR", 92, ry+22, 1, 60,60,75);
            draw_text(r, "BASCULER", 92, ry+30, 1, 60,60,75);

            // Boutons HUMAIN / IA
            if (cfg->joueur_actif[i]) {
                SDL_Rect bh  = {92, ry+48, 150, 28};
                SDL_Rect bia = {248, ry+48, 80, 28};
                draw_btn(r, bh,  "HUMAIN", cfg->joueur_ia[i]==0, cr,cg,cb);
                draw_btn(r, bia, "IA",    cfg->joueur_ia[i]==1, cr,cg,cb);
            } else {
                draw_text(r, "DESACTIVE", 96, ry+55, 1, 55,55,65);
            }
            SDL_SetRenderDrawColor(r, 35,35,50,255);
            SDL_RenderDrawLine(r, 0, ry+86, WINDOW_SIZE, ry+86);
        }

        // Bouton JOUER
        SDL_Rect bj = {(WINDOW_SIZE-200)/2, 415, 200, 45};
        uint8_t jg = survol ? 240 : 200;
        draw_btn(r, bj, "JOUER", 1, 50, jg, 50);
        draw_text(r, "MIN 2 JOUEURS ACTIFS",
                  (WINDOW_SIZE - text_width("MIN 2 JOUEURS ACTIFS",1))/2,
                  465, 1, 70,70,85);

        SDL_RenderPresent(r);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 0;
            if (e.type == SDL_MOUSEMOTION) {
                int mx=e.motion.x, my=e.motion.y;
                survol = (mx>=(WINDOW_SIZE-200)/2 && mx<=(WINDOW_SIZE+200)/2 &&
                          my>=415 && my<=460);
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx=e.button.x, my=e.button.y;
                if (mx>=(WINDOW_SIZE-200)/2 && mx<=(WINDOW_SIZE+200)/2 &&
                    my>=415 && my<=460 && compter_actifs(cfg)>=2) return 1;
                for (int i=1; i<=4; i++) {
                    int ry=58+(i-1)*88;
                    if (mx>=5&&mx<=85&&my>=ry+2&&my<=ry+80) {
                        if (cfg->joueur_actif[i] && compter_actifs(cfg)>2)
                            cfg->joueur_actif[i]=0;
                        else if (!cfg->joueur_actif[i])
                            cfg->joueur_actif[i]=1;
                    }
                    if (cfg->joueur_actif[i]) {
                        int by=ry+48;
                        if (mx>=92&&mx<=242&&my>=by&&my<=by+28) cfg->joueur_ia[i]=0;
                        if (mx>=248&&mx<=328&&my>=by&&my<=by+28) cfg->joueur_ia[i]=1;
                    }
                }
            }
        }
        SDL_Delay(16);
    }
}

// ================================================================
//  ÉCRAN 3 — Choix réseau : HÉBERGER / REJOINDRE
// ================================================================
static int ecran_net_choix(SDL_Renderer *r) {
    // Retourne : 1=héberger, 2=rejoindre, -1=quitter, 0=retour
    SDL_Event e;
    while (1) {
        SDL_SetRenderDrawColor(r, 16, 16, 26, 255);
        SDL_RenderClear(r);
        draw_header(r, "MODE RESEAU");

        draw_text(r, "CHOISISSEZ VOTRE ROLE",
                  (WINDOW_SIZE-text_width("CHOISISSEZ VOTRE ROLE",2))/2,
                  70, 2, 160,160,180);

        SDL_Rect b_host = {(WINDOW_SIZE/2-220), 160, 200, 60};
        SDL_Rect b_join = {(WINDOW_SIZE/2+20),  160, 200, 60};
        draw_btn(r, b_host, "HEBERGER", 1, 255, 140, 40);
        draw_btn(r, b_join, "REJOINDRE",1, 40, 180, 255);

        draw_text(r, "VOUS ETES LE SERVEUR",
                  b_host.x + (b_host.w - text_width("VOUS ETES LE SERVEUR",1))/2,
                  b_host.y+68, 1, 180,110,40);
        draw_text(r, "VOUS REJOIGNEZ",
                  b_join.x + (b_join.w - text_width("VOUS REJOIGNEZ",1))/2,
                  b_join.y+68, 1, 40,120,180);
        draw_text(r, "UN SERVEUR EXISTANT",
                  b_join.x + (b_join.w - text_width("UN SERVEUR EXISTANT",1))/2,
                  b_join.y+76, 1, 40,120,180);

        // Bouton retour
        SDL_Rect b_ret = {10, 10, 70, 24};
        draw_btn(r, b_ret, "RETOUR", 1, 120,120,140);

        SDL_RenderPresent(r);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return -1;
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx=e.button.x, my=e.button.y;
                if (mx>=b_host.x&&mx<=b_host.x+b_host.w&&
                    my>=b_host.y&&my<=b_host.y+b_host.h) return 1;
                if (mx>=b_join.x&&mx<=b_join.x+b_join.w&&
                    my>=b_join.y&&my<=b_join.y+b_join.h) return 2;
                if (mx>=b_ret.x&&mx<=b_ret.x+b_ret.w&&
                    my>=b_ret.y&&my<=b_ret.y+b_ret.h)   return 0;
            }
        }
        SDL_Delay(16);
    }
}

// ================================================================
//  ÉCRAN 4 — Héberger : attente des clients
// ================================================================
static const char *NOM_COULEUR[5] = {"", "BLANC", "NOIR", "ROUGE", "BLEU"};
static const Color SLOTS[MAX_CLIENTS] = { BLUE, BLACK, WHITE };

static int ecran_heberger(SDL_Renderer *r, Board *b_ptr) {
    // Lance le serveur TCP, attend au moins 1 client, puis le serveur clique JOUER
    if (!reseau_demarrer_serveur(b_ptr)) {
        // Erreur de création du socket
        SDL_SetRenderDrawColor(r, 16,16,26,255); SDL_RenderClear(r);
        draw_header(r, "ERREUR RESEAU");
        draw_text(r, "IMPOSSIBLE DE CREER LE SERVEUR",
                  (WINDOW_SIZE-text_width("IMPOSSIBLE DE CREER LE SERVEUR",1))/2,
                  200, 1, 255, 80, 80);
        SDL_RenderPresent(r); SDL_Delay(2000); return 0;
    }

    SDL_Event e;
    while (1) {
        SDL_SetRenderDrawColor(r, 16, 16, 26, 255);
        SDL_RenderClear(r);
        draw_header(r, "HEBERGER UNE PARTIE");

        // IP locale
        char buf[128];
        draw_text(r, "VOTRE IP :", 40, 58, 2, 160,160,180);
        draw_text(r, net_info.ip_locale,
                  40 + text_width("VOTRE IP : ", 2), 58, 2, 80, 220, 255);

        snprintf(buf, sizeof(buf), "PORT : %d", PORT_JEU);
        draw_text(r, buf,
                  (WINDOW_SIZE-text_width(buf,1))/2, 88, 1, 100,100,120);

        // Joueurs connectés
        snprintf(buf, sizeof(buf), "JOUEURS CONNECTES : %d / %d",
                 net_info.nb_connectes, MAX_CLIENTS);
        draw_text(r, buf,
                  (WINDOW_SIZE-text_width(buf,2))/2, 115, 2, 200, 200, 220);

        // Liste des slots
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int connected = (i < net_info.nb_connectes);
            uint8_t cr=JCOLS[SLOTS[i]][0], cg=JCOLS[SLOTS[i]][1], cb=JCOLS[SLOTS[i]][2];
            char slot_buf[64];
            snprintf(slot_buf, sizeof(slot_buf), "JOUEUR %d : %s",
                     i+2, connected ? NOM_COULEUR[SLOTS[i]] : "EN ATTENTE...");
            int tx = (WINDOW_SIZE - text_width(slot_buf, 2)) / 2;
            int ty = 170 + i * 60;
            draw_text(r, slot_buf, tx, ty, 2,
                      connected ? cr : 80,
                      connected ? cg : 80,
                      connected ? cb : 80);
        }

        // Bouton JOUER (actif si au moins 1 client connecté)
        int peut_jouer = (net_info.nb_connectes >= 1);
        SDL_Rect bj = {(WINDOW_SIZE-200)/2, 380, 200, 48};
        draw_btn(r, bj, "JOUER", peut_jouer, 50, 200, 50);

        if (!peut_jouer)
            draw_text(r, "ATTENDEZ AU MOINS 1 CLIENT",
                      (WINDOW_SIZE-text_width("ATTENDEZ AU MOINS 1 CLIENT",1))/2,
                      434, 1, 80,80,90);

        SDL_RenderPresent(r);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { reseau_fermer(); return 0; }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx=e.button.x, my=e.button.y;
                if (peut_jouer &&
                    mx>=(WINDOW_SIZE-200)/2 && mx<=(WINDOW_SIZE+200)/2 &&
                    my>=380 && my<=428) {
                    reseau_envoyer_start();
                    return 1;
                }
            }
        }
        SDL_Delay(100); // rafraîchissement moins fréquent (attente passive)
    }
}

// ================================================================
//  ÉCRAN 5a — Rejoindre : saisie de l'IP
// ================================================================
static int ecran_saisir_ip(SDL_Renderer *r, char *ip_buf, int max_len) {
    // Retourne 1 si l'utilisateur valide, 0 s'il quitte, -1 retour
    ip_buf[0] = '\0';
    SDL_Event e;
    SDL_StartTextInput();

    while (1) {
        SDL_SetRenderDrawColor(r, 16,16,26,255);
        SDL_RenderClear(r);
        draw_header(r, "REJOINDRE UNE PARTIE");

        draw_text(r, "ENTREZ L IP DU SERVEUR :",
                  (WINDOW_SIZE-text_width("ENTREZ L IP DU SERVEUR :",2))/2,
                  80, 2, 160,160,180);

        // Zone de saisie
        SDL_Rect box = {60, 130, WINDOW_SIZE-120, 36};
        SDL_SetRenderDrawColor(r, 25,25,44,255);
        SDL_RenderFillRect(r, &box);
        SDL_SetRenderDrawColor(r, 80,100,200,255);
        SDL_RenderDrawRect(r, &box);

        draw_text(r, ip_buf, box.x+8, box.y+8, 2, 180,220,255);

        // Curseur clignotant
        if ((SDL_GetTicks() / 500) % 2) {
            int cx = box.x + 8 + text_width(ip_buf, 2);
            SDL_Rect cur = {cx, box.y+6, 2, 24};
            SDL_SetRenderDrawColor(r, 180,220,255,255);
            SDL_RenderFillRect(r, &cur);
        }

        draw_text(r, "EXEMPLE : 192.168.1.42",
                  (WINDOW_SIZE-text_width("EXEMPLE : 192.168.1.42",1))/2,
                  175, 1, 70,70,90);

        // Boutons
        SDL_Rect b_conn = {(WINDOW_SIZE-200)/2, 230, 200, 44};
        SDL_Rect b_ret  = {10, 10, 70, 24};
        draw_btn(r, b_conn, "CONNECTER", strlen(ip_buf)>0, 40, 180, 255);
        draw_btn(r, b_ret,  "RETOUR", 1, 120,120,140);

        SDL_RenderPresent(r);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { SDL_StopTextInput(); return 0; }

            if (e.type == SDL_TEXTINPUT) {
                // Autoriser uniquement 0-9 et '.'
                char c = e.text.text[0];
                int len = (int)strlen(ip_buf);
                if (len < max_len - 1 && ((c >= '0' && c <= '9') || c == '.')) {
                    ip_buf[len] = c;
                    ip_buf[len+1] = '\0';
                }
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(ip_buf) > 0)
                    ip_buf[strlen(ip_buf)-1] = '\0';
                if (e.key.keysym.sym == SDLK_RETURN && strlen(ip_buf) > 0) {
                    SDL_StopTextInput(); return 1;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    SDL_StopTextInput(); return -1;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx=e.button.x, my=e.button.y;
                if (strlen(ip_buf)>0 &&
                    mx>=(WINDOW_SIZE-200)/2 && mx<=(WINDOW_SIZE+200)/2 &&
                    my>=230 && my<=274) { SDL_StopTextInput(); return 1; }
                if (mx>=b_ret.x&&mx<=b_ret.x+b_ret.w&&
                    my>=b_ret.y&&my<=b_ret.y+b_ret.h) { SDL_StopTextInput(); return -1; }
            }
        }
        SDL_Delay(16);
    }
}

// ================================================================
//  ÉCRAN 5b — Rejoindre : connexion + attente du START
// ================================================================
static int ecran_rejoindre(SDL_Renderer *r, const char *ip, Board *b_ptr) {
    // Tentative de connexion
    SDL_SetRenderDrawColor(r, 16,16,26,255); SDL_RenderClear(r);
    draw_header(r, "CONNEXION...");
    draw_text(r, "CONNEXION EN COURS...",
              (WINDOW_SIZE-text_width("CONNEXION EN COURS...",2))/2, 200, 2, 160,160,180);
    SDL_RenderPresent(r);

    if (!reseau_rejoindre(ip, b_ptr)) {
        SDL_SetRenderDrawColor(r, 16,16,26,255); SDL_RenderClear(r);
        draw_header(r, "ERREUR CONNEXION");
        draw_text(r, "IMPOSSIBLE DE REJOINDRE",
                  (WINDOW_SIZE-text_width("IMPOSSIBLE DE REJOINDRE",2))/2, 180, 2, 255,80,80);
        draw_text(r, ip,
                  (WINDOW_SIZE-text_width(ip,2))/2, 210, 2, 255,120,80);
        SDL_RenderPresent(r);
        SDL_Delay(2500);
        return 0;
    }

    // Connexion réussie — attendre le signal START
    SDL_Event e;
    while (!net_info.pret) {
        SDL_SetRenderDrawColor(r, 16,16,26,255); SDL_RenderClear(r);
        draw_header(r, "EN ATTENTE");

        draw_text(r, "CONNECTE AU SERVEUR",
                  (WINDOW_SIZE-text_width("CONNECTE AU SERVEUR",2))/2, 80, 2, 60,200,80);

        // Couleur assignée
        uint8_t cr=JCOLS[net_info.ma_couleur][0];
        uint8_t cg=JCOLS[net_info.ma_couleur][1];
        uint8_t cb=JCOLS[net_info.ma_couleur][2];
        draw_text(r, "VOUS JOUEZ :",
                  (WINDOW_SIZE-text_width("VOUS JOUEZ :",2))/2, 140, 2, 160,160,180);
        draw_text(r, NOM_COULEUR[net_info.ma_couleur],
                  (WINDOW_SIZE-text_width(NOM_COULEUR[net_info.ma_couleur],3))/2,
                  170, 3, cr, cg, cb);

        // Indicateur d'attente animé
        const char *pts[] = { ".", "..", "...", "...." };
        draw_text(r, "EN ATTENTE DU SERVEUR",
                  (WINDOW_SIZE-text_width("EN ATTENTE DU SERVEUR",2))/2, 260, 2, 100,100,120);
        draw_text(r, pts[(SDL_GetTicks()/400)%4],
                  (WINDOW_SIZE-text_width("....",2))/2, 284, 2, 100,100,120);

        SDL_RenderPresent(r);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { reseau_fermer(); return 0; }
        }
        SDL_Delay(100);
    }
    return 1;
}

// ================================================================
//  Point d'entrée public
// ================================================================
int afficher_menu(SDL_Renderer *renderer, GameConfig *config) {
    // Config par défaut
    for (int i = 1; i <= 4; i++) {
        config->joueur_actif[i] = 1;
        config->joueur_ia[i]    = 0;
    }
    config->mode_reseau = 0;
    config->ip_serveur[0] = '\0';

    // Plateau temporaire pour passer à reseau_*
    // (le vrai Board est créé dans main.c, mais reseau_* en a besoin pour g_board)
    // On passe NULL ici et on le réassigne dans main.c via reseau_demarrer_serveur/rejoindre
    // NOTE : pour simplifier, on alloue un Board statique temporaire dans le menu.
    static Board board_temp;

    while (1) {
        // Écran 1 : solo ou réseau
        int mode = ecran_mode(renderer);
        if (mode == -1) return 0; // quitter

        if (mode == 0) {
            // --- SOLO ---
            config->mode_reseau = 0;
            int ret = ecran_config_solo(renderer, config);
            if (ret == 0) return 0;
            if (ret == 1) return 1;
        } else {
            // --- RÉSEAU ---
            int choix = ecran_net_choix(renderer);
            if (choix == -1) return 0;
            if (choix == 0)  continue; // retour

            if (choix == 1) {
                // Héberger
                config->mode_reseau = 1;
                int ret = ecran_heberger(renderer, &board_temp);
                if (ret == 0) { reseau_fermer(); continue; }
                // Succès : START envoyé
                return 1;
            } else {
                // Rejoindre : saisir l'IP
                config->mode_reseau = 2;
                int ret_ip = ecran_saisir_ip(renderer, config->ip_serveur,
                                             (int)sizeof(config->ip_serveur));
                if (ret_ip <= 0) continue; // retour ou quitter

                int ret = ecran_rejoindre(renderer, config->ip_serveur, &board_temp);
                if (ret == 0) { reseau_fermer(); continue; }
                return 1;
            }
        }
    }
}
 