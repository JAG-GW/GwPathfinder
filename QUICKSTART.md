# Quick Start Guide - GWPathfinder

## Quick Installation (3 steps)

### 1. Prepare maps.zip

Convert your `maps.rar` file to `maps.zip`:

```bash
# Option A: Automatic (recommended)
.\ConvertRarToZip.ps1

# Option B: Manual
# Extract maps.rar with WinRAR/7-Zip
# Create maps.zip with all JSON files (format: {mapId}_*.json)
```

### 2. Build the DLL

```bash
# Simple build (Release)
.\build.bat

# Clean build
.\build.bat clean release

# Debug mode
.\build.bat debug

# Help
.\build.bat help
```

The `build.bat` script will automatically:
- Check that maps.zip exists
- Install vcpkg dependencies (nlohmann-json, libzip)
- Configure CMake
- Build the DLL
- Copy maps.zip to the output folder

### 3. Test

```bash
# The DLL will be in build/Release/
cd build\Release
AutoIt3.exe ..\..\TestAutoIt.au3
```

## Using with AutoIt

### Minimal Code

```autoit
; Load the DLL
Global Const $DLL_PATH = @ScriptDir & "\GWPathfinder.dll"

; Initialize
DllCall($DLL_PATH, "int:cdecl", "Initialize")

; Find a path
Local $pPath = DllCall($DLL_PATH, "ptr:cdecl", "FindPath", _
    "int", 123, _        ; Map ID
    "float", 100.0, _    ; Start X
    "float", 200.0, _    ; Start Y
    "float", 500.0, _    ; Dest X
    "float", 600.0, _    ; Dest Y
    "float", 50.0)       ; Simplify range

; Free memory
DllCall($DLL_PATH, "none:cdecl", "FreePathResult", "ptr", $pPath[0])

; Shutdown
DllCall($DLL_PATH, "none:cdecl", "Shutdown")
```

### Complete Example

See [TestAutoIt.au3](TestAutoIt.au3) for a complete example with:
- Error handling
- Map statistics display
- Available maps listing
- Pathfinding result parsing

## File Structure

After building, you will have:

```
Pathfinder/
├── build/
│   └── Release/
│       ├── GWPathfinder.dll    <- The DLL to use
│       └── maps.zip            <- Automatically copied
├── MapArchiveLoader.cpp/.h     <- ZIP loader
├── PathfinderAPI.cpp/.h        <- C API for AutoIt
├── PathfinderCore.cpp/.h       <- A* engine
├── MapDataRegistry.cpp/.h      <- Loading interface
├── build.bat                   <- Build script
├── ConvertRarToZip.ps1         <- RAR to ZIP conversion
├── TestAutoIt.au3              <- Test script
└── CMakeLists.txt              <- CMake configuration
```

## Deployment

To distribute your application:

```
YourApplication/
├── YourScript.au3
├── GWPathfinder.dll
└── maps.zip
```

That's it! The DLL will automatically find `maps.zip` in its own folder.

## Troubleshooting

### "CMake not found"
Install CMake: https://cmake.org/download/

### "vcpkg not found"
Install vcpkg: https://github.com/microsoft/vcpkg
```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### "Failed to initialize pathfinder"
- Check that `maps.zip` is present
- Check that the file is not corrupted
- Try recreating maps.zip with `ConvertRarToZip.ps1`

### "Map XXX not found in archive"
- Open maps.zip and check that a file starting with `XXX_` exists
- The filename must follow the format `{mapId}_{description}.json` (e.g., `7_Ascalon.json`)

### Build fails
```bash
# Clean and rebuild
.\build.bat clean release

# Check dependencies
cd ..\..\..\..\..
vcpkg install nlohmann-json:x64-windows
vcpkg install libzip:x64-windows
```

## Performance

### First use of a map
~10-50ms (ZIP reading + JSON parsing)

### Subsequent uses
<1ms (from cache)

### Cache
20 maps in memory by default (configurable in MapArchiveLoader.cpp:69)

## Comparison with Previous Version
```
| Aspect           | Old       | New (ZIP)   |
|------------------|-----------|-------------|
| DLL size         | ~500 MB   | ~5 MB       |
| Memory           | All maps  | 20 maps max |
| Startup          | ~5 sec    | ~0.1 sec    |
| First map access | Instant   | ~20 ms      |
| Map updates      | Recompile | Replace ZIP |
| AutoIt API       | Same      | Same        |
```
## API Reference

### Available Functions
```
| Function                                               | Description                            |
|--------------------------------------------------------|----------------------------------------|
| `Initialize()`                                         | Initialize the DLL and load maps.zip   |
| `Shutdown()`                                           | Clean up DLL resources                 |
| `GetPathfinderVersion()`                               | Get the DLL version string             |
| `FindPath(mapId, startX, startY, destX, destY, range)` | Find a path                            |
| `FreePathResult(result)`                               | Free PathResult memory                 |
| `IsMapAvailable(mapId)`                                | Check if a map exists                  |
| `GetAvailableMaps(count)`                              | List all available maps                |
| `FreeMapList(mapList)`                                 | Free map list memory                   |
| `GetMapStats(mapId)`                                   | Get map statistics                     |
| `FreeMapStats(stats)`                                  | Free MapStats memory                   |
| `LoadMapFromFile(mapId, filePath)`                     | Load a map from external JSON file     |
```
### Map File Naming Convention

Files in `maps.zip` must follow this naming format:
```
{mapId}_{description}.json
```

Examples:
- `7_Prophecies_Ascalon_AscalonCity.json`
- `100_Prophecies_Kryta_LionsArch.json`

The loader extracts the map ID from the beginning of the filename (before the first underscore).

## Full Documentation

- [README.md](README.md) - Main documentation
- [README_ARCHIVE_LOADING.md](README_ARCHIVE_LOADING.md) - Technical documentation
- [CHANGELOG.md](CHANGELOG.md) - Change history
- [TestAutoIt.au3](TestAutoIt.au3) - Code examples

## Support

If you have issues:
1. Check the [troubleshooting section](#troubleshooting)
2. See [README_ARCHIVE_LOADING.md](README_ARCHIVE_LOADING.md)
3. Verify vcpkg is properly configured
4. Try `build.bat clean release`

Happy coding!
