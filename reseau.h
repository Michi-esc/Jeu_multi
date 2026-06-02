#ifndef RESEAU_H
#define RESEAU_H

#include "plateau.h"
#include <stdint.h>

// ================================================================
//  Constantes
// ================================================================
#define PORT_JEU    12345   // port TCP utilisé par toutes les instances
#define MAX_CLIENTS 3       // serveur : 1 hôte (RED) + 3 clients max

// ================================================================
//  Mode réseau courant
// ================================================================
typedef enum {
    NET_NONE    = 0,   // jeu solo (pas de réseau)
    NET_SERVEUR = 1,   // cette instance héberge la partie
    NET_CLIENT  = 2    // cette instance a rejoint une partie
} NetMode;

// ================================================================
//  Paquet de données échangé pendant la partie (20 octets)
//  Représente un coup joué : pièce de (x1,y1) vers (x2,y2)
//  valeur spéciale x1=y1=x2=y2=-1 → signal de début de partie
// ================================================================
typedef struct {
    int32_t x1, y1, x2, y2;
    int32_t couleur; // valeur de l'enum Color
} PacketCoup;

// ================================================================
//  État global réseau (lu dans menu.c et main.c)
// ================================================================
typedef struct {
    NetMode mode;
    Color   ma_couleur;      // couleur assignée à ce joueur
    int     nb_connectes;    // nb de clients actuellement connectés (serveur)
    int     pret;            // 1 = START reçu/envoyé, prêt à jouer
    char    ip_locale[64];   // IP locale à afficher dans l'écran hébergeur
} NetInfo;

extern NetInfo net_info;

// ================================================================
//  API publique
// ================================================================

// SERVEUR : crée le socket d'écoute et lance le thread accept() en
// arrière-plan. Retourne 1 si OK, 0 si erreur. Non-bloquant.
int  reseau_demarrer_serveur(Board *b);

// CLIENT : connexion TCP à ip:PORT_JEU (timeout 3 s).
// Reçoit la couleur assignée. Lance le thread de réception.
// Retourne 1 si connecté, 0 si erreur/timeout.
int  reseau_rejoindre(const char *ip, Board *b);

// Envoie un coup aux autres joueurs après l'avoir joué localement :
//   serveur → tous les clients connectés
//   client  → serveur (qui le diffuse aux autres)
void reseau_envoyer_coup(int x1, int y1, int x2, int y2, Color couleur);

// SERVEUR : envoie le signal START à tous les clients (PacketCoup spécial).
void reseau_envoyer_start(void);

// Réassigne le pointeur vers le plateau réel (à appeler après init_board dans main.c).
// Nécessaire car le menu utilise un board temporaire pour initialiser les threads.
void reseau_set_board(Board *b);

// Ferme toutes les connexions et libère les ressources Winsock.
void reseau_fermer(void);

#endif
