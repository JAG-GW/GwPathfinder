#include "PathfinderAPI.h"
#include "PathfinderCore.h"
#include "MapDataRegistry.h"
#include <cstring>
#include <memory>
#include <fstream>
#include <sstream>

// Instance globale du moteur de pathfinding
static std::unique_ptr<Pathfinder::PathfinderEngine> g_engine;
static bool g_initialized = false;

extern "C" {

    PATHFINDER_API int32_t Initialize() {
        if (g_initialized) {
            return 1; // Déjà initialisé
        }

        try {
            g_engine = std::make_unique<Pathfinder::PathfinderEngine>();

            // Initialiser le registre des cartes (charge depuis maps.zip)
            auto& registry = Pathfinder::MapDataRegistry::GetInstance();
            if (!registry.Initialize()) {
                // Si l'initialisation échoue, retourner une erreur
                return 0;
            }

            // Note: Les cartes seront chargées à la demande (lazy loading)
            // lors de l'appel à FindPath

            g_initialized = true;
            return 1;
        }
        catch (...) {
            return 0;
        }
    }

    PATHFINDER_API void Shutdown() {
        if (g_initialized) {
            g_engine.reset();
            g_initialized = false;
        }
    }

    PATHFINDER_API PathResult* FindPath(
        int32_t map_id,
        float start_x,
        float start_y,
        float dest_x,
        float dest_y,
        float range
    ) {
        // Auto-initialisation si nécessaire
        if (!g_initialized) {
            if (!Initialize()) {
                PathResult* result = new PathResult();
                result->points = nullptr;
                result->point_count = 0;
                result->total_cost = -1.0f;
                result->error_code = -1;
                std::strncpy(result->error_message, "Failed to initialize pathfinder", 255);
                return result;
            }
        }

        PathResult* result = new PathResult();
        result->points = nullptr;
        result->point_count = 0;
        result->total_cost = -1.0f;
        result->error_code = 0;
        result->error_message[0] = '\0';

        try {
            // Vérifier si la carte est chargée, sinon la charger depuis l'archive
            if (!g_engine->IsMapLoaded(map_id)) {
                auto& registry = Pathfinder::MapDataRegistry::GetInstance();
                std::string map_data = registry.GetMapData(map_id);

                if (map_data.empty()) {
                    result->error_code = 1;
                    std::snprintf(result->error_message, 255, "Map %d not found in archive", map_id);
                    return result;
                }

                // Charger la carte dans le moteur
                if (!g_engine->LoadMapData(map_id, map_data)) {
                    result->error_code = 1;
                    std::snprintf(result->error_message, 255, "Failed to load map %d", map_id);
                    return result;
                }
            }

            // Trouver le chemin
            Pathfinder::Vec2f start(start_x, start_y);
            Pathfinder::Vec2f goal(dest_x, dest_y);
            float cost = 0.0f;

            std::vector<Pathfinder::Vec2f> path = g_engine->FindPath(map_id, start, goal, cost);

            if (path.empty()) {
                result->error_code = 2;
                std::strncpy(result->error_message, "No path found", 255);
                return result;
            }

            // Simplifier le chemin si demandé
            if (range > 0.0f) {
                path = g_engine->SimplifyPath(path, range);
            }

            // Allouer et copier les points
            result->point_count = static_cast<int32_t>(path.size());
            result->points = new PathPoint[result->point_count];
            result->total_cost = cost;

            for (int32_t i = 0; i < result->point_count; ++i) {
                result->points[i].x = path[i].x;
                result->points[i].y = path[i].y;
            }

            return result;
        }
        catch (const std::exception& e) {
            result->error_code = -2;
            std::snprintf(result->error_message, 255, "Exception: %s", e.what());
            return result;
        }
        catch (...) {
            result->error_code = -3;
            std::strncpy(result->error_message, "Unknown exception", 255);
            return result;
        }
    }

    PATHFINDER_API void FreePathResult(PathResult* result) {
        if (result) {
            if (result->points) {
                delete[] result->points;
            }
            delete result;
        }
    }

    PATHFINDER_API int32_t IsMapAvailable(int32_t map_id) {
        if (!g_initialized) {
            Initialize();
        }

        auto& registry = Pathfinder::MapDataRegistry::GetInstance();
        return registry.HasMap(map_id) ? 1 : 0;
    }

    PATHFINDER_API int32_t* GetAvailableMaps(int32_t* count) {
        if (!g_initialized) {
            Initialize();
        }

        if (!count) {
            return nullptr;
        }

        try {
            // Obtenir la liste de toutes les maps disponibles dans l'archive
            auto& registry = Pathfinder::MapDataRegistry::GetInstance();
            std::vector<int32_t> map_ids = registry.GetAvailableMapIds();
            *count = static_cast<int32_t>(map_ids.size());

            if (map_ids.empty()) {
                return nullptr;
            }

            int32_t* result = new int32_t[map_ids.size()];
            std::memcpy(result, map_ids.data(), map_ids.size() * sizeof(int32_t));

            return result;
        }
        catch (...) {
            *count = 0;
            return nullptr;
        }
    }

    PATHFINDER_API void FreeMapList(int32_t* map_list) {
        if (map_list) {
            delete[] map_list;
        }
    }

    PATHFINDER_API const char* GetPathfinderVersion() {
        return "GWPathfinder v1.0.0";
    }

    PATHFINDER_API int32_t LoadMapFromFile(int32_t map_id, const char* file_path) {
        // Auto-initialisation si nécessaire
        if (!g_initialized) {
            if (!Initialize()) {
                return 0;
            }
        }

        if (!file_path) {
            return 0;
        }

        try {
            // Ouvrir le fichier JSON
            std::ifstream file(file_path, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                return 0;
            }

            // Lire tout le contenu
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string json_data = buffer.str();

            // Charger dans le moteur
            if (g_engine->LoadMapData(map_id, json_data)) {
                return 1;
            }

            return 0;
        }
        catch (...) {
            return 0;
        }
    }

    PATHFINDER_API MapStats* GetMapStats(int32_t map_id) {
        // Auto-initialisation si nécessaire
        if (!g_initialized) {
            Initialize();
        }

        MapStats* result = new MapStats();

        if (!g_engine) {
            result->error_code = 1;
            std::snprintf(result->error_message, sizeof(result->error_message), "Pathfinder not initialized");
            return result;
        }

        Pathfinder::MapStatistics stats;
        if (!g_engine->GetMapStatistics(map_id, stats)) {
            result->error_code = 2;
            std::snprintf(result->error_message, sizeof(result->error_message), "Map %d not loaded", map_id);
            return result;
        }

        result->trapezoid_count = stats.trapezoid_count;
        result->point_count = stats.point_count;
        result->teleport_count = stats.teleport_count;
        result->travel_portal_count = stats.travel_portal_count;
        result->npc_travel_count = stats.npc_travel_count;
        result->enter_travel_count = stats.enter_travel_count;
        result->error_code = 0;
        result->error_message[0] = '\0';

        return result;
    }

    PATHFINDER_API void FreeMapStats(MapStats* stats) {
        if (stats) {
            delete stats;
        }
    }

} // extern "C"

// DLL Entry Point
#ifdef _WIN32
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Initialisation automatique au chargement
        Initialize();
        break;

    case DLL_PROCESS_DETACH:
        // Nettoyage automatique au déchargement
        Shutdown();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
#endif
