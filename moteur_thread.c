#include "moteur_thread.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "regles.h" // Ajout de l'inclusion de regles.h pour la déclaration de verifier_et_jouer_coup

// Structure pour passer les données nécessaires au thread de l'IA ou du Réseau
typedef struct {
    Board *board;
    int *jeu_actif;
} ThreadContext;

static pthread_mutex_t mutex_plateau;

void init_systeme_securite(void) {
    pthread_mutex_init(&mutex_plateau, NULL);
}

void fermer_systeme_securite(void) {
    pthread_mutex_destroy(&mutex_plateau);
}

void recuperer_plateau_securise(Board *dest, Board *source) {
    pthread_mutex_lock(&mutex_plateau);
    memcpy(dest, source, sizeof(Board));
    pthread_mutex_unlock(&mutex_plateau);
}

// Encapsule la logique de jeu dans un verrou pour le multi
int modifier_plateau_securise(Board *b, int x1, int y1, int x2, int y2) {
    pthread_mutex_lock(&mutex_plateau);
    // On appelle la fonction magique du Rôle 1
    int resultat = verifier_et_jouer_coup(b, x1, y1, x2, y2);
    pthread_mutex_unlock(&mutex_plateau);
    return resultat;
}

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