# Guide de démarrage rapide - GWPathfinder

## Installation rapide (3 étapes)

### 1. Préparer maps.zip

Convertissez votre fichier `maps.rar` en `maps.zip` :

```bash
# Option A: Automatique (recommandé)
.\ConvertRarToZip.ps1

# Option B: Manuel
# Extraire maps.rar avec WinRAR/7-Zip
# Créer maps.zip avec tous les fichiers map_*.json
```

### 2. Compiler la DLL

```bash
# Compilation simple (Release)
.\build.bat

# Compilation avec nettoyage
.\build.bat clean release

# Mode debug
.\build.bat debug

# Aide
.\build.bat help
```

Le script `build.bat` va automatiquement :
- ✅ Vérifier que maps.zip existe
- ✅ Installer les dépendances vcpkg (nlohmann-json, libzip)
- ✅ Configurer CMake
- ✅ Compiler la DLL
- ✅ Copier maps.zip dans le dossier de sortie

### 3. Tester

```bash
# La DLL sera dans build/Release/
cd build\Release
AutoIt3.exe ..\..\TestAutoIt.au3
```

## Utilisation avec AutoIt

### Code minimal

```autoit
; Charger la DLL
Global Const $DLL_PATH = @ScriptDir & "\GWPathfinder.dll"

; Initialiser
DllCall($DLL_PATH, "int:cdecl", "Initialize")

; Trouver un chemin
Local $pPath = DllCall($DLL_PATH, "ptr:cdecl", "FindPath", _
    "int", 123, _        ; Map ID
    "float", 100.0, _    ; Start X
    "float", 200.0, _    ; Start Y
    "float", 500.0, _    ; Dest X
    "float", 600.0, _    ; Dest Y
    "float", 50.0)       ; Simplify range

; Libérer la mémoire
DllCall($DLL_PATH, "none:cdecl", "FreePathResult", "ptr", $pPath[0])

; Fermer
DllCall($DLL_PATH, "none:cdecl", "Shutdown")
```

### Exemple complet

Voir [TestAutoIt.au3](TestAutoIt.au3) pour un exemple complet avec :
- Gestion des erreurs
- Affichage des statistiques de maps
- Liste des maps disponibles
- Parsing du résultat de pathfinding

## Structure des fichiers

Après compilation, vous aurez :

```
Pathfinder/
├── build/
│   └── Release/
│       ├── GWPathfinder.dll    ← La DLL à utiliser
│       └── maps.zip             ← Automatiquement copié
├── MapArchiveLoader.cpp/.h      ← Chargement depuis ZIP
├── PathfinderAPI.cpp/.h         ← API C pour AutoIt
├── PathfinderCore.cpp/.h        ← Moteur A*
├── MapDataRegistry.cpp/.h       ← Interface de chargement
├── build.bat                    ← Script de compilation
├── ConvertRarToZip.ps1          ← Conversion RAR→ZIP
├── TestAutoIt.au3               ← Script de test
└── CMakeLists.txt               ← Configuration CMake
```

## Déploiement

Pour distribuer votre application :

```
VotreApplication/
├── VotreScript.au3
├── GWPathfinder.dll
└── maps.zip
```

C'est tout! La DLL trouvera automatiquement `maps.zip` dans son propre dossier.

## Résolution de problèmes

### "CMake not found"
Installez CMake : https://cmake.org/download/

### "vcpkg not found"
Installez vcpkg : https://github.com/microsoft/vcpkg
```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### "Failed to initialize pathfinder"
- Vérifiez que `maps.zip` est présent
- Vérifiez que le fichier n'est pas corrompu
- Essayez de recréer maps.zip avec `ConvertRarToZip.ps1`

### "Map XXX not found in archive"
- Ouvrez maps.zip et vérifiez que `map_XXX.json` existe
- Le nom doit être exactement `map_123.json` (pas `Map_123.json` ou `123.json`)

### Compilation échoue
```bash
# Nettoyer et recompiler
.\build.bat clean release

# Vérifier les dépendances
cd ..\..\..\..\..
vcpkg install nlohmann-json:x64-windows
vcpkg install libzip:x64-windows
```

## Performance

### Première utilisation d'une map
~10-50ms (lecture ZIP + parsing JSON)

### Utilisations suivantes
<1ms (depuis le cache)

### Cache
20 maps en mémoire par défaut (modifiable dans MapArchiveLoader.cpp)

## Différences avec l'ancienne version

| Aspect | Ancienne | Nouvelle (ZIP) |
|--------|----------|----------------|
| Taille DLL | ~500 MB | ~5 MB |
| Mémoire | Toutes les maps | 20 maps max |
| Démarrage | ~5 sec | ~0.1 sec |
| Premier accès map | Immédiat | ~20 ms |
| Mise à jour maps | Recompiler | Remplacer ZIP |
| API AutoIt | Inchangée | Inchangée ✅ |

## Documentation complète

- [README_ARCHIVE_LOADING.md](README_ARCHIVE_LOADING.md) - Documentation technique
- [CHANGELOG.md](CHANGELOG.md) - Liste des modifications
- [TestAutoIt.au3](TestAutoIt.au3) - Exemples de code

## Support

En cas de problème :
1. Vérifiez la [section dépannage](#résolution-de-problèmes)
2. Consultez [README_ARCHIVE_LOADING.md](README_ARCHIVE_LOADING.md)
3. Vérifiez que vcpkg est bien configuré
4. Essayez `build.bat clean release`

Bon développement! 🚀
