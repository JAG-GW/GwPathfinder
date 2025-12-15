#pragma once

#ifdef PATHFINDER_EXPORTS
#define PATHFINDER_API __declspec(dllexport)
#else
#define PATHFINDER_API __declspec(dllimport)
#endif

#include <cstdint>

extern "C" {
    // Structure pour un point du chemin
    struct PathPoint {
        float x;
        float y;
    };

    // Structure pour le résultat du pathfinding
    struct PathResult {
        PathPoint* points;      // Tableau des points du chemin
        int32_t point_count;    // Nombre de points
        float total_cost;       // Coût total du chemin
        int32_t error_code;     // 0 = succès, autre = erreur
        char error_message[256]; // Message d'erreur si applicable
    };

    // Structure pour les statistiques d'une carte
    struct MapStats {
        int32_t trapezoid_count;    // Nombre de trapézoïdes
        int32_t point_count;        // Nombre de points
        int32_t teleport_count;     // Nombre de téléporteurs
        int32_t travel_portal_count; // Nombre de portails de voyage
        int32_t npc_travel_count;   // Nombre de voyages via NPC
        int32_t enter_travel_count; // Nombre de voyages via touche Entrée
        int32_t error_code;         // 0 = succès, autre = erreur
        char error_message[256];    // Message d'erreur si applicable
    };

    /**
     * @brief Trouve un chemin entre deux points sur une carte
     *
     * @param map_id ID de la carte GW
     * @param start_x Coordonnée X de départ
     * @param start_y Coordonnée Y de départ
     * @param dest_x Coordonnée X de destination
     * @param dest_y Coordonnée Y de destination
     * @param range Distance minimale entre les points simplifiés (0 = pas de simplification)
     * @return PathResult* Pointeur vers le résultat (doit être libéré avec FreePathResult)
     */
    PATHFINDER_API PathResult* FindPath(
        int32_t map_id,
        float start_x,
        float start_y,
        float dest_x,
        float dest_y,
        float range
    );

    /**
     * @brief Libère la mémoire allouée pour un résultat de pathfinding
     *
     * @param result Pointeur vers le résultat à libérer
     */
    PATHFINDER_API void FreePathResult(PathResult* result);

    /**
     * @brief Vérifie si une carte est disponible dans la DLL
     *
     * @param map_id ID de la carte GW
     * @return int32_t 1 si disponible, 0 sinon
     */
    PATHFINDER_API int32_t IsMapAvailable(int32_t map_id);

    /**
     * @brief Retourne la liste des IDs de cartes disponibles
     *
     * @param count Pointeur pour recevoir le nombre de cartes
     * @return int32_t* Tableau des IDs (doit être libéré avec FreeMapList)
     */
    PATHFINDER_API int32_t* GetAvailableMaps(int32_t* count);

    /**
     * @brief Libère la mémoire allouée pour la liste des cartes
     *
     * @param map_list Pointeur vers la liste à libérer
     */
    PATHFINDER_API void FreeMapList(int32_t* map_list);

    /**
     * @brief Retourne la version de la DLL
     *
     * @return const char* Version string (ne pas libérer)
     */
    PATHFINDER_API const char* GetPathfinderVersion();

    /**
     * @brief Initialise la DLL (optionnel, appelé automatiquement)
     *
     * @return int32_t 1 si succès, 0 sinon
     */
    PATHFINDER_API int32_t Initialize();

    /**
     * @brief Nettoie les ressources de la DLL
     */
    PATHFINDER_API void Shutdown();

    /**
     * @brief Charge une carte depuis un fichier JSON externe
     *
     * @param map_id ID de la carte à charger
     * @param file_path Chemin vers le fichier JSON
     * @return int32_t 1 si succès, 0 sinon
     */
    PATHFINDER_API int32_t LoadMapFromFile(int32_t map_id, const char* file_path);

    /**
     * @brief Récupère les statistiques d'une carte
     *
     * @param map_id ID de la carte
     * @return MapStats* Pointeur vers les stats (doit être libéré avec FreeMapStats)
     */
    PATHFINDER_API MapStats* GetMapStats(int32_t map_id);

    /**
     * @brief Libère la mémoire allouée pour les statistiques
     *
     * @param stats Pointeur vers les stats à libérer
     */
    PATHFINDER_API void FreeMapStats(MapStats* stats);

}
