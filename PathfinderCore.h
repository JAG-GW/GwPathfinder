#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <cmath>

namespace Pathfinder {

    // Structure for a 2D point
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

    // Structure for a graph point
    struct Point {
        int32_t id;
        Vec2f pos;

        Point() : id(-1), pos() {}
        Point(int32_t _id, float _x, float _y) : id(_id), pos(_x, _y) {}
        Point(int32_t _id, const Vec2f& _pos) : id(_id), pos(_pos) {}
    };

    // Structure for a visibility graph edge
    struct VisibilityEdge {
        int32_t target_id;      // Target point ID
        float distance;         // Distance to target point
        std::vector<uint32_t> blocking_layers; // Layers that block this path

        VisibilityEdge() : target_id(-1), distance(0.0f) {}
        VisibilityEdge(int32_t id, float dist) : target_id(id), distance(dist) {}
        VisibilityEdge(int32_t id, float dist, const std::vector<uint32_t>& layers)
            : target_id(id), distance(dist), blocking_layers(layers) {}
    };

    // Structure for a teleporter
    struct Teleporter {
        Vec2f enter;    // Entry point
        Vec2f exit;     // Exit point
        int32_t direction; // 0 = one-way, 1 = both-ways

        Teleporter() : enter(), exit(), direction(0) {}
        Teleporter(float enter_x, float enter_y, float exit_x, float exit_y, int32_t dir)
            : enter(enter_x, enter_y), exit(exit_x, exit_y), direction(dir) {}
    };

    // Structure for a travel portal connection
    struct PortalConnection {
        int32_t dest_map_id;    // Destination map ID
        Vec2f dest_pos;         // Destination position on target map

        PortalConnection() : dest_map_id(0), dest_pos() {}
        PortalConnection(int32_t map_id, float x, float y)
            : dest_map_id(map_id), dest_pos(x, y) {}
    };

    // Structure for a travel portal
    struct TravelPortal {
        Vec2f position;                             // Portal position
        std::vector<PortalConnection> connections;  // List of possible destinations

        TravelPortal() : position() {}
        TravelPortal(float x, float y) : position(x, y) {}
    };

    // Structure for NPC travel
    struct NpcTravel {
        Vec2f npc_pos;          // NPC position
        int32_t dialog_ids[5];  // Dialog IDs
        int32_t dest_map_id;    // Destination map ID
        Vec2f dest_pos;         // Destination position

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

    // Structure for Enter key travel
    struct EnterTravel {
        Vec2f enter_pos;        // Entry point position on the map
        int32_t dest_map_id;    // Destination map ID
        Vec2f dest_pos;         // Destination position

        EnterTravel() : enter_pos(), dest_map_id(0), dest_pos() {}
        EnterTravel(float enter_x, float enter_y, int32_t map_id, float dest_x, float dest_y)
            : enter_pos(enter_x, enter_y), dest_map_id(map_id), dest_pos(dest_x, dest_y) {}
    };

    // Structure for map statistics
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


    // Map data structure
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

    // Main pathfinding class
    class PathfinderEngine {
    public:
        PathfinderEngine() = default;
        ~PathfinderEngine() = default;

        // Loads map data from JSON
        bool LoadMapData(int32_t map_id, const std::string& json_data);

        // Finds a path between two points
        std::vector<Vec2f> FindPath(
            int32_t map_id,
            const Vec2f& start,
            const Vec2f& goal,
            float& out_cost
        );

        // Simplifies a path (removes intermediate points that are too close)
        std::vector<Vec2f> SimplifyPath(
            const std::vector<Vec2f>& path,
            float min_spacing
        );

        // Checks if a map is loaded
        bool IsMapLoaded(int32_t map_id) const;

        // Returns the IDs of loaded maps
        std::vector<int32_t> GetLoadedMapIds() const;

        // Gets the statistics of a map
        bool GetMapStatistics(int32_t map_id, MapStatistics& out_stats) const;

    private:
        // A* algorithm
        std::vector<int32_t> AStar(
            const MapData& map_data,
            int32_t start_id,
            int32_t goal_id
        );

        // Finds the closest point to a position
        int32_t FindClosestPoint(
            const MapData& map_data,
            const Vec2f& pos
        );

        // Calculates the heuristic for A*
        float Heuristic(
            const MapData& map_data,
            const Vec2f& from,
            const Vec2f& to
        );

        // Heuristic with teleporters
        float TeleporterHeuristic(
            const MapData& map_data,
            const Vec2f& from,
            const Vec2f& to
        );

        // Reconstructs the path from A* results
        std::vector<Vec2f> ReconstructPath(
            const MapData& map_data,
            const std::vector<int32_t>& came_from,
            int32_t start_id,
            int32_t goal_id
        );

        // Parses a map's JSON
        bool ParseMapJson(const std::string& json_data, MapData& out_map_data);

        // Loaded maps (map_id -> MapData)
        std::unordered_map<int32_t, MapData> m_loaded_maps;
    };

} // namespace Pathfinder
