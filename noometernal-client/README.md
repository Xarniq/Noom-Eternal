# NoomEternal - Client

[![pipeline status](https://gitlab.com/sco-chartreux/3ics-25-26/moulin-anton/projet-c/noometernal-client/badges/main/pipeline.svg)](https://gitlab.com/sco-chartreux/3ics-25-26/moulin-anton/projet-c/noometernal-client/-/commits/main) [![Latest Release](https://gitlab.com/sco-chartreux/3ics-25-26/moulin-anton/projet-c/noometernal-client/-/badges/release.svg)](https://gitlab.com/sco-chartreux/3ics-25-26/moulin-anton/projet-c/noometernal-client/-/releases)

## Vue d'ensemble

**NoomEternal Client** est un client de jeu Doom Eternal-inspiré développé en C avec le framework raylib. L'application combine une interface utilisateur immersive, une gestion locale des ressources et une connectivité en ligne pour permettre des matchs multijoueurs.

### Concepts clés

- **Architecture orientée écrans** : L'application utilise un système de gestion d'écrans (intro, accueil, magasin, chargement, jeu, victoire, défaite).
- **Gestion asynchrone du réseau** : Connexion au serveur EMAP avec synchronisation d'état non-bloquante.
- **Gestion des ressources** : Chargement/déchargement dynamique de textures, polices et fichiers audio.
- **Configuration persistante** : Sauvegarde des préférences locales (volume, adresse serveur) dans un fichier INI.

## Architecture générale

```
┌─────────────────────────────────────────────────┐
│           Application NoomEternal              │
├─────────────────────────────────────────────────┤
│  Main Loop                                      │
│  ├─ Network Processing (non-blocking)          │
│  ├─ Game Logic Updates                         │
│  ├─ UI Rendering & Input Handling              │
│  └─ Audio Management                           │
├─────────────────────────────────────────────────┤
│  Game State Management                         │
│  ├─ PlayerData (progression, ressources)       │
│  ├─ GameState (plateau, mouvements)            │
│  └─ GameSettings (préférences, paramètres)     │
├─────────────────────────────────────────────────┤
│  Resources & External Systems                  │
│  ├─ Assets (textures, polices, audio)          │
│  ├─ Network Client (EMAP protocol)             │
│  └─ Configuration File (INI)                   │
└─────────────────────────────────────────────────┘
```

## Fonctionnement

### 1. Initialisation

- Chargement de la configuration depuis `config/client.ini`
- Initialisation des assets (textures, musiques, polices)
- Création du contexte de client réseau
- Affichage de l'écran d'intro

### 2. Boucle principale

- **Traitement réseau** : Vérification des messages entrants sans bloquer le jeu
- **Mises à jour** : Logique du jeu, transitions d'écrans, gestion des entrées
- **Rendu** : Affichage de l'écran courant, UI et éléments de jeu
- **Audio** : Mise à jour de la musique de fond avec fondu adéquat

### 3. Gestion des écrans

| Écran | Rôle |
|-------|------|
| **INTRO** | Lecture d'une vidéo d'introduction |
| **HOME** | Menu principal avec options de jeu |
| **SHOP** | Interface d'achat de monnaies/skins |
| **LOADING** | Écran d'attente avant le matchmaking |
| **GAME** | Gameplay principal avec plateau interactif |
| **VICTORY** | Écran de victoire avec progression |
| **GAMEOVER** | Écran de défaite avec statistiques |

### 4. Interactions réseau

Le client établit une connexion EMAP au serveur pour :

- Récupérer les données de profil du joueur
- Envoyer/recevoir des mouvements de jeu
- Traiter les actions du magasin
- Synchroniser l'état de partie

## Structure des fichiers

```
NoomEternal-Client/
├── src/                          # Implémentations
│   ├── main.c                    # Boucle principale
│   ├── config.c                  # Gestion de configuration
│   ├── game_logic.c              # Logique du jeu
│   ├── network_client.c          # Communication EMAP
│   ├── audio_manager.c           # Gestion audio
│   ├── ui.c                      # Chargement des ressources
│   ├── settings_ui.c             # Panneau de paramètres
│   └── ui_screens/               # Implémentations des écrans
├── include/                      # En-têtes publiques
│   ├── types.h                   # Définitions de structures
│   ├── config.h, game_logic.h    # Interface publique
│   ├── settings_ui.h, network_client.h
│   ├── ui.h                      # Gestion des ressources
│   └── ui/                       # En-têtes des écrans
├── assets/                       # Ressources (images, audio, vidéo)
├── config/                       # Fichiers de configuration
└── lib/                          # Dépendances externes
    ├── emap-lib/                 # Protocole de communication
    ├── raylib/                   # Moteur graphique
    └── raylib-media/             # Lectures vidéo/audio avancées
```

## Points d'extension

- **Nouveaux écrans** : Ajouter un en-tête dans `include/ui/` et implémenter la fonction `DrawXxxScreen`
- **Logique de jeu** : Étendre `game_logic.c` avec nouvelles règles
- **Configuration** : Modifier `config.c` pour ajouter de nouveaux paramètres INI
- **Réseau** : Utiliser les fonctions de `network_client.c` pour des commandes supplémentaires

## Dépendances

- **raylib** : Framework graphique pour le rendu et l'entrée utilisateur
- **raylib-media** : Support de la vidéo et audio en streaming
- **ffmpeg**: Librairies videos pour raylib-media
- **EMAP** : Protocole de communication réseau structuré
- **librairies UI (debian)**: `libxcursor-dev`, `libxtst-dev`, `libxinerama-dev`, `libgl1-mesa-dev`.

## Compilation et exécution

```bash
cd NoomEternal-Client
make                    # Compilation
./build/noometernal-client   # Lancement
```

## Documentation détaillée

Pour une documentation technique complète des fonctions, classes et structures, consultez la **documentation Doxygen** intégrée accessible après génération.
