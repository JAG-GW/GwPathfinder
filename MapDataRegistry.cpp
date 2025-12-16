#include "MapDataRegistry.h"
#include "MapArchiveLoader.h"

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

namespace Pathfinder {

    MapDataRegistry& MapDataRegistry::GetInstance() {
        static MapDataRegistry instance;
        return instance;
    }

    MapDataRegistry::MapDataRegistry() {
        // Initialization is now done via Initialize()
    }

    bool MapDataRegistry::Initialize(const std::string& archive_path) {
        std::string path = archive_path;

        // If no path is provided, use the default path
        if (path.empty()) {
            path = GetDefaultArchivePath();
        }

        // Initialize the archive loader
        return MapArchiveLoader::GetInstance().Initialize(path);
    }

    std::string MapDataRegistry::GetMapData(int32_t map_id) {
        return MapArchiveLoader::GetInstance().LoadMapData(map_id);
    }

    bool MapDataRegistry::HasMap(int32_t map_id) const {
        return MapArchiveLoader::GetInstance().HasMap(map_id);
    }

    std::vector<int32_t> MapDataRegistry::GetAvailableMapIds() const {
        return MapArchiveLoader::GetInstance().GetAvailableMapIds();
    }

    bool MapDataRegistry::IsInitialized() const {
        return MapArchiveLoader::GetInstance().IsInitialized();
    }

    std::string MapDataRegistry::GetDefaultArchivePath() const {
#ifdef _WIN32
        // Get the DLL module path
        char dll_path[MAX_PATH];
        HMODULE hModule = NULL;

        // Get handle to current module
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&MapDataRegistry::GetInstance),
                               &hModule)) {
            GetModuleFileNameA(hModule, dll_path, MAX_PATH);

            // Remove filename to get just the folder
            PathRemoveFileSpecA(dll_path);

            // Add maps.zip
            std::string result(dll_path);
            result += "\\maps.zip";
            return result;
        }
#endif
        // Fallback: search in current directory
        return "maps.zip";
    }

} // namespace Pathfinder
