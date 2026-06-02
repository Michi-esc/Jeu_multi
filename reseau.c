#include "reseau.h"
#include "moteur_thread.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// ================================================================
//  Abstraction Winsock / POSIX
// ================================================================
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET        sock_t;
  #define INVALIDE      INVALID_SOCKET
  #define FERMER(s)     closesocket(s)
  #define SOCK_ERR      SOCKET_ERROR
  static void winsock_init(void) { WSADATA d; WSAStartup(MAKEWORD(2,2), &d); }
  static void winsock_fin(void)  { WSACleanup(); }
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netdb.h>
  #include <fcntl.h>
  #include <sys/select.h>
  #include <ifaddrs.h>
  #include <errno.h>
  typedef int  sock_t;
  #define INVALIDE  (-1)
  #define FERMER(s) close(s)
  #define SOCK_ERR  (-1)
  static void winsock_init(void) {}
  static void winsock_fin(void)  {}
#endif

// ================================================================
//  État global
// ================================================================
NetInfo net_info = { NET_NONE, NONE, 0, 0, "", NONE, {0,0,0,0,0} };

static sock_t  srv_sock          = (sock_t)INVALIDE;
static sock_t  cli_sock          = (sock_t)INVALIDE;
static sock_t  cli_socks[MAX_CLIENTS];  // côté serveur : un slot par client
static Board  *g_board           = NULL;
static int     g_running         = 0;

// Couleurs assignées aux clients dans l'ordre de connexion
static const Color COULEURS_CLIENTS[MAX_CLIENTS] = { BLACK, RED, BLUE };

static pthread_mutex_t mx_socks = PTHREAD_MUTEX_INITIALIZER;

// ================================================================
//  Helpers : recv / send garantis (gèrent les lectures partielles)
// ================================================================
static int recv_exact(sock_t s, void *buf, int n) {
    char *p = (char *)buf;
    while (n > 0) {
        int r = (int)recv(s, p, (size_t)n, 0);
        if (r <= 0) return 0;
        p += r; n -= r;
    }
    return 1;
}

static int send_exact(sock_t s, const void *buf, int n) {
    const char *p = (const char *)buf;
    while (n > 0) {
        int r = (int)send(s, p, (size_t)n, 0);
        if (r <= 0) return 0;
        p += r; n -= r;
    }
    return 1;
}

// ================================================================
//  Thread de réception — côté SERVEUR (un par client connecté)
// ================================================================
typedef struct { int idx; } CltArg;
static CltArg    clt_args[MAX_CLIENTS];
static pthread_t thr_clients[MAX_CLIENTS];

static void *thread_recv_client(void *arg) {
    int    idx  = ((CltArg *)arg)->idx;
    sock_t sock = cli_socks[idx];
    PacketCoup pkt;

    while (g_running) {
        if (!recv_exact(sock, &pkt, sizeof(pkt))) break;

        // Appliquer le coup via l'accès sécurisé (Rôle 3)
        modifier_plateau_securise(g_board, pkt.x1, pkt.y1, pkt.x2, pkt.y2);

        // Diffuser le coup à tous les AUTRES clients (pas d'écho)
        pthread_mutex_lock(&mx_socks);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != idx && cli_socks[i] != (sock_t)INVALIDE) {
                send_exact(cli_socks[i], &pkt, sizeof(pkt));
            }
        }
        pthread_mutex_unlock(&mx_socks);
    }

    // Client déconnecté
    pthread_mutex_lock(&mx_socks);
    FERMER(cli_socks[idx]);
    cli_socks[idx] = (sock_t)INVALIDE;
    if (net_info.nb_connectes > 0) net_info.nb_connectes--;
    pthread_mutex_unlock(&mx_socks);
    return NULL;
}

// ================================================================
//  Thread d'acceptation des connexions — côté SERVEUR
// ================================================================
static pthread_t thr_accept;

static void *thread_accept(void *arg) {
    (void)arg;
    for (int i = 0; i < MAX_CLIENTS && g_running; i++) {
        struct sockaddr_in addr;
#ifdef _WIN32
        int alen = sizeof(addr);
#else
        socklen_t alen = sizeof(addr);
#endif
        sock_t ns = accept(srv_sock, (struct sockaddr *)&addr, &alen);
        if (ns == (sock_t)INVALIDE) break;

        // Envoyer la couleur assignée (4 octets)
        int32_t c = (int32_t)COULEURS_CLIENTS[i];
        if (!send_exact(ns, &c, sizeof(c))) { FERMER(ns); continue; }

        pthread_mutex_lock(&mx_socks);
        cli_socks[i] = ns;
        net_info.nb_connectes++;
        pthread_mutex_unlock(&mx_socks);

        // Lancer le thread de réception pour ce client
        clt_args[i].idx = i;
        pthread_create(&thr_clients[i], NULL, thread_recv_client, &clt_args[i]);
        pthread_detach(thr_clients[i]);
    }
    return NULL;
}

// ================================================================
//  Thread de réception — côté CLIENT
// ================================================================
static pthread_t thr_client_recv;

static void *thread_recv_serveur(void *arg) {
    (void)arg;
    PacketCoup pkt;
    while (g_running) {
        if (!recv_exact(cli_sock, &pkt, sizeof(pkt))) break;

        // Signal de début de partie (x1=y1=x2=y2 = -1)
        // Le champ couleur encode : bits 0-2 = turn, bits 3-6 = player_active[1..4]
        if (pkt.x1 == -1 && pkt.y1 == -1 && pkt.x2 == -1 && pkt.y2 == -1) {
            int32_t sync = pkt.couleur;
            net_info.sync_turn = (Color)(sync & 0x7);
            for (int i = 1; i <= 4; i++)
                net_info.sync_active[i] = (sync >> (2 + i)) & 1;
            net_info.pret = 1;
            continue;
        }

        // Coup reçu → l'appliquer via l'accès sécurisé
        modifier_plateau_securise(g_board, pkt.x1, pkt.y1, pkt.x2, pkt.y2);
    }
    return NULL;
}

// ================================================================
//  API publique
// ================================================================

int reseau_demarrer_serveur(Board *b) {
    winsock_init();
    g_board  = b;
    g_running = 1;
    net_info.mode       = NET_SERVEUR;
    net_info.ma_couleur = WHITE; // le serveur joue toujours WHITE

    for (int i = 0; i < MAX_CLIENTS; i++) cli_socks[i] = (sock_t)INVALIDE;

    // Création du socket d'écoute
    srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_sock == (sock_t)INVALIDE) return 0;

    int opt = 1;
    setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT_JEU);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCK_ERR ||
        listen(srv_sock, MAX_CLIENTS)                           == SOCK_ERR) {
        FERMER(srv_sock); srv_sock = (sock_t)INVALIDE; return 0;
    }

    // Récupérer l'IP locale pour l'afficher dans le menu
#ifdef _WIN32
    {
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        struct hostent *he = gethostbyname(hostname);
        if (he && he->h_addr_list[0])
            strncpy(net_info.ip_locale,
                    inet_ntoa(*(struct in_addr *)he->h_addr_list[0]),
                    sizeof(net_info.ip_locale) - 1);
        else
            strncpy(net_info.ip_locale, "???", sizeof(net_info.ip_locale) - 1);
    }
#else
    // macOS / Linux : getifaddrs() pour l'IP de la première interface non-loopback
    {
        struct ifaddrs *ifap, *ifa;
        strncpy(net_info.ip_locale, "???", sizeof(net_info.ip_locale) - 1);
        if (getifaddrs(&ifap) == 0) {
            for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr) continue;
                if (ifa->ifa_addr->sa_family != AF_INET) continue;
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                // Ignorer loopback (127.x.x.x)
                if ((ntohl(sa->sin_addr.s_addr) >> 24) == 127) continue;
                strncpy(net_info.ip_locale,
                        inet_ntoa(sa->sin_addr),
                        sizeof(net_info.ip_locale) - 1);
                break;
            }
            freeifaddrs(ifap);
        }
    }
#endif

    // Lancer l'acceptation en arrière-plan (non-bloquant)
    pthread_create(&thr_accept, NULL, thread_accept, NULL);
    pthread_detach(thr_accept);
    return 1;
}

int reseau_rejoindre(const char *ip, Board *b) {
    winsock_init();
    g_board   = b;
    g_running = 1;
    net_info.mode = NET_CLIENT;

    cli_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_sock == (sock_t)INVALIDE) return 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT_JEU);
    addr.sin_addr.s_addr = inet_addr(ip);

    // Connexion avec timeout 3 s via mode non-bloquant + select()
#ifdef _WIN32
    {
        u_long nb = 1;
        ioctlsocket(cli_sock, FIONBIO, &nb);
        connect(cli_sock, (struct sockaddr *)&addr, sizeof(addr));
        fd_set fds; FD_ZERO(&fds); FD_SET(cli_sock, &fds);
        struct timeval tv = {3, 0};
        if (select(0, NULL, &fds, NULL, &tv) <= 0) {
            FERMER(cli_sock); cli_sock = (sock_t)INVALIDE; return 0;
        }
        int err = 0; int elen = sizeof(err);
        getsockopt(cli_sock, SOL_SOCKET, SO_ERROR, (char *)&err, &elen);
        if (err != 0) { FERMER(cli_sock); cli_sock = (sock_t)INVALIDE; return 0; }
        nb = 0; ioctlsocket(cli_sock, FIONBIO, &nb); // repasser en bloquant
    }
#else
    {
        int flags = fcntl(cli_sock, F_GETFL, 0);
        fcntl(cli_sock, F_SETFL, flags | O_NONBLOCK);
        connect(cli_sock, (struct sockaddr *)&addr, sizeof(addr));
        fd_set fds; FD_ZERO(&fds); FD_SET(cli_sock, &fds);
        struct timeval tv = {3, 0};
        if (select(cli_sock + 1, NULL, &fds, NULL, &tv) <= 0) {
            FERMER(cli_sock); cli_sock = (sock_t)INVALIDE; return 0;
        }
        int err = 0; socklen_t elen = sizeof(err);
        getsockopt(cli_sock, SOL_SOCKET, SO_ERROR, &err, &elen);
        if (err != 0) { FERMER(cli_sock); cli_sock = (sock_t)INVALIDE; return 0; }
        fcntl(cli_sock, F_SETFL, flags); // repasser en bloquant
    }
#endif

    // Recevoir la couleur assignée (4 octets)
    int32_t couleur = 0;
    if (!recv_exact(cli_sock, &couleur, sizeof(couleur))) {
        FERMER(cli_sock); cli_sock = (sock_t)INVALIDE; return 0;
    }
    net_info.ma_couleur = (Color)couleur;

    // Lancer le thread d'écoute en arrière-plan
    pthread_create(&thr_client_recv, NULL, thread_recv_serveur, NULL);
    pthread_detach(thr_client_recv);
    return 1;
}

void reseau_envoyer_coup(int x1, int y1, int x2, int y2, Color couleur) {
    PacketCoup pkt;
    pkt.x1      = (int32_t)x1;
    pkt.y1      = (int32_t)y1;
    pkt.x2      = (int32_t)x2;
    pkt.y2      = (int32_t)y2;
    pkt.couleur = (int32_t)couleur;

    if (net_info.mode == NET_SERVEUR) {
        pthread_mutex_lock(&mx_socks);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (cli_socks[i] != (sock_t)INVALIDE)
                send_exact(cli_socks[i], &pkt, sizeof(pkt));
        }
        pthread_mutex_unlock(&mx_socks);
    } else if (net_info.mode == NET_CLIENT) {
        send_exact(cli_sock, &pkt, sizeof(pkt));
    }
}

void reseau_envoyer_start(Board *b) {
    // Encoder l'état du plateau dans couleur :
    // bits 0-2 = b->turn, bits 3-6 = player_active[1..4]
    int32_t sync = (int32_t)b->turn;
    for (int i = 1; i <= 4; i++)
        sync |= (b->player_active[i] << (2 + i));

    PacketCoup pkt = { -1, -1, -1, -1, sync };
    pthread_mutex_lock(&mx_socks);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cli_socks[i] != (sock_t)INVALIDE)
            send_exact(cli_socks[i], &pkt, sizeof(pkt));
    }
    pthread_mutex_unlock(&mx_socks);

    // Stocker le sync pour le serveur aussi (main.c l'utilisera)
    net_info.sync_turn = b->turn;
    for (int i = 1; i <= 4; i++)
        net_info.sync_active[i] = b->player_active[i];
    net_info.pret = 1;
}

void reseau_set_board(Board *b) {
    g_board = b;
}

void reseau_fermer(void) {
    g_running = 0;
    if (srv_sock != (sock_t)INVALIDE) { FERMER(srv_sock); srv_sock = (sock_t)INVALIDE; }
    if (cli_sock != (sock_t)INVALIDE) { FERMER(cli_sock); cli_sock = (sock_t)INVALIDE; }
    pthread_mutex_lock(&mx_socks);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cli_socks[i] != (sock_t)INVALIDE) {
            FERMER(cli_socks[i]);
            cli_socks[i] = (sock_t)INVALIDE;
        }
    }
    pthread_mutex_unlock(&mx_socks);
    winsock_fin();
}
