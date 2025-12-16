# GWPathfinder - Pathfinding DLL for Guild Wars

Optimized pathfinding DLL for Guild Wars, usable from AutoIt with lazy loading of maps from ZIP archive.

## Quick Start

```bash
# 1. Convert maps.rar to maps.zip
.\ConvertRarToZip.ps1

# 2. Build
.\build.bat

# 3. Test
cd build\Release
AutoIt3.exe ..\..\TestAutoIt.au3
```

See [QUICKSTART.md](QUICKSTART.md) for more details.

## Features

- **Lazy loading**: Maps are loaded only when needed
- **LRU Cache**: Keeps the 20 most used maps in memory
- **Lightweight DLL**: ~5 MB instead of ~500 MB
- **Simple API**: Compatible with AutoIt, C, C++
- **A* Pathfinding**: Optimized algorithm with heuristics
- **Path simplification**: Automatic reduction of intermediate points
- **Teleporter support**: Handles teleportations within maps
- **Thread-safe**: Can be used from multiple threads

## Requirements

- Windows 10/11
- CMake 3.16+
- Visual Studio 2022 (or 2019)
- vcpkg
- 7-Zip or WinRAR (to convert maps.rar)

## Installation

### 1. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### 2. Prepare maps.zip

```powershell
# Automatic (recommended)
.\ConvertRarToZip.ps1

# OR manual
# 1. Extract maps.rar
# 2. Create maps.zip with all JSON files (format: {mapId}_*.json)
```

### 3. Build

```bash
.\build.bat
```

The DLL will be in `build/Release/GWPathfinder.dll`

## Usage

### AutoIt

```autoit
#include <Array.au3>

; Paths
Global Const $DLL_PATH = @ScriptDir & "\GWPathfinder.dll"

; Structures
Global Const $tagPathPoint = "float x;float y"
Global Const $tagPathResult = "ptr points;int point_count;float total_cost;int error_code;char error_message[256]"
Global Const $tagMapStats = "int trapezoid_count;int point_count;int teleport_count;int travel_portal_count;int npc_travel_count;int enter_travel_count;int error_code;char error_message[256]"

; Initialize
DllCall($DLL_PATH, "int:cdecl", "Initialize")

; Find a path
Local $pPath = DllCall($DLL_PATH, "ptr:cdecl", "FindPath", _
    "int", 7, _          ; Ascalon City
    "float", 100.0, _    ; Start X
    "float", 200.0, _    ; Start Y
    "float", 500.0, _    ; Dest X
    "float", 600.0, _    ; Dest Y
    "float", 50.0)       ; Simplify range

If @error = 0 And $pPath[0] <> 0 Then
    ; Process result
    Local $result = DllStructCreate($tagPathResult, $pPath[0])
    Local $pointCount = DllStructGetData($result, "point_count")
    Local $pPoints = DllStructGetData($result, "points")

    ; Read points
    For $i = 0 To $pointCount - 1
        Local $point = DllStructCreate($tagPathPoint, $pPoints + $i * 8)
        Local $x = DllStructGetData($point, "x")
        Local $y = DllStructGetData($point, "y")
        ConsoleWrite("Point " & $i & ": (" & $x & ", " & $y & ")" & @CRLF)
    Next

    ; Free memory
    DllCall($DLL_PATH, "none:cdecl", "FreePathResult", "ptr", $pPath[0])
EndIf

; Shutdown
DllCall($DLL_PATH, "none:cdecl", "Shutdown")
```

See [TestAutoIt.au3](TestAutoIt.au3) for a complete example.

### C/C++

```cpp
#include "PathfinderAPI.h"

// Initialize
Initialize();

// Find a path
PathResult* result = FindPath(7, 100.0f, 200.0f, 500.0f, 600.0f, 50.0f);

if (result && result->error_code == 0) {
    // Iterate through points
    for (int i = 0; i < result->point_count; i++) {
        printf("Point %d: (%.2f, %.2f)\n",
               i, result->points[i].x, result->points[i].y);
    }
}

// Free memory
FreePathResult(result);
Shutdown();
```

## API Reference

### Core Functions
```
| Function                 | Description                                                       |
|--------------------------|-------------------------------------------------------------------|
| `Initialize()`           | Initializes the DLL and loads maps.zip. Returns 1 on success, 0 on failure. |
| `Shutdown()`             | Cleans up DLL resources.                                          |
| `GetPathfinderVersion()` | Returns the DLL version string.                                   |
```
### Pathfinding Functions
```
| Function                                               | Description                                             |
|--------------------------------------------------------|---------------------------------------------------------|
| `FindPath(mapId, startX, startY, destX, destY, range)` | Finds a path between two points. Returns a `PathResult*`. |
| `FreePathResult(result)`                               | Frees the memory allocated for a `PathResult`.          |
```
### Map Functions
```
| Function                           | Description                                                                    |
|------------------------------------|--------------------------------------------------------------------------------|
| `IsMapAvailable(mapId)`            | Checks if a map exists in the archive. Returns 1 if available, 0 otherwise.    |
| `GetAvailableMaps(count)`          | Returns an array of all available map IDs. Must be freed with `FreeMapList()`. |
| `FreeMapList(mapList)`             | Frees the memory allocated by `GetAvailableMaps()`.                            |
| `GetMapStats(mapId)`               | Gets statistics for a map. Returns a `MapStats*`. Must be freed with `FreeMapStats()`. |
| `FreeMapStats(stats)`              | Frees the memory allocated for `MapStats`.                                     |
| `LoadMapFromFile(mapId, filePath)` | Loads a map from an external JSON file. Returns 1 on success, 0 on failure.    |
```
See [PathfinderAPI.h](PathfinderAPI.h) for complete documentation.

## Architecture

```
+-------------------------------------+
|      AutoIt Script (.au3)           |
+--------------+----------------------+
               | DllCall
               v
+-------------------------------------+
|   PathfinderAPI.cpp (C API)         |
|   - Initialize()                    |
|   - FindPath()                      |
|   - FreePathResult()                |
+--------------+----------------------+
               |
               v
+-------------------------------------+
|   PathfinderCore.cpp                |
|   - A* Algorithm                    |
|   - Path simplification             |
|   - Teleporter handling             |
+--------------+----------------------+
               |
               v
+-------------------------------------+
|   MapDataRegistry.cpp               |
|   - Loading interface               |
+--------------+----------------------+
               |
               v
+-------------------------------------+
|   MapArchiveLoader.cpp              |
|   - Reading from maps.zip           |
|   - LRU Cache (20 maps)             |
|   - Thread-safe                     |
+--------------+----------------------+
               |
               v
         +------------+
         | maps.zip   |
         | (400+ maps)|
         +------------+
```

## Project Structure

```
Pathfinder/
├── README.md                    <- This file
├── QUICKSTART.md                <- Quick start guide
├── README_ARCHIVE_LOADING.md    <- Technical documentation
├── CHANGELOG.md                 <- Change history
│
├── build.bat                    <- Build script
├── ConvertRarToZip.ps1          <- RAR to ZIP conversion
├── CMakeLists.txt               <- CMake configuration
│
├── PathfinderAPI.cpp/.h         <- Exported C API
├── PathfinderCore.cpp/.h        <- Pathfinding engine
├── MapDataRegistry.cpp/.h       <- Map registry
├── MapArchiveLoader.cpp/.h      <- ZIP archive loader
│
├── TestAutoIt.au3               <- AutoIt test script
│
├── maps.rar                     <- Source archive (to convert)
└── maps.zip                     <- Used archive (generated)
```

## How It Works

### Lazy Loading

1. At startup: DLL only initializes the loading system (~0.1 sec)
2. First `FindPath(mapId)`: Map is loaded from ZIP (~20 ms)
3. Subsequent calls: Map is already cached (<1 ms)
4. Cache full: Least recently used map is evicted

### LRU Cache

- Capacity: 20 maps by default
- Strategy: Least Recently Used
- Thread-safe: Mutex for concurrent access
- Configurable: See `MapArchiveLoader.cpp:69`

### Map File Naming Convention

Files in `maps.zip` must follow this naming format:
```
{mapId}_{description}.json
```

Examples:
- `7_Prophecies_Ascalon_AscalonCity.json`
- `100_Prophecies_Kryta_LionsArch.json`
- `248_Factions_Cantha_Kaineng.json`

The loader extracts the map ID from the beginning of the filename (before the first underscore).

### JSON Data Format

Map files contain the following structure:
```json
{
  "map_id": 7,
  "points": [...],
  "trapezoids": [...],
  "teleporters": [...],
  "travel_portals": [],
  "npc_travels": [],
  "enter_travels": []
}
```

## Performance
```
| Operation            | Time       | Notes                 |
|----------------------|------------|-----------------------|
| Initialize()         | ~100 ms    | Scans maps.zip        |
| First FindPath()     | ~20-50 ms  | Loading + pathfinding |
| Subsequent FindPath()| <1 ms      | From cache            |
| Memory per map       | ~1-5 MB    | Depends on size       |
| Total cache          | ~20-100 MB | 20 maps max           |
```
## Development

### Adding a New Map

1. Export the map to JSON
2. Name it `{mapId}_{description}.json` (e.g., `123_MyMap.json`)
3. Add it to `maps.zip`
4. No need to recompile!

### Changing Cache Size

In `MapArchiveLoader.cpp`:
```cpp
// Line 69
m_cache(std::make_unique<MapCache>(20))  // <- Change 20
```

### Debug Mode

```bash
.\build.bat debug
```

### Clean and Rebuild

```bash
.\build.bat clean release
```

## Contributing

1. Fork the project
2. Create a branch (`git checkout -b feature/AmazingFeature`)
3. Commit (`git commit -m 'Add some AmazingFeature'`)
4. Push (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## Important Notes

- **maps.zip required**: The DLL will not work without it
- **File naming**: Must follow `{mapId}_*.json` format (e.g., `7_Ascalon.json`)
- **Deployment**: Always distribute DLL + maps.zip together
- **Thread-safety**: Can be used from multiple threads
- **Memory management**: Always call `FreePathResult()`, `FreeMapList()`, and `FreeMapStats()`

## Acknowledgements

Special thanks to **[QuarkyUp](https://github.com/QuarkyUp)** for the inspiration and help with this project.

## Support

- Documentation: [README_ARCHIVE_LOADING.md](README_ARCHIVE_LOADING.md)
- Quick start: [QUICKSTART.md](QUICKSTART.md)
- Examples: [TestAutoIt.au3](TestAutoIt.au3)
- Issues: GitHub Issues

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

---

**Note**: This version uses a ZIP archive loading system to drastically reduce DLL size and improve startup performance.
