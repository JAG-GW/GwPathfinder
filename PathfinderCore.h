#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <cmath>

namespace Pathfinder {

    // Structure pour un point 2D
    struct Vec2f {
        float x;
        float y;

        Vec2f() : x(0), y(0) {}
        Vec2f(float _x, float _y) : x(_x), y(_y) {}

        Vec2f operator+(const Vec2f& other) const {
            return Vec2f(x + other.x, y + other.y);
        }

        Vec2f operator-(const Vec2f& other) const {
            return Vec2f(x - other.x, y - other.y);
        }

        Vec2f operator*(float scalar) const {
            return Vec2f(x * scalar, y * scalar);
        }

        float SquaredDistance(const Vec2f& other) const {
            float dx = x - other.x;
            float dy = y - other.y;
            return dx * dx + dy * dy;
        }

        float Distance(const Vec2f& other) const {
            return std::sqrt(SquaredDistance(other));
        }
    };

    // Structure pour un point du graphe
    struct Point {
        int32_t id;
        Vec2f pos;

        Point() : id(-1), pos() {}
        Point(int32_t _id, float _x, float _y) : id(_id), pos(_x, _y) {}
        Point(int32_t _id, const Vec2f& _pos) : id(_id), pos(_pos) {}
    };

    // Structure pour une arête du graphe de visibilité
    struct VisibilityEdge {
        int32_t target_id;      // ID du point cible
        float distance;         // Distance jusqu'au point cible
        std::vector<uint32_t> blocking_layers; // Layers qui bloquent ce chemin

        VisibilityEdge() : target_id(-1), distance(0.0f) {}
        VisibilityEdge(int32_t id, float dist) : target_id(id), distance(dist) {}
        VisibilityEdge(int32_t id, float dist, const std::vector<uint32_t>& layers)
            : target_id(id), distance(dist), blocking_layers(layers) {}
    };

    // Structure pour un téléporteur
    struct Teleporter {
        Vec2f enter;    // Point d'entrée
        Vec2f exit;     // Point de sortie
        int32_t direction; // 0 = one-way, 1 = both-ways

        Teleporter() : enter(), exit(), direction(0) {}
        Teleporter(float enter_x, float enter_y, float exit_x, float exit_y, int32_t dir)
            : enter(enter_x, enter_y), exit(exit_x, exit_y), direction(dir) {}
    };

    // Structure pour une connexion de portail de voyage
    struct PortalConnection {
        int32_t dest_map_id;    // ID de la carte de destination
        Vec2f dest_pos;         // Position de destination sur la carte cible

        PortalConnection() : dest_map_id(0), dest_pos() {}
        PortalConnection(int32_t map_id, float x, float y)
            : dest_map_id(map_id), dest_pos(x, y) {}
    };

    // Structure pour un portail de voyage
    struct TravelPortal {
        Vec2f position;                             // Position du portail
        std::vector<PortalConnection> connections;  // Liste des destinations possibles

        TravelPortal() : position() {}
        TravelPortal(float x, float y) : position(x, y) {}
    };

    // Structure pour un voyage via NPC
    struct NpcTravel {
        Vec2f npc_pos;          // Position du NPC
        int32_t dialog_ids[5];  // IDs de dialogue
        int32_t dest_map_id;    // ID de la carte de destination
        Vec2f dest_pos;         // Position de destination

        NpcTravel() : npc_pos(), dest_map_id(0), dest_pos() {
            for (int i = 0; i < 5; ++i) dialog_ids[i] = 0;
        }
        NpcTravel(float npc_x, float npc_y, int32_t d1, int32_t d2, int32_t d3, int32_t d4, int32_t d5,
                  int32_t map_id, float dest_x, float dest_y)
            : npc_pos(npc_x, npc_y), dest_map_id(map_id), dest_pos(dest_x, dest_y) {
            dialog_ids[0] = d1;
            dialog_ids[1] = d2;
            dialog_ids[2] = d3;
            dialog_ids[3] = d4;
            dialog_ids[4] = d5;
        }
    };

    // Structure pour un voyage via touche Entrée
    struct EnterTravel {
        Vec2f enter_pos;        // Position du point d'entrée sur la carte
        int32_t dest_map_id;    // ID de la carte de destination
        Vec2f dest_pos;         // Position de destination

        EnterTravel() : enter_pos(), dest_map_id(0), dest_pos() {}
        EnterTravel(float enter_x, float enter_y, int32_t map_id, float dest_x, float dest_y)
            : enter_pos(enter_x, enter_y), dest_map_id(map_id), dest_pos(dest_x, dest_y) {}
    };

    // Structure pour les statistiques d'une carte
    struct MapStatistics {
        int32_t trapezoid_count;
        int32_t point_count;
        int32_t teleport_count;
        int32_t travel_portal_count;
        int32_t npc_travel_count;
        int32_t enter_travel_count;

        MapStatistics() : trapezoid_count(0), point_count(0), teleport_count(0), travel_portal_count(0),
                          npc_travel_count(0), enter_travel_count(0) {}
    };


    // Données d'une carte
    struct MapData {
        int32_t map_id;
        std::vector<Point> points;
        std::vector<std::vector<VisibilityEdge>> visibility_graph;
        std::vector<Teleporter> teleporters;
        std::vector<TravelPortal> travel_portals;
        std::vector<NpcTravel> npc_travels;
        std::vector<EnterTravel> enter_travels;
        MapStatistics stats;

        MapData() : map_id(-1) {}

        bool IsValid() const {
            return map_id > 0 && !points.empty() && !visibility_graph.empty();
        }
    };

    // Classe principale de pathfinding
    class PathfinderEngine {
    public:
        PathfinderEngine() = default;
        ~PathfinderEngine() = default;

        // Charge les données d'une carte depuis JSON
        bool LoadMapData(int32_t map_id, const std::string& json_data);

        // Trouve un chemin entre deux points
        std::vector<Vec2f> FindPath(
            int32_t map_id,
            const Vec2f& start,
            const Vec2f& goal,
            float& out_cost
        );

        // Simplifie un chemin (retire les points intermédiaires trop proches)
        std::vector<Vec2f> SimplifyPath(
            const std::vector<Vec2f>& path,
            float min_spacing
        );

        // Vérifie si une carte est chargée
        bool IsMapLoaded(int32_t map_id) const;

        // Retourne les IDs des cartes chargées
        std::vector<int32_t> GetLoadedMapIds() const;

        // Récupère les statistiques d'une carte
        bool GetMapStatistics(int32_t map_id, MapStatistics& out_stats) const;

    private:
        // Algorithme A*
        std::vector<int32_t> AStar(
            const MapData& map_data,
            int32_t start_id,
            int32_t goal_id
        );

        // Trouve le point le plus proche d'une position
        int32_t FindClosestPoint(
            const MapData& map_data,
            const Vec2f& pos
        );

        // Calcule l'heuristique pour A*
        float Heuristic(
            const MapData& map_data,
            const Vec2f& from,
            const Vec2f& to
        );

        // Heuristique avec téléporteurs
        float TeleporterHeuristic(
            const MapData& map_data,
            const Vec2f& from,
            const Vec2f& to
        );

        // Reconstruit le chemin depuis les résultats de A*
        std::vector<Vec2f> ReconstructPath(
            const MapData& map_data,
            const std::vector<int32_t>& came_from,
            int32_t start_id,
            int32_t goal_id
        );

        // Parse le JSON d'une carte
        bool ParseMapJson(const std::string& json_data, MapData& out_map_data);

        // Maps chargées (map_id -> MapData)
        std::unordered_map<int32_t, MapData> m_loaded_maps;
    };

} // namespace Pathfinder
