#ifndef RESEAU_H
#define RESEAU_H

#include "plateau.h"
#include <stdint.h>

#define PORT_JEU    12345
#define MAX_CLIENTS 3

typedef enum {
    NET_NONE    = 0,
    NET_SERVEUR = 1,
    NET_CLIENT  = 2
} NetMode;

// Version du protocole — doit être identique sur toutes les machines
// Incrémenter si plateau.h change (BOARD_SIZE, enum Color, etc.)
#define PROTO_VERSION 2   // v2 : BOARD_SIZE=14, WHITE/BLACK/RED/BLUE

// Paquet coup (20 octets) — x1=y1=x2=y2=-1 = signal START
typedef struct {
    int32_t x1, y1, x2, y2;
    int32_t couleur;
} PacketCoup;

// État global réseau
typedef struct {
    NetMode mode;
    Color   ma_couleur;
    int     nb_connectes;
    int     pret;
    char    ip_locale[64];
    // Synchronisation initiale reçue du serveur (côté client)
    Color   sync_turn;
    int     sync_active[5]; // player_active[1..4] envoyé par le serveur
} NetInfo;

extern NetInfo net_info;

int  reseau_demarrer_serveur(Board *b);
int  reseau_rejoindre(const char *ip, Board *b);
void reseau_envoyer_coup(int x1, int y1, int x2, int y2, Color couleur);

// SERVEUR : envoie START + état du plateau (turn + player_active) à tous les clients
void reseau_envoyer_start(Board *b);

void reseau_set_board(Board *b);
void reseau_fermer(void);

#endif
