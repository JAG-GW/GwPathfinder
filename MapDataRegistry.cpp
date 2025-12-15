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
        // L'initialisation se fait maintenant via Initialize()
    }

    bool MapDataRegistry::Initialize(const std::string& archive_path) {
        std::string path = archive_path;

        // Si aucun chemin n'est fourni, utiliser le chemin par défaut
        if (path.empty()) {
            path = GetDefaultArchivePath();
        }

        // Initialiser le chargeur d'archives
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
        // Obtenir le chemin du module DLL
        char dll_path[MAX_PATH];
        HMODULE hModule = NULL;

        // Obtenir le handle du module actuel
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&MapDataRegistry::GetInstance),
                               &hModule)) {
            GetModuleFileNameA(hModule, dll_path, MAX_PATH);

            // Retirer le nom du fichier pour avoir juste le dossier
            PathRemoveFileSpecA(dll_path);

            // Ajouter maps.zip
            std::string result(dll_path);
            result += "\\maps.zip";
            return result;
        }
#endif
        // Fallback: chercher dans le dossier courant
        return "maps.zip";
    }

} // namespace Pathfinder
