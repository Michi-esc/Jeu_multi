#include "ia.h"
#include <stdlib.h>
#include <limits.h>

// --- Variables partagées ---
Coup ia_resultat;
pthread_mutex_t mutex_ia = PTHREAD_MUTEX_INITIALIZER;
int ia_en_cours = 0;

// Valeur matérielle de chaque type de pièce (indice = PieceType)
// EMPTY=0, PAWN=10, ROOK=50, KNIGHT=30, BISHOP=30, QUEEN=90, KING=10000
static const int VALEUR_PIECE[] = {0, 10, 50, 30, 30, 90, 10000, 0};

// Argument transmis au thread IA
typedef struct {
    Board board;
    Color couleur_ia;
} IaArgs;

// --- Fonctions internes ---

// Évaluation heuristique : somme matérielle IA - somme matérielle adversaires
static int evaluer_plateau(const Board *b, Color ia) {
    int score = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = b->grid[y][x];
            if (p.type == EMPTY || p.color == NONE) continue;
            int val = VALEUR_PIECE[p.type];
            score += (p.color == ia) ? val : -val;
        }
    }
    return score;
}

// Génère tous les coups légaux pour la couleur `c` sur le plateau `b`.
// Utilise verifier_et_jouer_coup sur des copies pour valider chaque coup.
// Retourne le nombre de coups remplis dans le tableau `coups`.
static int generer_coups(const Board *b, Color c, Coup *coups, int max) {
    int n = 0;
    for (int y1 = 0; y1 < BOARD_SIZE && n < max; y1++) {
        for (int x1 = 0; x1 < BOARD_SIZE && n < max; x1++) {
            if (b->grid[y1][x1].color != c) continue;
            for (int y2 = 0; y2 < BOARD_SIZE && n < max; y2++) {
                for (int x2 = 0; x2 < BOARD_SIZE && n < max; x2++) {
                    if (x1 == x2 && y1 == y2) continue;
                    // Copie locale pour ne pas modifier le plateau source
                    Board tmp = *b;
                    tmp.turn = c;
                    if (verifier_et_jouer_coup(&tmp, x1, y1, x2, y2)) {
                        coups[n++] = (Coup){x1, y1, x2, y2, 1};
                    }
                }
            }
        }
    }
    return n;
}

// Vérifie si le roi de la couleur `c` est encore sur le plateau
static int roi_present(const Board *b, Color c) {
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (b->grid[y][x].type == KING && b->grid[y][x].color == c)
                return 1;
    return 0;
}

// Algorithme Minimax "paranoïaque" avec élagage alpha-bêta.
// Stratégie : l'IA maximise son score, tous les autres joueurs le minimisent.
// b         : plateau courant (copie par valeur — pas de side effect)
// profondeur: nombre de demi-tours restants à explorer
// alpha/beta: bornes pour l'élagage
// ia        : couleur que l'on cherche à optimiser
static int minimax(Board b, int profondeur, int alpha, int beta, Color ia) {
    // Cas terminal : l'IA n'a plus de roi
    if (!roi_present(&b, ia)) return -100000 - profondeur;

    // Feuille de l'arbre : évaluation statique
    if (profondeur == 0) return evaluer_plateau(&b, ia);

    Color current = b.turn;
    int is_maximizing = (current == ia);

    Coup coups[200];
    int n = generer_coups(&b, current, coups, 200);

    // Aucun coup légal : passer la main au joueur suivant
    if (n == 0) {
        Board passe = b;
        Color orig = passe.turn;
        do {
            passe.turn = (passe.turn % 4) + 1;
        } while (!passe.player_active[passe.turn] && passe.turn != orig);
        if (passe.turn == orig) return evaluer_plateau(&b, ia);
        return minimax(passe, profondeur - 1, alpha, beta, ia);
    }

    if (is_maximizing) {
        int best = INT_MIN;
        for (int i = 0; i < n; i++) {
            Board next = b;
            next.turn = current;
            verifier_et_jouer_coup(&next, coups[i].x1, coups[i].y1,
                                          coups[i].x2, coups[i].y2);
            int score = minimax(next, profondeur - 1, alpha, beta, ia);
            if (score > best) best = score;
            if (best > alpha) alpha = best;
            if (beta <= alpha) break; // Élagage bêta
        }
        return best;
    } else {
        int best = INT_MAX;
        for (int i = 0; i < n; i++) {
            Board next = b;
            next.turn = current;
            verifier_et_jouer_coup(&next, coups[i].x1, coups[i].y1,
                                          coups[i].x2, coups[i].y2);
            int score = minimax(next, profondeur - 1, alpha, beta, ia);
            if (score < best) best = score;
            if (best < beta) beta = best;
            if (beta <= alpha) break; // Élagage alpha
        }
        return best;
    }
}

// --- Thread IA ---

void *ia_thread_func(void *arg) {
    IaArgs *args = (IaArgs *)arg;
    Board b = args->board;
    Color ia = args->couleur_ia;
    free(args);

    // Générer tous les coups de l'IA
    Coup coups[200];
    int n = generer_coups(&b, ia, coups, 200);

    Coup meilleur = {0, 0, 0, 0, 0};
    int meilleur_score = INT_MIN;

    for (int i = 0; i < n; i++) {
        Board next = b;
        next.turn = ia;
        verifier_et_jouer_coup(&next, coups[i].x1, coups[i].y1,
                                      coups[i].x2, coups[i].y2);
        int score = minimax(next, IA_PROFONDEUR - 1, INT_MIN, INT_MAX, ia);
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleur = coups[i];
        }
    }

    // Écriture thread-safe du résultat
    pthread_mutex_lock(&mutex_ia);
    ia_resultat = meilleur;
    ia_en_cours = 0;
    pthread_mutex_unlock(&mutex_ia);

    return NULL;
}

void ia_lancer(Board *b, Color couleur_ia) {
    pthread_mutex_lock(&mutex_ia);
    ia_en_cours = 1;
    pthread_mutex_unlock(&mutex_ia);

    IaArgs *args = malloc(sizeof(IaArgs));
    args->board = *b;
    args->couleur_ia = couleur_ia;

    pthread_t tid;
    pthread_create(&tid, NULL, ia_thread_func, args);
    pthread_detach(tid); // Le thread se libère seul quand il a fini
}

Coup ia_recuperer_coup(void) {
    Coup c;
    pthread_mutex_lock(&mutex_ia);
    c = ia_resultat;
    pthread_mutex_unlock(&mutex_ia);
    return c;
}
