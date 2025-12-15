# GWPathfinder - DLL de Pathfinding pour Guild Wars

DLL de pathfinding optimisée pour Guild Wars, utilisable depuis AutoIt avec chargement lazy des maps depuis archive ZIP.

## 🚀 Démarrage rapide

```bash
# 1. Convertir maps.rar en maps.zip
.\ConvertRarToZip.ps1

# 2. Compiler
.\build.bat

# 3. Tester
cd build\Release
AutoIt3.exe ..\..\TestAutoIt.au3
```

Voir [QUICKSTART.md](QUICKSTART.md) pour plus de détails.

## 📋 Caractéristiques

- ✅ **Chargement lazy** : Les maps sont chargées uniquement quand nécessaires
- ✅ **Cache LRU** : Garde les 20 maps les plus utilisées en mémoire
- ✅ **DLL ultra-légère** : ~5 MB au lieu de ~500 MB
- ✅ **API simple** : Compatible avec AutoIt, C, C++
- ✅ **Pathfinding A*** : Algorithme optimisé avec heuristiques
- ✅ **Simplification de chemin** : Réduction automatique des points intermédiaires
- ✅ **Support téléporteurs** : Gère les téléportations dans les maps
- ✅ **Thread-safe** : Utilisation depuis plusieurs threads

## 📦 Prérequis

- Windows 10/11
- CMake 3.16+
- Visual Studio 2022 (ou 2019)
- vcpkg
- 7-Zip ou WinRAR (pour convertir maps.rar)

## 🔧 Installation

### 1. Installer vcpkg

```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### 2. Cloner le projet

Le projet fait partie de GWToolboxpp.

### 3. Préparer maps.zip

```powershell
# Automatique (recommandé)
.\ConvertRarToZip.ps1

# OU manuel
# 1. Extraire maps.rar
# 2. Créer maps.zip avec tous les map_*.json
```

### 4. Compiler

```bash
.\build.bat
```

La DLL sera dans `build/Release/GWPathfinder.dll`

## 📖 Utilisation

### AutoIt

```autoit
#include <Array.au3>

; Chemins
Global Const $DLL_PATH = @ScriptDir & "\GWPathfinder.dll"

; Structures
Global Const $tagPathPoint = "float x;float y"
Global Const $tagPathResult = "ptr points;int point_count;float total_cost;int error_code;char error_message[256]"

; Initialiser
DllCall($DLL_PATH, "int:cdecl", "Initialize")

; Trouver un chemin
Local $pPath = DllCall($DLL_PATH, "ptr:cdecl", "FindPath", _
    "int", 7, _          ; Ascalon City
    "float", 100.0, _    ; Start X
    "float", 200.0, _    ; Start Y
    "float", 500.0, _    ; Dest X
    "float", 600.0, _    ; Dest Y
    "float", 50.0)       ; Simplify range

If @error = 0 And $pPath[0] <> 0 Then
    ; Traiter le résultat
    Local $result = DllStructCreate($tagPathResult, $pPath[0])
    Local $pointCount = DllStructGetData($result, "point_count")
    Local $pPoints = DllStructGetData($result, "points")

    ; Lire les points
    For $i = 0 To $pointCount - 1
        Local $point = DllStructCreate($tagPathPoint, $pPoints + $i * 8)
        Local $x = DllStructGetData($point, "x")
        Local $y = DllStructGetData($point, "y")
        ConsoleWrite("Point " & $i & ": (" & $x & ", " & $y & ")" & @CRLF)
    Next

    ; Libérer la mémoire
    DllCall($DLL_PATH, "none:cdecl", "FreePathResult", "ptr", $pPath[0])
EndIf

; Fermer
DllCall($DLL_PATH, "none:cdecl", "Shutdown")
```

Voir [TestAutoIt.au3](TestAutoIt.au3) pour un exemple complet.

### C/C++

```cpp
#include "PathfinderAPI.h"

// Initialiser
Initialize();

// Trouver un chemin
PathResult* result = FindPath(7, 100.0f, 200.0f, 500.0f, 600.0f, 50.0f);

if (result && result->error_code == 0) {
    // Parcourir les points
    for (int i = 0; i < result->point_count; i++) {
        printf("Point %d: (%.2f, %.2f)\n",
               i, result->points[i].x, result->points[i].y);
    }
}

// Libérer
FreePathResult(result);
Shutdown();
```

## 📚 API

### Fonctions principales

| Fonction | Description |
|----------|-------------|
| `Initialize()` | Initialise la DLL et charge maps.zip |
| `FindPath(mapId, startX, startY, destX, destY, range)` | Trouve un chemin |
| `FreePathResult(result)` | Libère la mémoire d'un résultat |
| `IsMapAvailable(mapId)` | Vérifie si une map existe |
| `GetAvailableMaps(count)` | Liste toutes les maps |
| `GetMapStats(mapId)` | Obtient les statistiques d'une map |
| `Shutdown()` | Ferme la DLL |

Voir [PathfinderAPI.h](PathfinderAPI.h) pour la documentation complète.

## 🏗️ Architecture

```
┌─────────────────────────────────────┐
│      AutoIt Script (.au3)           │
└──────────────┬──────────────────────┘
               │ DllCall
               ▼
┌─────────────────────────────────────┐
│   PathfinderAPI.cpp (API C)         │
│   - Initialize()                     │
│   - FindPath()                       │
│   - FreePathResult()                 │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   PathfinderCore.cpp                │
│   - Algorithme A*                   │
│   - Simplification de chemin        │
│   - Gestion téléporteurs            │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   MapDataRegistry.cpp               │
│   - Interface de chargement         │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   MapArchiveLoader.cpp              │
│   - Lecture depuis maps.zip         │
│   - Cache LRU (20 maps)             │
│   - Thread-safe                     │
└──────────────┬──────────────────────┘
               │
               ▼
         ┌────────────┐
         │ maps.zip   │
         │ (400+ maps)│
         └────────────┘
```

## 📁 Structure du projet

```
Pathfinder/
├── 📄 README.md                    ← Ce fichier
├── 📄 QUICKSTART.md                ← Guide de démarrage rapide
├── 📄 README_ARCHIVE_LOADING.md   ← Documentation technique
├── 📄 CHANGELOG.md                 ← Historique des changements
│
├── 🔧 build.bat                    ← Script de compilation
├── 🔧 ConvertRarToZip.ps1          ← Conversion RAR→ZIP
├── 🔧 CMakeLists.txt               ← Configuration CMake
│
├── 💻 PathfinderAPI.cpp/.h         ← API C exportée
├── 💻 PathfinderCore.cpp/.h        ← Moteur de pathfinding
├── 💻 MapDataRegistry.cpp/.h       ← Registre des maps
├── 💻 MapArchiveLoader.cpp/.h      ← Chargement depuis ZIP
│
├── 📝 TestAutoIt.au3               ← Script de test AutoIt
│
├── 📦 maps.rar                     ← Archive source (à convertir)
└── 📦 maps.zip                     ← Archive utilisée (généré)
```

## 🔍 Fonctionnement

### Chargement lazy

1. Au démarrage : La DLL initialise juste le système de chargement (~0.1 sec)
2. Premier `FindPath(mapId)` : La map est chargée depuis ZIP (~20 ms)
3. Appels suivants : La map est déjà en cache (<1 ms)
4. Cache plein : La map la moins utilisée est supprimée

### Cache LRU

- Capacité : 20 maps par défaut
- Stratégie : Least Recently Used
- Thread-safe : Mutex pour accès concurrent
- Modifiable : Voir `MapArchiveLoader.cpp:72`

### Format des données

Les fichiers dans `maps.zip` sont au format JSON :
```json
{
  "map_id": 7,
  "points": [...],
  "trapezoids": [...],
  "teleporters": [...],
  "travel_portals": [...]
}
```

## 🎯 Performance

| Opération | Temps | Notes |
|-----------|-------|-------|
| Initialize() | ~100 ms | Scan de maps.zip |
| Premier FindPath() | ~20-50 ms | Chargement + pathfinding |
| FindPath() suivants | <1 ms | Depuis cache |
| Mémoire par map | ~1-5 MB | Dépend de la taille |
| Cache total | ~20-100 MB | 20 maps max |

## 🛠️ Développement

### Ajouter une nouvelle map

1. Exporter la map en JSON depuis GWToolbox
2. Nommer `map_XXX.json` (XXX = map ID)
3. Ajouter dans `maps.zip`
4. Pas besoin de recompiler!

### Modifier la taille du cache

Dans `MapArchiveLoader.cpp` :
```cpp
// Ligne 72
m_cache(std::make_unique<MapCache>(20))  // <- Changer 20
```

### Mode debug

```bash
.\build.bat debug
```

### Nettoyer et recompiler

```bash
.\build.bat clean release
```

## 📝 License

Ce projet fait partie de GWToolbox++.

## 🤝 Contribution

1. Forker le projet
2. Créer une branche (`git checkout -b feature/AmazingFeature`)
3. Commit (`git commit -m 'Add some AmazingFeature'`)
4. Push (`git push origin feature/AmazingFeature`)
5. Ouvrir une Pull Request

## ⚠️ Notes importantes

- **maps.zip obligatoire** : La DLL ne fonctionnera pas sans
- **Noms de fichiers** : Doivent être exactement `map_123.json`
- **Déploiement** : Toujours distribuer DLL + maps.zip ensemble
- **Thread-safety** : Peut être utilisé depuis plusieurs threads
- **Pas de fuite mémoire** : Toujours appeler `FreePathResult()`

## 📞 Support

- Documentation : [README_ARCHIVE_LOADING.md](README_ARCHIVE_LOADING.md)
- Démarrage rapide : [QUICKSTART.md](QUICKSTART.md)
- Exemples : [TestAutoIt.au3](TestAutoIt.au3)
- Issues : GitHub Issues

## 🎉 Changelog

Voir [CHANGELOG.md](CHANGELOG.md)

---

**Note** : Cette version utilise un système de chargement depuis archive ZIP pour réduire drastiquement la taille de la DLL et améliorer les performances de démarrage.
