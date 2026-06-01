#include "moteur_thread.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// Structure pour passer les données nécessaires au thread de l'IA ou du Réseau
typedef struct {
    Board *board;
    int *jeu_actif;
} ThreadContext;

// Routine du thread "Ouvrier" (Rôle 4)
void* routine_moteur_ia(void* arg) {
    ThreadContext *ctx = (ThreadContext*)arg;
    
    while (*(ctx->jeu_actif)) {
        // Ici, le Rôle 4 pourra analyser le plateau via recuperer_plateau_securise
        // et proposer un coup via modifier_plateau_securise
        
        // On libère un peu de temps CPU pour ne pas saturer le processeur
        usleep(100000); // 100ms
    }
    return NULL;
}

void lancer_moteur_asynchrone(pthread_t *thread_id, Board *b, int *active_flag) {
    static ThreadContext context;
    context.board = b;
    context.jeu_actif = active_flag;

    if (pthread_create(thread_id, NULL, routine_moteur_ia, &context) != 0) {
        fprintf(stderr, "Erreur : Impossible de créer le thread moteur.\n");
    }
}