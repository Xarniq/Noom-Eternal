# Serveur NoomEternal - Référence API

**Généré:** 8 janvier 2026  
**Version:** 1.0.0

## Table des matières

1. [Module de connexion](#module-de-connexion)
2. [Module de travail](#module-de-travail)
3. [Module de logique de jeu](#module-de-logique-de-jeu)
4. [Module de gestionnaire de base de données](#module-de-gestionnaire-de-base-de-données)

---

## Module de connexion

**En-tête:** `include/server_connection.h`  
**Implémentation:** `src/server_connection.c`

Opérations TCP socket bas niveau fournissant l'initialisation de socket indépendante de la plateforme, l'acceptation client et le nettoyage.

### Fonctions

#### `int srv_create_listener(const char* host, const char* port, int backlog)`

Créez un socket TCP d'écoute sur l'hôte et le port spécifiés.

**Paramètres:**

- `host` - Adresse IP à lier (par exemple, "0.0.0.0" pour toutes les interfaces, "127.0.0.1" pour localhost)
- `port` - Numéro de port sous forme de chaîne (par exemple, "5001"), plage valide 1-65535
- `backlog` - Nombre maximum de connexions en attente dans la file d'attente (généralement 1-128)

**Retours:**

- En cas de succès : Descripteur de fichier valide (≥ 0)
- En cas d'échec : -1 (errno défini)

**Erreurs:**

- Échec `getaddrinfo()` : Erreur de résolution DNS/adresse
- Échec `socket()` : Impossible de créer un socket (problème d'autorisation/ressource)
- Échec `setsockopt()` : Impossible de définir SO_REUSEADDR
- Échec `bind()` : Adresse déjà en utilisation ou pas d'autorisation
- Échec `listen()` : Impossible de marquer comme en écoute

**Remarques:**

- Définit automatiquement SO_REUSEADDR pour permettre une reliure rapide
- Utilise IPv4 (AF_INET) uniquement
- L'appelant est responsable de la fermeture avec `srv_close_fd()` ou `close()`

**Exemple:**

```c
int listen_fd = srv_create_listener("0.0.0.0", "5001", 2);
if (listen_fd == -1) {
    fprintf(stderr, "Failed to create listener\n");
    exit(1);
}
```

---

#### `int srv_accept_client(int listen_fd, struct sockaddr_storage* addr, socklen_t* addrlen)`

Acceptez une connexion client entrante sur un socket d'écoute.

**Paramètres:**

- `listen_fd` - Socket d'écoute de `srv_create_listener()`
- `addr` - Pointeur optionnel vers `sockaddr_storage` pour l'adresse du client (NULL autorisé)
- `addrlen` - Pointeur optionnel vers la longueur de l'adresse (doit être défini à `sizeof(struct sockaddr_storage)` avant l'appel)

**Retours:**

- En cas de succès : Descripteur de fichier valide (≥ 0) pour le client connecté
- En cas d'échec : -1 (errno défini)

**Remarques:**

- Appel bloquant : attend jusqu'à la disponibilité de la connexion
- Sûr de passer NULL pour addr/addrlen (utilise en interne le stockage temporaire)
- L'appelant est responsable de la fermeture du socket renvoyé

**Exemple:**

```c
struct sockaddr_storage client_addr;
socklen_t addr_len = sizeof(client_addr);
int client_fd = srv_accept_client(listen_fd, &client_addr, &addr_len);
if (client_fd == -1) {
    perror("accept");
} else {
    // Handle client
    close(client_fd);
}
```

---

#### `int srv_set_reuseaddr(int fd)`

Activez la réutilisation de l'adresse de socket (option SO_REUSEADDR).

**Paramètres:**

- `fd` - Descripteur de fichier socket valide

**Retours:**

- 0 en cas de succès
- -1 en cas d'échec (errno défini)

**Remarques:**

- Permet la liaison à l'adresse dans l'état TIME_WAIT (important pour le redémarrage du serveur)
- Doit être appelé avant `bind()`
- Déjà appelé par `srv_create_listener()`, donc rarement nécessaire directement

**Exemple:**

```c
int fd = socket(AF_INET, SOCK_STREAM, 0);
if (srv_set_reuseaddr(fd) == -1) {
    perror("setsockopt");
}
```

---

#### `void srv_close_fd(int fd)`

Fermez un descripteur de fichier socket en toute sécurité.

**Paramètres:**

- `fd` - Descripteur de fichier socket (toute valeur acceptable)

**Retours:**

- void (erreurs supprimées)

**Remarques:**

- Sûr d'appeler avec des valeurs négatives (ignorées silencieusement)
- Sûr d'appeler plusieurs fois (suppression d'erreur)
- Les erreurs de `close()` ne sont pas signalées
- Les I/O suivantes sur le même fd échoueront (comportement attendu)

**Exemple:**

```c
srv_close_fd(client_fd);   // Safe even if already closed
srv_close_fd(-1);           // Safe, does nothing
```

---

## Module de travail

**En-tête:** `include/server_worker.h`  
**Implémentation:** `src/server_worker.c`

Orchestration de session client, coordination des matchs et synchronisation de l'état du jeu.

### Types

#### `struct Thread_args`

Structure d'arguments transmise aux threads de gestion des clients.

```c
typedef struct {
    int fd;                    // Descripteur de fichier socket client
    int id;                    // ID client/joueur dans le lobby (0 ou 1)
    struct ServerLobby* lobby; // Pointeur vers l'état du lobby partagé
} Thread_args;
```

**Propriété du thread:** La fonction prend la propriété et appelle `free()`.

---

#### `struct ServerLobby`

État du lobby de jeu partagé pour la coordination des matchs deux joueurs.

```c
typedef struct ServerLobby {
    pthread_mutex_t mutex;           // Verrou d'exclusion mutuelle
    pthread_cond_t cond;             // Variable de condition pour le signalement
    int client_fds[2];               // Sockets client (emplacements inutilisés = -1)
    int count;                       // Nombre de clients (0, 1 ou 2)
    uint8_t board[12];               // Plateau de jeu (1=pièce, 0=vide)
    int current_turn;                // Index du joueur avec tour actuel (0 ou 1)
    int winner_id;                   // ID du gagnant (-1=aucun, 0 ou 1=index)
    int game_started;                // Booléen : le jeu a commencé
    int game_over;                   // Booléen : le jeu est terminé
} ServerLobby;
```

**Sécurité des threads:** Tous les accès aux champs doivent être protégés par le verrouillage `mutex`.

**Initialisation (dans main):**

```c
static ServerLobby lobby;
pthread_mutex_init(&lobby.mutex, NULL);
pthread_cond_init(&lobby.cond, NULL);
lobby.count = 0;
lobby.game_started = 0;
lobby.game_over = 0;
lobby.current_turn = 0;
lobby.winner_id = -1;
```

---

### Fonctions

#### `void* server_client_thread(void* arg)`

Point d'entrée du thread pour la gestion d'une connexion client unique.

**Paramètres:**

- `arg` - Pointeur vers `Thread_args` (la fonction libère ceci)

**Retours:**

- Toujours NULL après achèvement

**Flux d'exécution:**

1. Extraire les arguments, libérer Thread_args
2. **Phase de connexion:** Recevoir CONNECT, envoyer ACK + PLAYER_INFO
3. **Boucle de lobby:**
   - `wait_for_game_join()` - Gérer GAME_JOIN ou SHOP_ACTION
   - `play_single_match()` - Coordonner le match avec l'autre joueur
4. Quitter quand le client se déconnecte ou erreur d'I/O

**Sécurité des threads:**

- Chaque client s'exécute dans un thread séparé
- ServerLobby partagé accessible uniquement dans les sections critiques de mutex
- Aucun état thread-local statique

**Exemple (dans main):**

```c
Thread_args* arg = malloc(sizeof(Thread_args));
arg->fd = client_fd;
arg->lobby = &lobby;
arg->id = -1;

pthread_t th;
pthread_create(&th, NULL, server_client_thread, arg);
pthread_detach(th);  // Laisser le thread se nettoyer lui-même
```

**Séquence des messages:**

```
Client                          Serveur (server_client_thread)
  |                               |
  |------ CONNECT ------------->  | (extraction du nom d'utilisateur)
  |<------- ACK_OK --------------|
  |                               | (charger le joueur de la BD)
  |<---- PLAYER_INFO ----------  | (niveau, xp, âmes, skins)
  |------- ACK_OK ------------>  |
  |                               |
  | [Boucle : wait_for_game_join + play_single_match]
  |
  | [Déconnexion ou erreur]
  |                               | (retour NULL, fermeture du socket)
```

---

## Module de logique de jeu

**En-tête:** `include/game_logic.h`  
**Implémentation:** `src/game_logic.c`

Règles de jeu principales, validation de l'état du plateau et détection de la condition terminale.

### Résumé des règles du jeu

- **Plateau:** 12 cellules dans un tableau linéaire
- **Pièces:** Pièces (1) et espaces vides (0)
- **Mouvement:** Strictement vers la gauche (l'index diminue)
- **Blocage:** Ne peut pas sauter par-dessus les pièces adjacentes
- **Terminal:** Le jeu se termine quand aucun mouvement n'est possible

### Fonctions

#### `int* random_board_generator(void)`

Générez une configuration de plateau de jeu aléatoire.

**Paramètres:**

- (aucun)

**Retours:**

- Pointeur vers tableau `int[12]` nouvellement alloué
- L'appelant **doit libérer** le pointeur renvoyé

**Contenu du plateau:**

- Exactement 5 pièces (valeur 1)
- Exactement 7 espaces vides (valeur 0)
- Positions randomisées

**Algorithme:**

1. Initialiser toutes les cellules à 0
2. Tant que coins_placed < 5:
   - Générer une position aléatoire [0, 11]
   - Si la cellule est vide, placer une pièce et incrémenter le compte

**Mémoire:** Alloue `sizeof(int) * 12` octets

**Exemple:**

```c
int* board = random_board_generator();
// Utiliser le plateau...
free(board);
```

---

#### `void game_init_board(uint8_t* board)`

Initialisez un plateau de jeu avec une configuration jouable garantie.

**Paramètres:**

- `board` - Tableau pré-alloué de 12 éléments à remplir

**Retours:**

- void (modifie le plateau sur place)

**Algorithme:**

1. Tentez jusqu'à 32 générations aléatoires
2. Gardez la première configuration avec des mouvements légaux disponibles
3. Retombez sur un motif codé en dur si tous échouent : pièces aux positions 2, 4, 6, 8, 10

**Postcondition:**

- Au moins un mouvement légal existe
- `game_is_finished(board) == 0`

**Remarques:**

- Sûr de passer NULL (retour silencieux)
- Utilise `random_board_generator()` et `game_is_finished()`

**Exemple:**

```c
uint8_t board[12];
game_init_board(board);
// le plateau est garanti d'avoir des mouvements légaux
```

---

#### `int game_is_move_valid(const uint8_t* board, uint8_t coin_index, uint8_t coin_pos)`

Validez un mouvement selon les règles du jeu.

**Paramètres:**

- `board` - État actuel du plateau
- `coin_index` - Position source de la pièce (0-11)
- `coin_pos` - Position cible (0-11)

**Retours:**

- 1 si le mouvement est légal
- 0 si le mouvement est illégal ou si le plateau est NULL

**Vérifications de validation (tous doivent réussir):**

1. **Limites:** `0 ≤ coin_index < 12` ET `0 ≤ coin_pos < 12`
2. **Direction:** `coin_pos < coin_index` (mouvement strictement vers la gauche)
3. **Source:** `board[coin_index] == 1` (la source a une pièce)
4. **Destination:** `board[coin_pos] == 0` (destination vide)
5. **Blocage:** `coin_pos > nearest_left_coin` (ne peut pas sauter)

**Exemple de règle de blocage:**

```
Plateau: [0, 0, 1, 1, 0, 1, ...]  (pièces aux positions 2, 3, 5)
Déplacer la pièce à 5 vers 4: INVALIDE (4 ≤ 3, bloque la pièce à 3)
Déplacer la pièce à 5 vers 1: VALIDE (1 > 3 est faux, mais 3 est la pièce de blocage)
```

**Remarques:**

- Ne modifie pas le plateau
- Appelez avant `game_apply_move()` pour assurer la légalité

**Exemple:**

```c
if (game_is_move_valid(board, 5, 3)) {
    game_apply_move(board, 5, 3);
} else {
    // Envoyer ILLEGAL_PLAY au client
}
```

---

#### `void game_apply_move(uint8_t* board, uint8_t coin_index, uint8_t coin_pos)`

Appliquez un mouvement validé à l'état du plateau.

**Paramètres:**

- `board` - État du plateau à modifier (modifié sur place)
- `coin_index` - Position source
- `coin_pos` - Position cible

**Retours:**

- void (modifie le plateau directement)

**Opération:**

```c
board[coin_index] = 0;  // Effacer la source
board[coin_pos] = 1;    // Définir la destination
```

**Préconditions:**

- Le mouvement doit être validé avec `game_is_move_valid()` retournant 1
- Comportement non défini si les préconditions ne sont pas respectées

**Remarques:**

- Très rapide : 2 affectations de tableau
- Pas de validation (responsabilité de l'appelant)
- Idempotent si appelé plusieurs fois avec les mêmes paramètres

**Exemple:**

```c
if (game_is_move_valid(board, coin_index, coin_pos)) {
    game_apply_move(board, coin_index, coin_pos);
    // Le plateau est maintenant mis à jour
}
```

---

#### `int game_is_finished(const uint8_t* board)`

Vérifiez si le jeu a atteint un état terminal.

**Paramètres:**

- `board` - État actuel du plateau

**Retours:**

- 1 s'il ne reste aucun mouvement légal (jeu terminé)
- 0 s'il existe au moins un mouvement légal

**Algorithme:**

1. Balayez le plateau de gauche à droite
2. Tracez si un espace vide a été vu
3. Si une pièce trouvée après un espace vide vu → retour 0 (mouvement possible)
4. Si le balayage se termine sans trouver une telle pièce → retour 1 (terminé)

**Base mathématique:**

- Les pièces ne peuvent se déplacer que vers la gauche
- Si toutes les cellules vides sont à droite de toutes les pièces → aucun mouvement possible
- Si vide existe à gauche d'une pièce → cette pièce peut se déplacer à gauche

**Exemples d'états du plateau:**

```
[0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1] → PAS terminé (les pièces 3,4,5 peuvent se déplacer)
[1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0] → TERMINÉ (toutes les pièces à gauche)
[1, 0, 1, 0, 1, 0, ...] → PAS terminé (toutes les pièces peuvent se déplacer)
```

**Remarques:**

- Complexité O(12) = O(1)
- Appelé après chaque mouvement pour détecter la fin du jeu
- Sûr avec NULL (échouerait, mais l'appelant vérifie)

**Exemple:**

```c
game_apply_move(board, coin_index, coin_pos);
if (game_is_finished(board)) {
    // Déclarer le joueur actuel comme gagnant
    // Envoyer GAME_END aux deux clients
}
```

---

## Module de gestionnaire de base de données

**En-tête:** `include/db_manager.h`  
**Implémentation:** `src/db_manager.c`

Persistance du profil joueur, suivi de la progression et gestion d'inventaire cosmétique.

### Schéma de base de données

**Stockage:** `db/player_db.bin` (fichier binaire ajout seul)

**Format d'enregistrement:**

```c
typedef struct {
    char username[MAX_PLAYER_USERNAME_SIZE];  // Clé primaire
    uint8_t level;                             // 1-255
    uint16_t progression;                      // 0-999 (XP vers le prochain niveau)
    uint16_t money;                            // 0-65535 (âmes)
    uint8_t owned_skins[TOTAL_SKIN_AMOUNT];    // Tableau booléen
    uint8_t selected_skin;                     // Index équipé actuellement
} PlayerRecord;  // ~40 octets chacun
```

### Constantes de configuration

```c
#define PLAYER_DB_PATH "db/player_db.bin"
#define XP_PER_LEVEL 1000
#define XP_REWARD_WIN 200
#define XP_REWARD_LOSE 50
#define SOUL_REWARD_WIN 250
#define SOUL_REWARD_LOSE 100
#define DEFAULT_SOULS 100
```

### Fonctions

#### `int db_get_player_info(const char* username, EMAP_PayloadPlayerInfo* out_info)`

Chargez ou créez un profil joueur à partir de la base de données.

**Paramètres:**

- `username` - Identificateur du joueur (non-NULL)
- `out_info` - Structure de sortie (non-NULL, sera remplie)

**Retours:**

- 0 en cas de succès (joueur trouvé ou créé)
- -1 en cas d'échec (erreur d'accès à la base de données)

**Comportement:**

- Si le joueur existe : Charger à partir de la base de données
- Si le joueur n'est pas trouvé : Créer un profil par défaut avec level=1, âmes=100, skin 0
- Appliquer la normalisation pour assurer la cohérence
- Convertir du format interne au format de charge EMAP

**Champs de sortie (out_info):**

- `level` - Niveau actuel
- `progression` - XP vers le prochain niveau (0-999)
- `money` - Devise d'âme
- `selected_skin` - Index de skin équipé
- `possessed_skins[]` - Drapeaux de propriété de skin (0=aucun, 1=possédé, 2=possédé+sélectionné)

**Défauts du nouveau joueur:**

- Niveau : 1
- Progression : 0 XP
- Argent : 100 âmes
- Possédé : skin 0 uniquement
- Sélectionné : skin 0

**Remarques:**

- Réussit toujours sauf si le fichier de base de données est illisible
- Si le fichier est manquant, le crée à la demande
- Sûr pour les threads (le scan linéaire est atomique)

**Exemple:**

```c
EMAP_PayloadPlayerInfo info;
if (db_get_player_info("alice", &info) == 0) {
    printf("Alice is level %u with %u souls\n", info.level, info.money);
} else {
    printf("Database error\n");
}
```

---

#### `int db_update_player_info(const char* username, const EMAP_PayloadPlayerInfo* info)`

Persistez un profil joueur complet dans la base de données.

**Paramètres:**

- `username` - Identificateur du joueur (non-NULL)
- `info` - Données de profil à persister (non-NULL)

**Retours:**

- 0 en cas de succès
- -1 en cas d'échec (erreur d'écriture de fichier)

**Comportement:**

1. Convertir la charge EMAP au format d'enregistrement interne
2. Chercher le joueur dans la base de données
3. Si trouvé : Remplacer sur place (recherche + écriture)
4. Si non trouvé : Ajouter à la fin du fichier
5. Opération de log

**Conversion de champ:**

- EMAP `possessed_skins[i]` (0/1/2) → Interne `owned_skins[i]` (booléen)
- Skin sélectionné tracé séparément dans les deux formats

**Remarques:**

- **Avertissement:** Remplace le profil entier. Utilisez `db_apply_match_result()` pour les résultats du jeu
- Non-idempotent avec les tentatives réseau (attention aux messages dupliqués)
- Crée le fichier de base de données si nécessaire

**Exemple:**

```c
EMAP_PayloadPlayerInfo updated = {
    .level = 6,
    .progression = 500,
    .money = 2000,
    .selected_skin = 2,
    .possessed_skins = {2, 1, 1, 0, 0, 0}  // Possède les skins 0,1,2; équipé 0
};
if (db_update_player_info("alice", &updated) == 0) {
    printf("Profile persisted\n");
}
```

---

#### `int db_apply_match_result(const char* username, int has_won, EMAP_PayloadPlayerInfo* out_info)`

Appliquez les récompenses de résultat de match au profil d'un joueur.

**Paramètres:**

- `username` - Identificateur du joueur (non-NULL)
- `has_won` - Non-zéro pour victoire, zéro pour défaite
- `out_info` - Tampon de sortie optionnel pour le profil mis à jour (NULL autorisé)

**Retours:**

- 0 en cas de succès
- -1 en cas d'échec (erreur de base de données)

**Application des récompenses:**

```
Victoire : +200 XP, +250 Âmes
Défaite : +50 XP, +100 Âmes
```

**Avancement du niveau:**

- Quand `progression + xp_gain >= 1000`:
  - `progression` enroule (soustrait 1000)
  - `level` s'incrémente (plafonnée à 255)
  - Répéter jusqu'à `progression < 1000`

**Plafonnement des âmes:**

- `money + soul_gain` plafonné à UINT16_MAX (65535)

**Comportement:**

1. Charger ou créer le profil joueur
2. Calculer les récompenses en fonction du résultat
3. Appliquer XP avec logique de montée de niveau
4. Appliquer les âmes avec cap
5. Normaliser l'enregistrement
6. Persister dans la base de données
7. Retourner optionnellement le profil mis à jour

**Exemple:**

```c
EMAP_PayloadPlayerInfo updated;
if (db_apply_match_result("alice", 1, &updated) == 0) {
    printf("Alice won! New level: %u, souls: %u\n", 
           updated.level, updated.money);
}
```

---

#### `int db_purchase_skin(const char* username, uint8_t skin_index, uint16_t price, EMAP_PayloadPlayerInfo* out_info)`

Achetez une skin cosmétique pour un joueur.

**Paramètres:**

- `username` - Identificateur du joueur (non-NULL)
- `skin_index` - Skin à acheter (0 à TOTAL_SKIN_AMOUNT-1)
- `price` - Coût en âmes (ordre d'octets hôte)
- `out_info` - Tampon de sortie optionnel (NULL autorisé)

**Retours:**

- 0 en cas de succès (acheté ou déjà possédé)
- -1 en cas d'échec (erreur I/O de base de données)
- -2 en cas de fonds insuffisants (âmes < prix)
- -3 si skin invalide (skin_index hors de portée)

**Comportement:**

1. Valider skin_index
2. Charger ou créer le joueur
3. Si déjà possédé : Retour 0 (succès, pas de charge)
4. Si fonds insuffisants : Retour -2
5. Déduire les âmes, marquer comme possédé, l'équiper
6. Normaliser et persister
7. Retourner optionnellement le profil mis à jour

**Remarques:**

- Le skin acheté devient automatiquement équipé
- Idempotent (plusieurs achats du même skin en toute sécurité)
- Pas de transactions partielles (tout ou rien)

**Exemple:**

```c
uint16_t skin_price = 150;
int rc = db_purchase_skin("alice", 2, skin_price, NULL);
switch (rc) {
    case 0: printf("Skin purchased\n"); break;
    case -2: printf("Not enough souls\n"); break;
    case -3: printf("Invalid skin\n"); break;
    case -1: printf("Database error\n"); break;
}
```

---

#### `int db_equip_skin(const char* username, uint8_t skin_index, EMAP_PayloadPlayerInfo* out_info)`

Équipez une skin déjà possédée pour un joueur.

**Paramètres:**

- `username` - Identificateur du joueur (non-NULL)
- `skin_index` - Skin à équiper (0 à TOTAL_SKIN_AMOUNT-1)
- `out_info` - Tampon de sortie optionnel (NULL autorisé)

**Retours:**

- 0 en cas de succès (équipé)
- -1 en cas d'échec (erreur I/O de base de données)
- -3 si skin invalide ou non possédé

**Comportement:**

1. Valider skin_index
2. Charger ou créer le joueur
3. Si skin non possédé : Retour -3
4. Définir selected_skin à skin_index
5. Normaliser et persister
6. Retourner optionnellement le profil mis à jour

**Coût:**

- Gratuit (pas de déduction d'âme)

**Exemple:**

```c
if (db_equip_skin("alice", 3, NULL) == 0) {
    printf("Skin equipped\n");
} else {
    printf("Skin not owned\n");
}
```

---

## Diagrammes de flux de données

### Détermination du gagnant du match

```
Le joueur se déplace → état du plateau mis à jour
        ↓
game_is_finished(board) appelé
        ↓
Aucun mouvement légal ne reste?
    ↓               ↓
   OUI              NON
    ↓               ↓
  winner_id    current_turn
   = my_id    = (current_turn + 1) % 2
    ↓               ↓
game_over = 1  cond_broadcast()
    ↓
Les deux joueurs voient GAME_END
    ↓
db_apply_match_result(winner=1, loser=0)
```

### Séquence de mise à jour du profil

```
Demande du client
    ↓
Gestionnaire de thread (server_worker.c)
    ↓
db_get_player_info() ──→ Lecture de la base de données
    ↓
Appliquer la modification (achat/récompense)
    ↓
normalize_record() ──→ Vérification de cohérence des données
    ↓
db_update_player_info() ──→ Écriture de la base de données
    ↓
PLAYER_INFO envoyé au client
```

---

## Conventions de gestion des erreurs

### Modèles de valeur de retour

**Module de connexion:**

- Retourne -1 en cas d'erreur (fonction : `srv_create_listener`, `srv_accept_client`, `srv_set_reuseaddr`)
- Retourne void et ignore les erreurs (fonction : `srv_close_fd`)

**Module de logique de jeu:**

- Retourne 0 pour illégal / faux (fonction : `game_is_move_valid`, `game_is_finished`)
- Retourne 1 pour légal / vrai
- Retourne pointeur (fonction : `random_board_generator`)
- Retourne void (fonctions : `game_apply_move`, `game_init_board`)

**Module de base de données:**

- Retourne 0 en cas de succès
- Retourne négatif en cas d'échec (code spécifique indique le problème):
  - -1 : Erreur I/O de base de données
  - -2 : Fonds insuffisants (achat uniquement)
  - -3 : Index de skin invalide

**Module de travail:**

- Retourne bool (vrai = continuer, faux = déconnecter)
- Retourne NULL à la sortie du thread (fonction : `server_client_thread`)

---

## Résumé de la sécurité des threads

| Module | Sûr pour les threads | Protection |
|--------|-----|-----------|
| Connexion | Oui | Aucun état partagé |
| Travail | Partiellement | ServerLobby protégé par mutex |
| Logique de jeu | Oui | Aucun état partagé (fonctions sans état) |
| Base de données | Oui | Opérations de fichier atomiques |

---

## Historique des versions

- **1.0.0** (8 janvier 2026) : Sortie initiale
  - Implémentation complète de tous les modules
  - Documentation Doxygen complète
  - Coordination de lobby sûre pour les threads

---

**Pour des questions ou des mises à jour, contactez:** Équipe de développement NoomEternal
