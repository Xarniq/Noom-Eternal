# NoomEternal — How to Play

NoomEternal is a **two-player online coin-sliding board game** inspired by the Doom Eternal universe. Players take turns moving coins leftward on a 12-cell board. The last player able to make a legal move wins.

---

## Table of Contents

- [Game Rules](#game-rules)
- [Prerequisites](#prerequisites)
- [Building the Project](#building-the-project)
  - [EMAP Library](#1-emap-library)
  - [Server](#2-server)
  - [Client](#3-client)
- [Server Configuration](#server-configuration)
- [Client Configuration](#client-configuration)
- [Launching a Game](#launching-a-game)
- [In-Game Controls](#in-game-controls)
- [Docker](#docker)
- [Authors](#authors)

---

## Game Rules

- The board has **12 cells** and **5 coins** placed at random positions.
- Two players take turns. On your turn, you must move **one coin to the left**.
- A coin **cannot jump over** another coin (it is blocked by the nearest coin to its left).
- A coin can only move to an **empty cell** to its left.
- The game ends when **no legal move remains**.
- The **last player to make a move wins**.

---

## Prerequisites

| Dependency | Required for |
|---|---|
| **GCC** (or Clang) — C99+ | Server & Client |
| **Make** | Server & Client |
| **CMake** | Client (raylib-media) |
| **raylib** (included as submodule) | Client |
| **FFmpeg** development libraries | Client |
| **pkg-config** | Client |
| **Git** | Cloning & submodules |

### Installing FFmpeg dev packages

- **Ubuntu/Debian:** `sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev`
- **Fedora/RHEL:** `sudo dnf install ffmpeg-devel`
- **macOS (Homebrew):** `brew install ffmpeg`

---

## Building the Project

Clone the repository and initialize all submodules:

```bash
git clone --recursive <repository-url>
cd NoomEternal
```

### 1. EMAP Library

The EMAP protocol library is a shared dependency used by both server and client. It is built automatically as part of their build processes, but you can also build it standalone:

```bash
cd emap
make build
```

### 2. Server

```bash
cd noometernal-server
make build
```

The compiled binary is located at `output/noometernal-server`.

### 3. Client

```bash
cd noometernal-client
make all
```

The compiled binary is located at `build/noometernal-client`.

> **Note:** The client build will automatically compile raylib, raylib-media, and the EMAP library from the `lib/` submodules.

---

## Server Configuration

The server reads its settings from `noometernal-server/config/server.ini`:

```ini
[server]
; IP address to bind to
; Use 0.0.0.0 to listen on all interfaces
; Use 127.0.0.1 for localhost only
ip = 0.0.0.0

; Port number to listen on (1-65535)
port = 5002

; Maximum pending connections in queue (backlog)
backlog = 2
```

| Key | Description | Default |
|---|---|---|
| `ip` | Network interface to bind. Use `0.0.0.0` for all interfaces, `127.0.0.1` for local only. | `0.0.0.0` |
| `port` | TCP port the server listens on. | `5002` |
| `backlog` | Maximum number of pending connections in the accept queue. | `2` |

---

## Client Configuration

The client reads its settings from `noometernal-client/config/client.ini`:

```ini
[Audio]
MusicVolume=0.03
SFXVolume=0.05
MusicEnabled=false

[Network]
ServerIP=noometernal.ics.quest
ServerPort=5001

[Display]
ShowFPS=false
```

| Section | Key | Description | Default |
|---|---|---|---|
| **Audio** | `MusicVolume` | Background music volume (`0.0` to `1.0`). | `0.03` |
| **Audio** | `SFXVolume` | Sound effects volume (`0.0` to `1.0`). | `0.05` |
| **Audio** | `MusicEnabled` | Enable/disable background music (`true`/`false`). | `false` |
| **Network** | `ServerIP` | IP address or hostname of the game server. | `noometernal.ics.quest` |
| **Network** | `ServerPort` | TCP port of the game server. | `5001` |
| **Display** | `ShowFPS` | Show FPS counter on screen (`true`/`false`). | `false` |

> **Tip:** To play locally, set `ServerIP=127.0.0.1` and make sure `ServerPort` matches the server's `port` value.

---

## Launching a Game

### Step 1 — Start the server

```bash
cd noometernal-server
make run
```

Or run the binary directly:

```bash
./output/noometernal-server
```

The server will start listening for connections on the configured IP and port.

### Step 2 — Launch two clients

Since NoomEternal is a **two-player** game, you need **two client instances** to start a match. Open two terminals:

**Terminal 1:**
```bash
cd noometernal-client
make run
```

**Terminal 2:**
```bash
cd noometernal-client
make run
```

### Step 3 — Connect and play

1. Both clients open to the **Home Screen**.
2. Click the **Play** button on each client to request an online match.
3. Each client enters the **Loading Screen** while waiting for an opponent.
4. Once both players are connected, the server pairs them and the **Game Screen** appears.
5. Players take turns selecting a coin and clicking a destination cell to the left.
6. The game ends when no moves remain — the winner sees the **Victory Screen** and the loser sees the **Game Over Screen**.
7. Rewards (XP and Souls currency) are awarded to the winner.

### Playing over a network

To play across different machines:

1. Start the server on a machine accessible over the network.
2. On each client machine, edit `config/client.ini` and set `ServerIP` to the server's IP address, and `ServerPort` to the server's port.
3. Launch the client on each machine and click **Play**.

---

## In-Game Controls

| Action | Control |
|---|---|
| Select a coin | Click on one of your coins on the board |
| Move a coin | Click on a valid empty cell to the left of the selected coin |
| Open settings | Click the settings button |
| Cancel matchmaking | Click the cancel button on the loading screen |

---

## Docker

Both the server and client include a `Dockerfile` for containerized builds.

### Server

```bash
cd noometernal-server
docker build -t noometernal-server .
docker run -p 5002:5002 noometernal-server make run
```

### Client

```bash
cd noometernal-client
docker build -t noometernal-client .
```

> **Note:** Running the client in Docker requires X11 forwarding or a display server, since it is a graphical application.

---

## Authors

- Mattéo BEZET-TORRES
- Elias GARACH-MALULY
- Anton MOULIN
