#include "moteur_thread.h"
#include "regles.h"
#include <pthread.h>
#include <string.h>

// Le verrou (Mutex) qui protège l'unique source de vérité : la matrice du plateau
static pthread_mutex_t plateau_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_systeme_securite(void) {
    pthread_mutex_init(&plateau_mutex, NULL);
}

void fermer_systeme_securite(void) {
    pthread_mutex_destroy(&plateau_mutex);
}

void recuperer_plateau_securise(Board *dest, Board *source) {
    pthread_mutex_lock(&plateau_mutex);
    // On effectue une copie mémoire rapide de la structure Board
    memcpy(dest, source, sizeof(Board));
    pthread_mutex_unlock(&plateau_mutex);
}

int modifier_plateau_securise(Board *b, int x1, int y1, int x2, int y2) {
    int succes = 0;
    
    pthread_mutex_lock(&plateau_mutex);
    // On appelle la logique métier du Rôle 1
    succes = verifier_et_jouer_coup(b, x1, y1, x2, y2);
    pthread_mutex_unlock(&plateau_mutex);
    
    return succes;
}