#ifndef IA_H
#define IA_H

#include "plateau.h"
#include "regles.h"
#include <pthread.h>

// Profondeur de recherche Minimax (2-3 recommandé pour la vitesse)
#define IA_PROFONDEUR 3

// Représente un coup (déplacement d'une pièce)
typedef struct {
    int x1, y1; // Case source
    int x2, y2; // Case destination
    int valide;  // 1 = coup trouvé, 0 = aucun coup légal disponible
} Coup;

// --- Variables partagées (protégées par mutex_ia) ---
// Résultat calculé par le thread IA
extern Coup ia_resultat;
// Mutex pour accéder à ia_resultat et ia_en_cours
extern pthread_mutex_t mutex_ia;
// 1 = l'IA est en train de calculer, 0 = résultat disponible
extern int ia_en_cours;

// Lance un thread détaché qui calcule le meilleur coup pour `couleur_ia`
// La copie du plateau est faite en interne — le plateau original n'est pas modifié.
// Appel non bloquant : retourne immédiatement.
void ia_lancer(Board *b, Color couleur_ia);

// Lit ia_resultat de façon thread-safe (non bloquant)
Coup ia_recuperer_coup(void);

// Fonction exécutée par le thread IA (interne — exposée pour pthread_create)
void *ia_thread_func(void *arg);

#endif
