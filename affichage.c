#include "affichage.h"
#include "regles.h"
#include <string.h>
#include <stdio.h>

// Définition des sprites 8x8 pour chaque type de pièce
const int sprites[7][8][8] = {
    {{0}}, // EMPTY
    { // PAWN
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,1,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,1,1,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0}
    },
    { // ROOK
        {0,0,0,0,0,0,0,0},
        {0,1,0,1,1,0,1,0},
        {0,1,1,1,1,1,1,0},
        {0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0}
    },
    { // KNIGHT
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,1,0,0,0},
        {0,1,1,1,1,1,0,0},
        {0,0,0,1,1,1,0,0},
        {0,0,1,1,1,0,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0}
    },
    { // BISHOP
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,1,1,0,0},
        {0,0,1,0,0,1,0,0},
        {0,0,1,1,1,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0}
    },
    { // QUEEN
        {0,0,0,0,0,0,0,0},
        {0,1,0,1,1,0,1,0},
        {0,1,0,1,1,0,1,0},
        {0,1,1,1,1,1,1,0},
        {0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0}
    },
    { // KING
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,1,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0}
    }
};

// ============================================================
//  Police de caractères 5×5 (pixel art)
// ============================================================
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

static void dessiner_char(SDL_Renderer *r, char c, int x, int y, int scale,
                           uint8_t cr, uint8_t cg, uint8_t cb) {
    int idx;
    if      (c == ' ')                idx = 0;
    else if (c >= '0' && c <= '9')   idx = 1  + (c - '0');
    else if (c >= 'A' && c <= 'Z')   idx = 11 + (c - 'A');
    else if (c >= 'a' && c <= 'z')   idx = 11 + (c - 'a');
    else return;

    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (FONT[idx][row] & (0x10 >> col)) {
                SDL_Rect px = { x + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

void dessiner_texte(SDL_Renderer *r, const char *s, int x, int y, int scale,
                            uint8_t cr, uint8_t cg, uint8_t cb) {
    int pas = (5 + 1) * scale;
    for (int i = 0; s[i]; i++) {
        dessiner_char(r, s[i], x + i * pas, y, scale, cr, cg, cb);
    }
}

int largeur_texte(const char *s, int scale) {
    return (int)strlen(s) * (5 + 1) * scale;
}

int init_sdl(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erreur d'initialisation SDL: %s\n", SDL_GetError());
        return 0;
    }

    // On crée la fenêtre directement à la taille WINDOW_SIZE (qui est maintenant de 560x560)
    *window = SDL_CreateWindow("Echecs Multijoueur", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!*window) {
        printf("Erreur de creation de la fenetre: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        printf("Erreur de creation du renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 0;
    }

    // Cette fonction est capitale : elle dit à SDL que peu importe la taille de la fenêtre,
    // l'espace de dessin "logique" reste WINDOW_SIZE x WINDOW_SIZE.
    SDL_RenderSetLogicalSize(*renderer, WINDOW_SIZE, WINDOW_SIZE);

    return 1;
}

void nettoyer_sdl(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void dessiner_plateau(SDL_Renderer *renderer, Board *b, int sel_x, int sel_y) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            SDL_Rect rect = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
            
            if (!is_valid_cell(x, y)) {
                // Coins morts en gris foncé
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            } else {
                // Damier classique (Beige et Marron)
                if ((x + y) % 2 == 0) SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
                else                  SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
            }
            SDL_RenderFillRect(renderer, &rect);
            
            // Surligner les coups possibles pour la pièce sélectionnée
            if (sel_x != -1 && sel_y != -1 && is_valid_cell(x, y)) {
                if (est_mouvement_legal(b, sel_x, sel_y, x, y)) {
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 150); // Vert semi-transparent
                    SDL_Rect highlight = { rect.x + TILE_SIZE / 3, rect.y + TILE_SIZE / 3, TILE_SIZE / 3, TILE_SIZE / 3 };
                    SDL_RenderFillRect(renderer, &highlight);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }
            }

            // Affichage des pièces sous forme de pixel art
            int p_type = b->grid[y][x].type;
            if (is_valid_cell(x, y) && p_type != EMPTY && p_type <= KING) {
                int c = b->grid[y][x].color;
                int block_size = 5; // Chaque "pixel" de l'image fera 5x5 (8x5=40 pixels par case)
                int offset_x = rect.x + (TILE_SIZE - (8 * block_size)) / 2;
                int offset_y = rect.y + (TILE_SIZE - (8 * block_size)) / 2;

                // 1. Dessiner l'ombre/bordure (les pixels élargis en gris très sombre)
                SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
                for (int sy = 0; sy < 8; sy++) {
                    for (int sx = 0; sx < 8; sx++) {
                        if (sprites[p_type][sy][sx]) {
                            SDL_Rect shadow = { offset_x + sx * block_size - 1, offset_y + sy * block_size - 1, block_size + 2, block_size + 2 };
                            SDL_RenderFillRect(renderer, &shadow);
                        }
                    }
                }

                // 2. Dessiner la couleur principale de la pièce
                switch(c) {
                    case WHITE: SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); break;
                    case BLACK: SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); break;
                    case RED:   SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); break;
                    case BLUE:  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); break;
                    default:    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); break;
                }

                for (int sy = 0; sy < 8; sy++) {
                    for (int sx = 0; sx < 8; sx++) {
                        if (sprites[p_type][sy][sx]) {
                            SDL_Rect p_rect = { offset_x + sx * block_size, offset_y + sy * block_size, block_size, block_size };
                            SDL_RenderFillRect(renderer, &p_rect);
                        }
                    }
                }
            }
            
            // Feedback visuel de la sélection
            if (x == sel_x && y == sel_y) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100); // Jaune semi-transparent
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }
    }
}

void dessiner_interface(SDL_Renderer *renderer, Board *b) {
    // Un simple carré coloré en haut à gauche pour le tour
    SDL_Rect rect = { 10, 10, 40, 40 };
    switch (b->turn) {
        case WHITE: SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); break;
        case BLACK: SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); break;
        case RED:   SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); break;
        case BLUE:  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); break;
        default: break;
    }
    
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &rect);
}