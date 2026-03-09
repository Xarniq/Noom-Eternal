# Serveur NoomEternal - Documentation d'Architecture et de Design

**Version:** 1.0  
**Date:** 8 janvier 2026  

## Vue d'ensemble du projet

Le serveur de jeu NoomEternal est une application C multi-thread et pilotée par événements implémentant un jeu de glissement de pièces pour deux joueurs avec progression persistante du joueur, inventaire cosmétique et coordination des matchs en temps réel.

### Caractéristiques principales

- **Matchs deux joueurs en temps réel** : État du plateau synchronisé avec jeu au tour par tour
- **Système de progression persistant** : Gestion du niveau, de l'expérience et de la devise du jeu
- **Inventaire cosmétique** : Skins de personnages achetables et équipables
- **Lobby sécurisé par thread** : Coordination des matchs protégée par mutex pour clients concurrents
- **Protocole sans état** : Communication basée sur les messages EMAP (voir la documentation de la bibliothèque EMAP)
- **Gestion d'erreurs robuste** : Isolation par client avec dégradation gracieuse

## Architecture

### Conception haut niveau

```
┌─────────────────────────────────────────────┐
│         Serveur TCP (main.c)                │
│  Écoute sur 0.0.0.0:5001, Accept Backlog=2 │
└──────────────┬──────────────────────────────┘
               │
        ┌──────┴──────┐
        │             │
   [Client 1]    [Client 2]
        │             │
        ▼             ▼
  ┌─────────────────────────────┐
  │  ServerLobby partagé        │
  │  - Mutex (sécurité thread)  │
  │  - Variables de condition   │
  │  - État du plateau [12]     │
  │  - Suivi des tours         │
  │  - Détection du gagnant    │
  └─────────────────────────────┘
        ▲             ▲
        │             │
  [BD Joueur]    [Logique de jeu]
  (db_manager)   (game_logic)
```

### Organisation des modules

#### 1. **Couche de connexion** (`server_connection.h/c`)

- Opérations TCP socket bas niveau
- Résolution d'adresses indépendante de la plateforme via `getaddrinfo()`
- Option de socket SO_REUSEADDR pour redémarrage du serveur
- Fermeture sécurisée du descripteur de fichier

**Fonctions clés:**

- `srv_create_listener()`: Crée un socket d'écoute
- `srv_accept_client()`: Accepte les connexions entrantes
- `srv_set_reuseaddr()`: Configure la réutilisation du socket
- `srv_close_fd()`: Nettoyage sécurisé du descripteur

#### 2. **Threads de travail** (`server_worker.h/c`)

- Orchestration de session client
- Implémentation de la machine d'état du protocole
- Coordination des matchs via lobby partagé
- Gestion des transactions de boutique

**Composants clés:**

- `server_client_thread()`: Gestionnaire client principal (point d'entrée par client)
- `ServerLobby`: Structure d'état du match partagée
- Cycle de vie du match: Authentification → Lobby → Jeu → Récompenses

**Motif de sécurité des threads:**

```c
pthread_mutex_lock(&lobby->mutex);
// Section critique - modifier l'état du lobby
if (state_changed) {
    pthread_cond_broadcast(&lobby->cond);  // Réveille les threads en attente
}
pthread_mutex_unlock(&lobby->mutex);
```

#### 3. **Logique de jeu** (`game_logic.h/c`)

- Représentation et validation de l'état du plateau
- Validation du mouvement au tour par tour
- Détection de la fin de jeu
- Génération de plateau aléatoire

**Disposition du plateau:**

- Tableau linéaire de 12 cellules
- Valeurs de cellule : 1 = pièce, 0 = vide
- Les pièces glissent vers la gauche (l'indice diminue)
- Ne peut pas passer des pièces adjacentes

**Règles de validation du mouvement:**

1. Les deux indices dans la plage [0, 11]
2. Destination < source (mouvement vers la gauche)
3. La source contient une pièce (`board[source] == 1`)
4. Destination vide (`board[dest] == 0`)
5. Pas de blocage : la destination doit être à droite de la pièce gauche la plus proche

**Condition terminale:**
Le jeu se termine quand aucune pièce n'a d'espace vide à sa gauche (aucun mouvement légal disponible).

#### 4. **Couche base de données** (`db_manager.h/c`)

- Base de données fichier plat binaire (fichier ajout seul)
- Persistance du profil joueur
- Application des récompenses de match
- Gestion de l'inventaire cosmétique

**Format de base de données:**

- Fichier : `db/player_db.bin`
- Enregistrements : structures `PlayerRecord` de taille fixe
- Recherche : scan linéaire par nom d'utilisateur (pas d'indexation)
- Upsert : mise à jour sur place si trouvée, ajout si nouvelle

**Schéma de profil joueur:**

```
struct PlayerRecord {
    char username[32];              // Clé primaire
    uint8_t level;                  // Niveau actuel (1-255)
    uint16_t progression;           // XP vers le prochain niveau (0-999)
    uint16_t money;                 // Devise âme (0-65535)
    uint8_t owned_skins[N];         // Tableau booléen des skins possédés
    uint8_t selected_skin;          // Index du skin actuellement équipé
}
```

**Tableau des récompenses:**

- Victoire : +200 XP, +250 Âmes
- Défaite : +50 XP, +100 Âmes
- Jalon XP : 1000 XP = 1 niveau
- Plafond : Âmes plafonnées à UINT16_MAX

**Normalisation:**
Après chaque opération de base de données, les enregistrements sont normalisés pour assurer:

- Au moins un skin est possédé
- Le skin sélectionné est valide ou réinitialisé par défaut
- Pas de sélections orphelines

### Flux du protocole

#### Phase de connexion

```
Client                              Serveur
   │                                  │
   ├────────── CONNECT ──────────────>│
   │                                  │ Décoder le nom d'utilisateur
   │                                  │ Charger le profil joueur
   │<───────── ACK_OK ────────────────┤
   │                                  │
   │<──── PLAYER_INFO ────────────────┤
   │ (niveau, xp, âmes, skins)       │
   │                                  │
   ├────────── ACK_OK ────────────────>│
```

#### Phase lobby et boutique

```
Client                              Serveur
   │                                  │
   ├─── GAME_JOIN / SHOP_ACTION ─────>│
   │                                  │
   │  (si SHOP_ACTION)                │
   │<───── PLAYER_INFO ────────────────┤
   │ (skins/âmes mis à jour)         │
   │                                  │
   │  (si GAME_JOIN)                  │
   │<────── GAME_WAIT ──────────────────┤
   │ (répéter jusqu'à 2 joueurs prêts)│
```

#### Phase de match

```
Joueurs prêts                      Démarrage du jeu
        │                            │
        ├──── GAME_START ───────────>│ (plateau, assignation de tour)
        │                            │
        │<─── GAME_STATE ────────────┤ (plateau initial)
        │                            │
        │                      Boucle de tour
        │                            │
        │<─── GAME_STATE ────────────┤ (avant le tour du joueur)
        │                            │
        │<────── PLAY ──────────────┤ (YOUR_TURN)
        │                            │
        │─────── MOVE ──────────────>│ (coin_index, coin_pos)
        │                            │ Valider et appliquer
        │                            │
        │<─── GAME_STATE ────────────┤ (plateau mis à jour)
        │                            │
   [Répéter ou fin du jeu]           │
        │                            │
        │<─────── GAME_END ─────────┤ (WIN/LOSE)
        │ Récompenses appliquées et persistantes│
```

### Modèle de concurrence

#### Disposition des threads

- **Thread principal** : Boucle d'acceptation unique, aucune I/O bloquante
- **Threads client** : Un par client connecté (détaché)
- **Section critique** : ServerLobby (protégée par mutex)

#### Mécanisme de synchronisation

**Utilisation du mutex:**

- Protège : État `ServerLobby` (plateau, tour, game_over, etc.)
- Maintenu pour : Durée minimale (vérifications/mises à jour d'état uniquement)

**Utilisation de variable de condition:**

- Signal : Changements d'état du plateau (tour passé, jeu terminé)
- Attendre : Clients en attente de leur tour
- Motif : Vérifier condition → attendre si faux → libérer mutex pendant l'attente

**Exemple - Jeu au tour par tour:**

```c
// Joueur 2 attend son tour
pthread_mutex_lock(&lobby->mutex);
while (lobby->current_turn != my_id && !lobby->game_over) {
    pthread_cond_wait(&lobby->cond, &lobby->mutex);
    // Mutex auto-libéré pendant l'attente, réacquis au réveil
}
// Maintenant c'est mon tour ou le jeu est terminé
pthread_mutex_unlock(&lobby->mutex);
```

#### Prévention des conditions de course

1. **Snapshot du plateau** : Chaque joueur lit un snapshot avant d'envoyer (empêche d'observer les mises à jour mi-message)
2. **Mises à jour d'état atomiques** : Acquisition simple du mutex pour toutes les mises à jour associées
3. **Sérialisation des tours** : Un seul joueur peut mettre à jour le plateau par tour
4. **Broadcast au changement** : Variable de condition garantit que tous les threads en attente voient le nouvel état

### Gestion des erreurs

#### Erreurs au niveau de la connexion

- Échec de création de socket → Quitter avec message d'erreur
- Échec d'acceptation → Journaliser et continuer l'acceptation
- Déconnexion client → Arrêt immédiat du match (l'adversaire gagne)

#### Erreurs de protocole

- Paquet mal formé → ACK_NOK ou réinitialisation de connexion
- Mouvement invalide → Message ILLEGAL_PLAY (le client réessaie)
- Échec d'authentification → Connexion fermée

#### Erreurs de base de données

- Erreur I/O fichier → Dégradation gracieuse (profil par défaut)
- Fonds insuffisants → Retour -2 (le client voit l'erreur)
- Skin invalide → Retour -3 (validation côté client)

## Exemples de flux de données

### Scénario : Deux joueurs jouant un match

**Configuration:**

- Joueur A se connecte (thread-1), Joueur B se connecte (thread-2)
- Les deux demandent GAME_JOIN
- Le compte du lobby atteint 2

**Exécution du jeu:**

1. Thread principal : Initialise le plateau via `game_init_board()`
2. Thread-1 : Reçoit GAME_START (Joueur A est en premier, obtient coin_pos 0)
3. Thread-2 : Reçoit GAME_START (Joueur B est deuxième, obtient coin_pos 1)
4. Thread-1 : Envoie MOVE (pièce 5→3)
   - Validé par `game_is_move_valid()` ✓
   - Appliqué par `game_apply_move()`
   - Plateau mis à jour dans le lobby
   - Thread-2 réveillé via `cond_broadcast()`
5. Thread-2 : Fait maintenant un mouvement, etc.
6. Finalement : L'état du plateau atteint terminal (pas de mouvements)
7. Thread-X : Jeu terminé, calcule winner_id
8. Les deux threads : Reçoivent PLAYER_INFO mis à jour avec récompenses
9. Les deux threads : Retournent au lobby pour le prochain match

### Scénario : Achat de skin par un joueur

1. Client envoie SHOP_ACTION (PURCHASE, skin_index=2, prix dans la table résolue)
2. `handle_shop_action()` appelée
3. `db_purchase_skin()` exécutée :
   - Charge l'enregistrement du joueur
   - Vérifie les fonds (retour -2 si insuffisant)
   - Déduit les âmes, marque le skin comme possédé
   - Appelle `normalize_record()` (assure la cohérence)
   - Persiste dans `db/player_db.bin`
4. Serveur répond avec PLAYER_INFO mis à jour
5. Client met à jour l'interface utilisateur

## Scalabilité et limitations

### Conception actuelle

- **Threads par client** : O(n) mémoire/changement de contexte pour n clients
- **Scan linéaire de base de données** : Temps de recherche O(n) par joueur
- **Lobby unique** : Sérialise les matchs (max 1 jeu à la fois)

### Améliorations de production

**Pour gérer 1000+ joueurs concurrents:**

1. Thread pool au lieu de threads par client
2. Indexation de base de données ou magasin clé-valeur (Redis)
3. Instances de lobby multiples (sharding par hash)
4. Regroupement de connexions pour accès à base de données
5. I/O non bloquante (epoll/kqueue)

### Contraintes de conception

- Maximum 2 joueurs par match (limite architecturale)
- Pas de jeu d'équipe ou spectateurs
- Instance de serveur unique (pas de réplication)

## Tests et validation

### Tests unitaires (Bibliothèque EMAP)

```
Exécuter : make test -C EMAP/
Couvre : Sérialisation des messages, validation du plateau
```

### Tests d'intégration

**Approche de test manuel:**

1. Démarrer le serveur : `./build/examples/noometernal-server`
2. Connecter 2 clients (console de test ou application mobile)
3. Vérifier : CONNECT → PLAYER_INFO → GAME_JOIN → matchs

**Comportement attendu:**

- Les joueurs voient un état de plateau cohérent
- Les mouvements illégaux sont rejetés
- Les récompenses sont appliquées correctement
- Les skins persistent entre les sessions

## Configuration et réglage

### Paramètres du serveur (dans main.c)

```c
#define PORT "5001"          // Port d'écoute du serveur
#define BACKLOG 2            // Taille de la file d'attente de connexion

// Dans les types EMAP:
#define MAX_PLAYER_USERNAME_SIZE 32
#define TOTAL_SKIN_AMOUNT 6   // Nombre de skins disponibles
```

### Paramètres de base de données (dans db_manager.c)

```c
#define PLAYER_DB_PATH "db/player_db.bin"
#define XP_PER_LEVEL 1000
#define XP_REWARD_WIN 200
#define XP_REWARD_LOSE 50
#define SOUL_REWARD_WIN 250
#define SOUL_REWARD_LOSE 100
#define DEFAULT_SOULS 100
```

## Dépendances

### Bibliothèques externes

- **EMAP** (intégré) : Implémentation du protocole de messages
  - Emplacement : `lib/emap-lib/`
  - Utilisation : Codage/décodage des messages, aides
  - Build : `make build-lib`

### Bibliothèques système

- Threads POSIX (`<pthread.h>`)
- Bibliothèque standard C (`<stdlib.h>`, `<stdio.h>`, `<string.h>`)
- API réseau (`<netdb.h>`, `<arpa/inet.h>`, `<sys/socket.h>`)

## Build et déploiement

### Compilation

```bash
cd NoomEternal-Server
make build       # Construit le serveur et la bibliothèque EMAP
make test        # Exécute les tests unitaires EMAP
./output/noometernal-server  # Démarrer le serveur
```

### Support Docker

```bash
docker build -f Dockerfile -t noometernal-server:latest .
docker run -p 5001:5001 noometernal-server:latest
```

### Initialisation de la base de données

La base de données est créée automatiquement lors de la première connexion d'un joueur.

```
db/
└── player_db.bin  (créé à la demande, format binaire)
```

## Documentation Doxygen

Générer la documentation HTML:

```bash
doxygen Doxyfile
open docs/html/index.html
```

La documentation inclut:

- Référence API complète
- Diagrammes d'architecture (intégrés)
- Définitions et structures de types
- Annotations de sécurité des threads
- Exemples d'utilisation

## Considérations de performance

### Latence

- Mises à jour du match : < 50ms (envoi/réception + logique de jeu)
- Opérations de base de données : ~1ms (fichier binaire, scan linéaire)
- Aller-retour réseau : ~5-20ms (LAN)

### Mémoire

- Par client : ~100 KB (buffer socket + pile de thread)
- Base de données : ~500 octets par joueur
- 1000 joueurs : ~500 MB disque + 100 MB mémoire (clients)

### CPU

- Pourcentage d'attente active : <1% (inactif la plupart du temps)
- Calcul du match : ~100 µs par mouvement (validation + mise à jour d'état)

## Notes de sécurité

### Implémentation actuelle

- Pas d'authentification au-delà du nom d'utilisateur (fait confiance au client)
- Pas de chiffrement (suppose un LAN/réseau de confiance)
- Pas de limitation de débit (vulnérable aux DOS)
- Pas de validation d'entrée sur la longueur du nom d'utilisateur

### Durcissement de production requis

- Validation du nom d'utilisateur (alphanumérique, limites de longueur)
- Jetons d'authentification (JWT, OAuth)
- Chiffrement TLS
- Désinfection des entrées
- Limitation de débit par IP
- Journalisation des transactions de base de données

## Maintenance et débogage

### Journalisation

Tous les événements significatifs sont journalisés sur stdout:

```
[SERVER] Received CONNECT from alice
[DB] Loaded info for alice (level=5 xp=234 souls=1500)
[SERVER] Sent GAME_START to alice (Starting: YES)
[SERVER] Move Valid. Sent ACK OK.
```

### Symboles de débogage

Build avec symboles de débogage:

```bash
make clean
CFLAGS="-g -O0" make build
gdb ./output/noometernal-server
(gdb) break server_worker.c:123
(gdb) run
```

### Problèmes courants

**Problème:** "Address already in use"

- **Cause:** SO_REUSEADDR non défini ou TIME_WAIT encore actif
- **Solution:** Attendre 60 secondes ou lancer avec `SO_REUSEADDR` (déjà implémenté)

**Problème:** Les joueurs voient un plateau obsolète

- **Cause:** Condition de course dans le snapshot du plateau
- **Solution:** S'assurer que toutes les lectures du plateau se font dans la section critique du mutex

**Problème:** Le joueur est bloqué "Attente de l'adversaire"

- **Cause:** Variable de condition non diffusée
- **Solution:** Vérifier que `pthread_cond_broadcast()` est appelée au changement d'état

## Références

- **Protocole EMAP** : Voir [README EMAP](../EMAP/README.md)
- **Règles du jeu** : Détaillées dans [game_logic.h](include/game_logic.h)
- **Schéma de base de données** : [db_manager.c](src/db_manager.c) (types internes)
- **Programmation de socket** : Stevens & Fenner, "Unix Network Programming"

---

**Dernière mise à jour:** 8 janvier 2026  
**Maintenu par:** Équipe de développement NoomEternal
