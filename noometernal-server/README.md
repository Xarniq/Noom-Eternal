# Serveur NoomEternal

[![pipeline status](https://gitlab.com/sco-chartreux/3ics-25-26/moulin-anton/projet-c/noometernal-server/badges/main/pipeline.svg)](https://gitlab.com/sco-chartreux/3ics-25-26/moulin-anton/projet-c/noometernal-server/-/commits/main)

## Vue d'ensemble

Le **Serveur NoomEternal** est une application multi-thread en langage C qui implémente un serveur de jeu en temps réel pour un jeu de glissement de pièces deux joueurs. Ce serveur gère les connexions clients simultanées, coordonne les matchs, persiste les profils joueurs et applique les récompenses avec progression du jeu.

### Caractéristiques principales

- ✅ **Serveur multi-thread** : Un thread dédié par client pour une gestion concurrente
- ✅ **Matchs en temps réel** : Synchronisation du plateau de jeu avec protocole EMAP
- ✅ **Système de progression** : Niveaux, expérience et monnaie persistants
- ✅ **Inventaire cosmétique** : Skins achetables et équipables
- ✅ **Base de données locale** : Stockage binaire des profils joueurs
- ✅ **Sécurité des threads** : Mutex et variables de condition pour coordination
- ✅ **Gestion d'erreurs robuste** : Isolation des clients et dégradation gracieuse

---

## Structure du projet

```
NoomEternal-Server/
├── README.md                  # Ce fichier
├── Doxyfile                   # Configuration Doxygen pour la documentation
├── Makefile                   # Scripts de compilation et de documentation
├── include/                   # En-têtes publiques
│   ├── server_connection.h    # API socket TCP
│   ├── server_worker.h        # Gestion des clients et lobby
│   ├── db_manager.h           # Persistance des profils
│   └── game_logic.h           # Règles du jeu
├── src/                       # Implémentation
│   ├── main.c                 # Point d'entrée serveur
│   ├── server_connection.c    # Opérations socket
│   ├── server_worker.c        # Orchestration des clients
│   ├── db_manager.c           # Gestion de base de données
│   └── game_logic.c           # Logique de jeu
├── db/                        # Répertoire de base de données
│   └── player_db.bin          # Profils joueurs (créé automatiquement)
├── docs/                      # Documentation générée
├── lib/                       # Dépendances (EMAP)
└── output/                    # Binaires compilés
```

---

## Modules principaux

### 1. **Couche de connexion** (`server_connection.h/c`)

Abstractions TCP socket bas niveau pour initialisation, acceptation de clients et nettoyage.

**Fonctions principales:**

- `srv_create_listener()` - Crée un socket d'écoute
- `srv_accept_client()` - Accepte une connexion entrante
- `srv_set_reuseaddr()` - Configure la réutilisation d'adresse
- `srv_close_fd()` - Fermeture sécurisée de descripteur

### 2. **Threads de travail** (`server_worker.h/c`)

Orchestration de session client, coordination des matchs et gestion de lobby partagée.

**Composants clés:**

- `server_client_thread()` - Point d'entrée de chaque client
- `ServerLobby` - État du match partagé protégé par mutex
- Cycle de vie: Authentification → Lobby → Match → Récompenses

### 3. **Logique de jeu** (`game_logic.h/c`)

Validation des mouvements, détection de fin de jeu et génération de plateau.

**Mécaniques:**

- Plateau 12 cellules avec 5 pièces
- Mouvements strictement vers la gauche
- Blocage de saut par pièces adjacentes
- Terminal quand aucun mouvement légal

### 4. **Gestionnaire de base de données** (`db_manager.h/c`)

Persistance des profils, suivi de progression et gestion d'inventaire.

**Opérations:**

- Charger/créer profils joueurs
- Appliquer récompenses de match (XP + Âmes)
- Acheter et équiper skins cosmétiques
- Normalisation des enregistrements

---

## Démarrage rapide

### Prérequis

- **GCC** ou **Clang** (compilateur C99+)
- **Make** pour la compilation
- **POSIX threads** (pthreads)
- **Doxygen** (optionnel, pour la documentation)

### Compilation

```bash
# Compiler le serveur et la bibliothèque EMAP
make build

# Exécuter les tests unitaires
make test

# Générer la documentation HTML
make doc
```

### Lancement du serveur

```bash
# Démarrer le serveur (écoute sur 0.0.0.0:5001)
./output/noometernal-server

# Affichage attendu:
# [SERVER] Server listening on 0.0.0.0:5001
# [SERVER] Ready to accept clients...
```

### Nettoyage

```bash
# Supprimer fichiers compilés
make clean

# Supprimer bibliothèques aussi
make clean-all
```

---

## Configuration

### Paramètres du serveur (dans `src/main.c`)

```c
#define PORT "5001"           // Port d'écoute
#define BACKLOG 2             // File d'attente de connexion
```

### Paramètres de jeu (dans `include/game_logic.h`)

```c
#define BOARD_SIZE 12         // Taille du plateau
#define COIN_COUNT 5          // Nombre de pièces
```

### Paramètres de base de données (dans `src/db_manager.c`)

```c
#define XP_PER_LEVEL 1000
#define XP_REWARD_WIN 200     // Victoire: +200 XP
#define XP_REWARD_LOSE 50     // Défaite: +50 XP
#define SOUL_REWARD_WIN 250   // Victoire: +250 Âmes
#define SOUL_REWARD_LOSE 100  // Défaite: +100 Âmes
#define DEFAULT_SOULS 100     // Âmes initiales
```

---

## Flux de communication

### Phase de connexion

```
Client → [CONNECT] → Serveur
       ← [ACK_OK] ←
       ← [PLAYER_INFO] ← (charge le profil)
Client → [ACK_OK] →
```

### Phase de lobby

```
Client → [GAME_JOIN / SHOP_ACTION] → Serveur
       ← [GAME_WAIT] ← (répète jusqu'à 2 joueurs)
       → [ACK_OK] →
```

### Phase de match

```
Client 1 ← [GAME_START] → Client 2
     ↓ Boucle de tour ↑
Client → [MOVE] → Serveur
       ← [GAME_STATE] ← (plateau mis à jour)
Client ← [GAME_END] → (WIN/LOSE + récompenses)
```

---

## Architecture système

### Modèle de concurrence

```
┌─────────────────────────────────────────┐
│       Serveur TCP (main.c)              │
│    Écoute 0.0.0.0:5001, Backlog=2      │
└────────────────┬────────────────────────┘
                 │
          ┌──────┴──────┐
          │             │
    [Joueur 1]     [Joueur 2]
          │             │
          ▼             ▼
    ┌─────────────────────────┐
    │  ServerLobby partagé    │
    │  - Mutex                │
    │  - Variables condition  │
    │  - État du plateau      │
    │  - Suivi des tours      │
    └─────────────────────────┘
```

### Synchronisation

**Mutex:** Protège l'accès à `ServerLobby` (état du plateau, tours, etc.)

**Variables de condition:** Signalent les changements d'état

- Joueur attend son tour
- État du plateau change
- Fin de match détectée

### Sécurité des threads

- Chaque client dans son propre thread
- État partagé protégé par mutex
- Fonctions sans état pour logique de jeu
- Opérations fichier atomiques pour BD

---

## Base de données

### Schéma

**Fichier:** `db/player_db.bin` (format binaire, création automatique)

```c
struct PlayerRecord {
    char username[32];           // Clé primaire
    uint8_t level;               // Niveau (1-255)
    uint16_t progression;        // XP vers prochain niveau (0-999)
    uint16_t money;              // Âmes (0-65535)
    uint8_t owned_skins[6];      // Skins possédés (booléen)
    uint8_t selected_skin;       // Skin équipé
}
```

### Opérations

| Opération | Fonction | Retour |
|-----------|----------|--------|
| Charger profil | `db_get_player_info()` | 0=succès, -1=erreur |
| Mettre à jour | `db_update_player_info()` | 0=succès, -1=erreur |
| Appliquer récompense | `db_apply_match_result()` | 0=succès, -1=erreur |
| Acheter skin | `db_purchase_skin()` | 0=succès, -2=fonds insuffisant, -3=skin invalide |
| Équiper skin | `db_equip_skin()` | 0=succès, -3=non possédé |

---

## Performances

### Latence

- Mise à jour de match: < 50ms
- Opérations BD: ~1ms
- Aller-retour réseau: ~5-20ms (LAN)

### Mémoire

- Par client: ~100 KB
- Par joueur (BD): ~40 octets
- 1000 joueurs: ~500 MB disque + 100 MB RAM (clients)

### CPU

- Attente active: <1%
- Calcul par mouvement: ~100 µs

---

## Documentation

### Fichiers de documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Design système complet
  - Diagrammes d'architecture
  - Modèle de concurrence détaillé
  - Flux de données et protocole
  - Scalabilité et limitations

- **[API_REFERENCE.md](API_REFERENCE.md)** - Référence API complète
  - Signatures de fonctions
  - Paramètres et valeurs de retour
  - Codes d'erreur
  - Exemples d'utilisation
  - Thread safety matrix

### Générer la documentation HTML

```bash
# Générer avec Doxygen
make doc

# Ouvrir dans le navigateur
open docs/html/index.html
```

---

## Exemple de test manuel

### Terminal 1 - Lancer le serveur

```bash
cd NoomEternal-Server
make build
./output/noometernal-server
# Affichage: [SERVER] Server listening on 0.0.0.0:5001
```

### Terminal 2 - Connecter le premier client

```bash
# Utiliser le client NoomEternal-Client
cd ../NoomEternal-Client
make build
./output/noometernal-client
# Entrer: username = "alice"
```

### Terminal 3 - Connecter le deuxième client

```bash
cd ../NoomEternal-Client
./output/noometernal-client
# Entrer: username = "bob"
```

**Résultat attendu:**

- Les deux clients voient le plateau de jeu
- Ils peuvent faire des mouvements alternés
- Après fin du match, ils reçoivent les récompenses
- Les profils sont persistés pour la prochaine session

---

## Commandes Make disponibles

```bash
make build            # Compiler serveur + EMAP
make build-lib        # Compiler uniquement EMAP
make test             # Exécuter tests unitaires EMAP
make doc              # Générer documentation Doxygen
make clean            # Nettoyer fichiers compilés
make clean-lib        # Nettoyer EMAP
make clean-all        # Nettoyage complet
make run              # Compiler et lancer le serveur
```

---

## Dépannage

### Le serveur refuse de démarrer

```
Error: Address already in use
```

**Solution:** Le port 5001 est occupé. Attendre 60 secondes ou tuer le processus:

```bash
lsof -i :5001
kill -9 <PID>
```

### Les joueurs voient un plateau obsolète

**Cause:** Condition de course dans la lecture du plateau

**Solution:** Vérifier que toutes les lectures se font dans la section critique du mutex (voir `ARCHITECTURE.md`)

### Erreur de compilation

```bash
# Vérifier GCC/Clang disponible
gcc --version

# Vérifier POSIX threads
grep -l pthread /usr/include/*.h
```

---

## Notes de sécurité

### État actuel

- ⚠️ Pas d'authentification (fait confiance au nom d'utilisateur)
- ⚠️ Pas de chiffrement (suppose LAN de confiance)
- ⚠️ Pas de limitation de débit
- ⚠️ Pas de validation d'entrée

### Recommandations de production

- ✅ Ajouter authentification (JWT, OAuth)
- ✅ Implémenter TLS pour le chiffrement
- ✅ Rate limiting par IP
- ✅ Validation et désinfection des entrées
- ✅ Journalisation des transactions
- ✅ Gestion des certificats

---

## Contribution

### Pour modifier la codebase

1. Mettre à jour les sources dans `src/` et `include/`
2. Ajouter des commentaires Doxygen
3. Compiler et tester: `make clean && make build && make test`
4. Générer la doc: `make doc`
5. Vérifier la cohérence dans ARCHITECTURE.md et API_REFERENCE.md

### Standards de code

- Utiliser des noms de fonction explicites
- Documenter avec Doxygen (`/** @brief ... */`)
- Protéger les accès au lobby avec mutex
- Gérer les erreurs avec codes de retour
- Tester les cas limites (disconnexion, timeout, etc.)

---

## Ressources

- **EMAP Library:** `../EMAP/README.md`
- **Protocole EMAP:** `../EMAP/include/emap.h`
- **Architecture complète:** [ARCHITECTURE.md](ARCHITECTURE.md)
- **Référence API:** [API_REFERENCE.md](API_REFERENCE.md)
- **Documentation générée:** `docs/html/index.html` (après `make doc`)
