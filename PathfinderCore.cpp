#include "PathfinderCore.h"
#include <queue>
#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_set>

// Simple JSON parser (minimal, just for our format)
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Pathfinder {

    bool PathfinderEngine::LoadMapData(int32_t map_id, const std::string& json_data) {
        MapData map_data;
        if (!ParseMapJson(json_data, map_data)) {
            return false;
        }

        map_data.map_id = map_id;
        m_loaded_maps[map_id] = std::move(map_data);
        return true;
    }

    bool PathfinderEngine::ParseMapJson(const std::string& json_data, MapData& out_map_data) {
        try {
            auto j = json::parse(json_data);

            // Parse map IDs (may contain multiple IDs)
            if (j.contains("map_ids") && j["map_ids"].is_array()) {
                // Take the first ID from the list
                if (!j["map_ids"].empty()) {
                    out_map_data.map_id = j["map_ids"][0].get<int32_t>();
                }
            }

            // Parse points
            if (j.contains("points") && j["points"].is_array()) {
                for (const auto& p : j["points"]) {
                    if (p.is_array() && p.size() >= 3) {
                        int32_t id = p[0].get<int32_t>();
                        float x = p[1].get<float>();
                        float y = p[2].get<float>();
                        int32_t layer = (p.size() >= 4) ? p[3].get<int32_t>() : 0;
                        out_map_data.points.emplace_back(id, x, y, layer);
                    }
                }
            }

            // Parse visibility graph
            if (j.contains("vis_graph") && j["vis_graph"].is_array()) {
                out_map_data.visibility_graph.resize(j["vis_graph"].size());

                for (size_t i = 0; i < j["vis_graph"].size(); ++i) {
                    const auto& edges = j["vis_graph"][i];
                    if (!edges.is_array()) continue;

                    for (const auto& edge : edges) {
                        if (edge.is_array() && edge.size() >= 2) {
                            int32_t target_id = edge[0].get<int32_t>();
                            float distance = edge[1].get<float>();

                            std::vector<uint32_t> blocking_layers;
                            if (edge.size() >= 3 && edge[2].is_array()) {
                                for (const auto& layer : edge[2]) {
                                    blocking_layers.push_back(layer.get<uint32_t>());
                                }
                            }

                            out_map_data.visibility_graph[i].emplace_back(
                                target_id, distance, blocking_layers
                            );
                        }
                    }
                }
            }

            // Parse trapezoids
            // Format: [id, layer, ax, ay, bx, by, cx, cy, dx, dy]
            if (j.contains("trapezoids") && j["trapezoids"].is_array()) {
                for (const auto& trap : j["trapezoids"]) {
                    if (trap.is_array() && trap.size() >= 10) {
                        int32_t id = trap[0].get<int32_t>();
                        int32_t layer = trap[1].get<int32_t>();
                        float ax = trap[2].get<float>();
                        float ay = trap[3].get<float>();
                        float bx = trap[4].get<float>();
                        float by = trap[5].get<float>();
                        float cx = trap[6].get<float>();
                        float cy = trap[7].get<float>();
                        float dx = trap[8].get<float>();
                        float dy = trap[9].get<float>();

                        out_map_data.trapezoids.emplace_back(
                            id, layer, ax, ay, bx, by, cx, cy, dx, dy
                        );
                    }
                }
            }

            // Parse teleporters
            if (j.contains("teleports") && j["teleports"].is_array()) {
                for (const auto& tp : j["teleports"]) {
                    if (tp.is_array() && tp.size() >= 6) {
                        float enter_x = tp[0].get<float>();
                        float enter_y = tp[1].get<float>();
                        int32_t enter_layer = tp[2].get<int32_t>();
                        float exit_x = tp[3].get<float>();
                        float exit_y = tp[4].get<float>();
                        int32_t exit_layer = tp[5].get<int32_t>();
                        int32_t direction = (tp.size() >= 7) ? tp[6].get<int32_t>() : 0;

                        out_map_data.teleporters.emplace_back(
                            enter_x, enter_y, exit_x, exit_y, direction
                        );
                    }
                }
            }

            // Parse travel portals
            // Format: [x, y, [[map_id, dest_x, dest_y], ...]]
            if (j.contains("travel_portals") && j["travel_portals"].is_array()) {
                for (const auto& portal : j["travel_portals"]) {
                    if (portal.is_array() && portal.size() >= 2) {
                        float portal_x = portal[0].get<float>();
                        float portal_y = portal[1].get<float>();

                        TravelPortal tp(portal_x, portal_y);

                        // Parse connections if present
                        if (portal.size() >= 3 && portal[2].is_array()) {
                            for (const auto& conn : portal[2]) {
                                if (conn.is_array() && conn.size() >= 3) {
                                    int32_t map_id = conn[0].get<int32_t>();
                                    float dest_x = conn[1].get<float>();
                                    float dest_y = conn[2].get<float>();

                                    tp.connections.emplace_back(map_id, dest_x, dest_y);
                                }
                            }
                        }

                        out_map_data.travel_portals.push_back(std::move(tp));
                    }
                }
            }

            // Parse NPC Travel
            // Format: [npcX, npcY, dialogid1, dialogid2, dialogid3, dialogid4, dialogid5, mapid, posX, posY]
            if (j.contains("npc_travel") && j["npc_travel"].is_array()) {
                for (const auto& npc : j["npc_travel"]) {
                    if (npc.is_array() && npc.size() >= 10) {
                        float npc_x = npc[0].get<float>();
                        float npc_y = npc[1].get<float>();
                        int32_t d1 = npc[2].get<int32_t>();
                        int32_t d2 = npc[3].get<int32_t>();
                        int32_t d3 = npc[4].get<int32_t>();
                        int32_t d4 = npc[5].get<int32_t>();
                        int32_t d5 = npc[6].get<int32_t>();
                        int32_t map_id = npc[7].get<int32_t>();
                        float dest_x = npc[8].get<float>();
                        float dest_y = npc[9].get<float>();

                        out_map_data.npc_travels.emplace_back(
                            npc_x, npc_y, d1, d2, d3, d4, d5, map_id, dest_x, dest_y
                        );
                    }
                }
            }

            // Parse Enter Travel
            // Format: [enterX, enterY, mapid, destX, destY]
            if (j.contains("enter_travel") && j["enter_travel"].is_array()) {
                for (const auto& enter : j["enter_travel"]) {
                    if (enter.is_array() && enter.size() >= 5) {
                        float enter_x = enter[0].get<float>();
                        float enter_y = enter[1].get<float>();
                        int32_t map_id = enter[2].get<int32_t>();
                        float dest_x = enter[3].get<float>();
                        float dest_y = enter[4].get<float>();

                        out_map_data.enter_travels.emplace_back(
                            enter_x, enter_y, map_id, dest_x, dest_y
                        );
                    }
                }
            }

            // Parse stats
            if (j.contains("stats") && j["stats"].is_object()) {
                const auto& stats = j["stats"];
                if (stats.contains("trapezoid_count")) {
                    out_map_data.stats.trapezoid_count = stats["trapezoid_count"].get<int32_t>();
                }
                if (stats.contains("point_count")) {
                    out_map_data.stats.point_count = stats["point_count"].get<int32_t>();
                }
                if (stats.contains("teleport_count")) {
                    out_map_data.stats.teleport_count = stats["teleport_count"].get<int32_t>();
                }
                if (stats.contains("travel_portal_count")) {
                    out_map_data.stats.travel_portal_count = stats["travel_portal_count"].get<int32_t>();
                }
                if (stats.contains("npc_travel_count")) {
                    out_map_data.stats.npc_travel_count = stats["npc_travel_count"].get<int32_t>();
                }
                if (stats.contains("enter_travel_count")) {
                    out_map_data.stats.enter_travel_count = stats["enter_travel_count"].get<int32_t>();
                }
            }

            return out_map_data.IsValid();
        }
        catch (const std::exception&) {
            return false;
        }
    }

    std::vector<PathPointWithLayer> PathfinderEngine::FindPath(
        int32_t map_id,
        const Vec2f& start,
        const Vec2f& goal,
        float& out_cost
    ) {
        out_cost = -1.0f;

        auto it = m_loaded_maps.find(map_id);
        if (it == m_loaded_maps.end()) {
            return {}; // Map not loaded
        }

        // Use const reference - we will NOT modify map_data
        const MapData& map_data = it->second;

        // Find start point: closest existing point
        int32_t start_id = FindClosestPoint(map_data, start);
        if (start_id < 0) {
            return {}; // No valid start point
        }

        // Find goal point: closest existing point
        int32_t goal_id = FindClosestPoint(map_data, goal);
        if (goal_id < 0) {
            return {}; // No valid goal point
        }

        // Run A* (no graph modification needed)
        std::vector<int32_t> came_from = AStar(map_data, start_id, goal_id);

        std::vector<PathPointWithLayer> path;
        if (!came_from.empty()) {
            // Reconstruct the path
            path = ReconstructPath(map_data, came_from, start_id, goal_id);

            // Add the actual start position as first point if different from closest point
            if (!path.empty()) {
                const Vec2f& first_point = path[0].pos;
                if (start.SquaredDistance(first_point) > 1.0f) {
                    // Find layer for start position
                    int32_t start_layer = path[0].layer;
                    const Trapezoid* trap = map_data.FindTrapezoidContaining(start);
                    if (trap) {
                        start_layer = trap->layer;
                    }
                    path.insert(path.begin(), PathPointWithLayer(start, start_layer));
                }
            }

            // Add the actual goal position as last point if different from closest point
            if (!path.empty()) {
                const Vec2f& last_point = path.back().pos;
                if (goal.SquaredDistance(last_point) > 1.0f) {
                    // Find layer for goal position
                    int32_t goal_layer = path.back().layer;
                    const Trapezoid* trap = map_data.FindTrapezoidContaining(goal);
                    if (trap) {
                        goal_layer = trap->layer;
                    }
                    path.push_back(PathPointWithLayer(goal, goal_layer));
                }
            }

            // Calculate total cost
            out_cost = 0.0f;
            for (size_t i = 1; i < path.size(); ++i) {
                out_cost += path[i - 1].pos.Distance(path[i].pos);
            }
        }

        return path;
    }

    std::vector<PathPointWithLayer> PathfinderEngine::FindPathWithObstacles(
        int32_t map_id,
        const Vec2f& start,
        const Vec2f& goal,
        const std::vector<ObstacleZone>& obstacles,
        float& out_cost
    ) {
        out_cost = -1.0f;

        auto it = m_loaded_maps.find(map_id);
        if (it == m_loaded_maps.end()) {
            return {}; // Map not loaded
        }

        // Use const reference - we will NOT modify map_data
        const MapData& map_data = it->second;

        // Find start point: closest existing point avoiding obstacles
        int32_t start_id = FindClosestPointAvoidingObstacles(map_data, start, obstacles);
        if (start_id < 0) {
            return {}; // No valid start point
        }

        // Find goal point: closest existing point avoiding obstacles
        int32_t goal_id = FindClosestPointAvoidingObstacles(map_data, goal, obstacles);
        if (goal_id < 0) {
            return {}; // No valid goal point
        }

        // Run A* with obstacle avoidance (no graph modification needed)
        std::vector<int32_t> came_from = AStarWithObstacles(map_data, start_id, goal_id, obstacles);

        std::vector<PathPointWithLayer> path;
        if (!came_from.empty()) {
            // Reconstruct the path
            path = ReconstructPath(map_data, came_from, start_id, goal_id);

            // Add the actual start position as first point if different from closest point
            if (!path.empty()) {
                const Vec2f& first_point = path[0].pos;
                if (start.SquaredDistance(first_point) > 1.0f) {
                    // Find layer for start position
                    int32_t start_layer = path[0].layer;
                    const Trapezoid* trap = map_data.FindTrapezoidContaining(start);
                    if (trap) {
                        start_layer = trap->layer;
                    }
                    path.insert(path.begin(), PathPointWithLayer(start, start_layer));
                }
            }

            // Add the actual goal position as last point if different from closest point
            if (!path.empty()) {
                const Vec2f& last_point = path.back().pos;
                if (goal.SquaredDistance(last_point) > 1.0f) {
                    // Find layer for goal position
                    int32_t goal_layer = path.back().layer;
                    const Trapezoid* trap = map_data.FindTrapezoidContaining(goal);
                    if (trap) {
                        goal_layer = trap->layer;
                    }
                    path.push_back(PathPointWithLayer(goal, goal_layer));
                }
            }

            // Calculate total cost
            out_cost = 0.0f;
            for (size_t i = 1; i < path.size(); ++i) {
                out_cost += path[i - 1].pos.Distance(path[i].pos);
            }
        }

        return path;
    }

    std::vector<int32_t> PathfinderEngine::AStar(
        const MapData& map_data,
        int32_t start_id,
        int32_t goal_id
    ) {
        if (start_id < 0 || start_id >= static_cast<int32_t>(map_data.points.size()) ||
            goal_id < 0 || goal_id >= static_cast<int32_t>(map_data.points.size())) {
            return {};
        }

        // Priority queue: (priority, node_id)
        using PQElement = std::pair<float, int32_t>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> open_set;

        std::vector<float> cost_so_far(map_data.points.size(), std::numeric_limits<float>::infinity());
        std::vector<int32_t> came_from(map_data.points.size(), -1);

        cost_so_far[start_id] = 0.0f;
        came_from[start_id] = start_id;
        open_set.emplace(0.0f, start_id);

        const bool has_teleporters = !map_data.teleporters.empty();
        int32_t current_id = -1;

        while (!open_set.empty()) {
            current_id = open_set.top().second;
            open_set.pop();

            if (current_id == goal_id) {
                break; // Path found
            }

            // Explore neighbors
            if (current_id >= 0 && current_id < static_cast<int32_t>(map_data.visibility_graph.size())) {
                const auto& edges = map_data.visibility_graph[current_id];

                for (const auto& edge : edges) {
                    int32_t neighbor_id = edge.target_id;
                    if (neighbor_id < 0 || neighbor_id >= static_cast<int32_t>(map_data.points.size())) {
                        continue;
                    }

                    float new_cost = cost_so_far[current_id] + edge.distance;

                    if (new_cost < cost_so_far[neighbor_id]) {
                        cost_so_far[neighbor_id] = new_cost;
                        came_from[neighbor_id] = current_id;

                        // Calculate priority with heuristic
                        float priority = new_cost;

                        if (has_teleporters) {
                            const Vec2f& neighbor_pos = map_data.points[neighbor_id].pos;
                            const Vec2f& goal_pos = map_data.points[goal_id].pos;

                            float direct_dist = neighbor_pos.Distance(goal_pos);
                            float tp_dist = TeleporterHeuristic(map_data, neighbor_pos, goal_pos);

                            priority += std::min(direct_dist, tp_dist);
                        } else {
                            const Vec2f& neighbor_pos = map_data.points[neighbor_id].pos;
                            const Vec2f& goal_pos = map_data.points[goal_id].pos;
                            priority += neighbor_pos.Distance(goal_pos);
                        }

                        open_set.emplace(priority, neighbor_id);
                    }
                }
            }
        }

        if (current_id != goal_id) {
            return {}; // No path found
        }

        return came_from;
    }

    std::vector<int32_t> PathfinderEngine::AStarWithObstacles(
        const MapData& map_data,
        int32_t start_id,
        int32_t goal_id,
        const std::vector<ObstacleZone>& obstacles
    ) {
        if (start_id < 0 || start_id >= static_cast<int32_t>(map_data.points.size()) ||
            goal_id < 0 || goal_id >= static_cast<int32_t>(map_data.points.size())) {
            return {};
        }

        // Priority queue: (priority, node_id)
        using PQElement = std::pair<float, int32_t>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> open_set;

        std::vector<float> cost_so_far(map_data.points.size(), std::numeric_limits<float>::infinity());
        std::vector<int32_t> came_from(map_data.points.size(), -1);

        cost_so_far[start_id] = 0.0f;
        came_from[start_id] = start_id;
        open_set.emplace(0.0f, start_id);

        const bool has_teleporters = !map_data.teleporters.empty();
        int32_t current_id = -1;

        while (!open_set.empty()) {
            current_id = open_set.top().second;
            open_set.pop();

            if (current_id == goal_id) {
                break; // Path found
            }

            // Skip if this node is inside an obstacle zone (shouldn't happen if start was validated)
            const Vec2f& current_pos = map_data.points[current_id].pos;
            if (IsPointBlocked(current_pos, obstacles)) {
                continue;
            }

            // Explore neighbors
            if (current_id >= 0 && current_id < static_cast<int32_t>(map_data.visibility_graph.size())) {
                const auto& edges = map_data.visibility_graph[current_id];

                for (const auto& edge : edges) {
                    int32_t neighbor_id = edge.target_id;
                    if (neighbor_id < 0 || neighbor_id >= static_cast<int32_t>(map_data.points.size())) {
                        continue;
                    }

                    // Skip neighbors that are inside obstacle zones
                    const Vec2f& neighbor_pos = map_data.points[neighbor_id].pos;
                    if (IsPointBlocked(neighbor_pos, obstacles)) {
                        continue;
                    }

                    float new_cost = cost_so_far[current_id] + edge.distance;

                    if (new_cost < cost_so_far[neighbor_id]) {
                        cost_so_far[neighbor_id] = new_cost;
                        came_from[neighbor_id] = current_id;

                        // Calculate priority with heuristic
                        float priority = new_cost;

                        if (has_teleporters) {
                            const Vec2f& goal_pos = map_data.points[goal_id].pos;

                            float direct_dist = neighbor_pos.Distance(goal_pos);
                            float tp_dist = TeleporterHeuristic(map_data, neighbor_pos, goal_pos);

                            priority += std::min(direct_dist, tp_dist);
                        } else {
                            const Vec2f& goal_pos = map_data.points[goal_id].pos;
                            priority += neighbor_pos.Distance(goal_pos);
                        }

                        open_set.emplace(priority, neighbor_id);
                    }
                }
            }
        }

        if (current_id != goal_id) {
            return {}; // No path found
        }

        return came_from;
    }

    std::vector<PathPointWithLayer> PathfinderEngine::ReconstructPath(
        const MapData& map_data,
        const std::vector<int32_t>& came_from,
        int32_t start_id,
        int32_t goal_id
    ) {
        std::vector<PathPointWithLayer> path;
        int32_t current = goal_id;
        int32_t count = 0;
        const int32_t max_count = static_cast<int32_t>(map_data.points.size() * 2);

        while (current != start_id && count < max_count) {
            if (current < 0 || current >= static_cast<int32_t>(map_data.points.size())) {
                break;
            }

            path.emplace_back(map_data.points[current].pos, map_data.points[current].layer);
            current = came_from[current];
            count++;
        }

        if (current == start_id) {
            path.emplace_back(map_data.points[start_id].pos, map_data.points[start_id].layer);
        }

        std::reverse(path.begin(), path.end());
        return path;
    }

    int32_t PathfinderEngine::FindClosestPoint(
        const MapData& map_data,
        const Vec2f& pos
    ) {
        if (map_data.points.empty()) {
            return -1;
        }

        int32_t closest_id = -1;
        float min_dist = std::numeric_limits<float>::infinity();

        for (const auto& point : map_data.points) {
            float dist = pos.SquaredDistance(point.pos);
            if (dist < min_dist) {
                min_dist = dist;
                closest_id = point.id;
            }
        }

        return closest_id;
    }

    int32_t PathfinderEngine::FindClosestPointAvoidingObstacles(
        const MapData& map_data,
        const Vec2f& pos,
        const std::vector<ObstacleZone>& obstacles
    ) {
        if (map_data.points.empty()) {
            return -1;
        }

        int32_t closest_id = -1;
        float min_dist = std::numeric_limits<float>::infinity();

        for (const auto& point : map_data.points) {
            // Skip points that are inside obstacle zones
            if (IsPointBlocked(point.pos, obstacles)) {
                continue;
            }

            float dist = pos.SquaredDistance(point.pos);
            if (dist < min_dist) {
                min_dist = dist;
                closest_id = point.id;
            }
        }

        return closest_id;
    }

    bool PathfinderEngine::IsPointBlocked(
        const Vec2f& point,
        const std::vector<ObstacleZone>& obstacles
    ) const {
        for (const auto& obstacle : obstacles) {
            if (obstacle.Contains(point)) {
                return true;
            }
        }
        return false;
    }

    float PathfinderEngine::Heuristic(
        const MapData& map_data,
        const Vec2f& from,
        const Vec2f& to
    ) {
        return from.Distance(to);
    }

    float PathfinderEngine::TeleporterHeuristic(
        const MapData& map_data,
        const Vec2f& from,
        const Vec2f& to
    ) {
        if (map_data.teleporters.empty()) {
            return std::numeric_limits<float>::infinity();
        }

        // Find the teleporter closest to start
        float closest_to_start_dist = std::numeric_limits<float>::infinity();
        const Teleporter* closest_to_start = nullptr;

        for (const auto& tp : map_data.teleporters) {
            float dist = from.Distance(tp.enter);
            if (tp.direction == 1) { // both-ways
                dist = std::min(dist, from.Distance(tp.exit));
            }

            if (dist < closest_to_start_dist) {
                closest_to_start_dist = dist;
                closest_to_start = &tp;
            }
        }

        // Find the teleporter closest to goal
        float closest_to_goal_dist = std::numeric_limits<float>::infinity();
        const Teleporter* closest_to_goal = nullptr;

        for (const auto& tp : map_data.teleporters) {
            float dist = to.Distance(tp.exit);
            if (tp.direction == 1) { // both-ways
                dist = std::min(dist, to.Distance(tp.enter));
            }

            if (dist < closest_to_goal_dist) {
                closest_to_goal_dist = dist;
                closest_to_goal = &tp;
            }
        }

        if (!closest_to_start || !closest_to_goal) {
            return std::numeric_limits<float>::infinity();
        }

        // Calculate cost via teleporters
        if (closest_to_start == closest_to_goal) {
            return closest_to_start_dist + closest_to_start->exit.Distance(to);
        }

        float tp_travel_cost = closest_to_start->exit.Distance(closest_to_goal->exit);
        return closest_to_start_dist + tp_travel_cost + closest_to_goal_dist;
    }

    std::vector<PathPointWithLayer> PathfinderEngine::SimplifyPath(
        const std::vector<PathPointWithLayer>& path,
        float min_spacing
    ) {
        if (path.size() <= 2 || min_spacing <= 0.0f) {
            return path;
        }

        std::vector<PathPointWithLayer> simplified;
        simplified.push_back(path[0]); // Always include the first point

        PathPointWithLayer last_added = path[0];

        for (size_t i = 1; i < path.size() - 1; ++i) {
            float dist = last_added.pos.Distance(path[i].pos);
            // Keep point if distance is enough OR if layer changes (important for bridges!)
            if (dist >= min_spacing || path[i].layer != last_added.layer) {
                simplified.push_back(path[i]);
                last_added = path[i];
            }
        }

        simplified.push_back(path.back()); // Always include the last point

        return simplified;
    }

    bool PathfinderEngine::IsMapLoaded(int32_t map_id) const {
        return m_loaded_maps.find(map_id) != m_loaded_maps.end();
    }

    std::vector<int32_t> PathfinderEngine::GetLoadedMapIds() const {
        std::vector<int32_t> ids;
        ids.reserve(m_loaded_maps.size());

        for (const auto& pair : m_loaded_maps) {
            ids.push_back(pair.first);
        }

        return ids;
    }

    bool PathfinderEngine::GetMapStatistics(int32_t map_id, MapStatistics& out_stats) const {
        auto it = m_loaded_maps.find(map_id);
        if (it == m_loaded_maps.end()) {
            return false;
        }

        out_stats = it->second.stats;
        return true;
    }

    int32_t PathfinderEngine::CreateTemporaryPoint(
        MapData& map_data,
        const Vec2f& pos
    ) {
        // Find the trapezoid containing this position
        const Trapezoid* trap = map_data.FindTrapezoidContaining(pos);
        if (!trap) {
            return -1; // Point is not in a valid walkable area
        }

        // Create a new point with a unique ID
        int32_t new_id = static_cast<int32_t>(map_data.points.size());
        map_data.points.emplace_back(new_id, pos, trap->layer);

        // Ensure the visibility graph has space for this point
        if (map_data.visibility_graph.size() <= static_cast<size_t>(new_id)) {
            map_data.visibility_graph.resize(new_id + 1);
        }

        return new_id;
    }

    void PathfinderEngine::InsertPointIntoVisGraph(
        MapData& map_data,
        int32_t point_id,
        int32_t max_connections,
        float max_range
    ) {
        if (point_id < 0 || point_id >= static_cast<int32_t>(map_data.points.size())) {
            return;
        }

        const Point& point = map_data.points[point_id];
        const float max_range_squared = max_range * max_range;

        // Collect all nearby points with their distances
        struct Connection {
            int32_t id;
            float distance;
        };
        std::vector<Connection> connections;

        for (size_t i = 0; i < map_data.points.size(); ++i) {
            if (static_cast<int32_t>(i) == point_id) continue;

            const Point& other = map_data.points[i];

            // Skip points on different layers (unless we have teleporters)
            // For now, only connect to points on the same layer
            if (other.layer != point.layer) continue;

            float dist_sq = point.pos.SquaredDistance(other.pos);
            if (dist_sq < max_range_squared) {
                // Check if this point has connections in the vis_graph
                // Only connect to points that are already connected to others
                if (i < map_data.visibility_graph.size() && !map_data.visibility_graph[i].empty()) {
                    connections.push_back({static_cast<int32_t>(i), std::sqrt(dist_sq)});
                }
            }
        }

        // Sort by distance and keep only the closest connections
        std::sort(connections.begin(), connections.end(),
            [](const Connection& a, const Connection& b) { return a.distance < b.distance; });

        if (connections.size() > static_cast<size_t>(max_connections)) {
            connections.resize(max_connections);
        }

        // Add edges from the new point to nearby points
        for (const auto& conn : connections) {
            map_data.visibility_graph[point_id].emplace_back(conn.id, conn.distance);

            // Add reverse edge (bidirectional)
            if (conn.id < static_cast<int32_t>(map_data.visibility_graph.size())) {
                map_data.visibility_graph[conn.id].emplace_back(point_id, conn.distance);
            }
        }
    }

    void PathfinderEngine::RemoveTemporaryPoints(
        MapData& map_data,
        size_t original_point_count,
        size_t original_visgraph_size
    ) {
        // Remove edges added to existing points that reference temporary points
        // IMPORTANT: We must iterate through ALL original points, not just up to original_visgraph_size
        // because InsertPointIntoVisGraph adds bidirectional edges to existing points
        for (size_t i = 0; i < original_point_count && i < map_data.visibility_graph.size(); ++i) {
            auto& edges = map_data.visibility_graph[i];
            edges.erase(
                std::remove_if(edges.begin(), edges.end(),
                    [original_point_count](const VisibilityEdge& edge) {
                        return edge.target_id >= static_cast<int32_t>(original_point_count);
                    }),
                edges.end()
            );
        }

        // Remove temporary vis_graph entries
        if (map_data.visibility_graph.size() > original_visgraph_size) {
            map_data.visibility_graph.resize(original_visgraph_size);
        }

        // Remove temporary points from the points array
        if (map_data.points.size() > original_point_count) {
            map_data.points.resize(original_point_count);
        }
    }

} // namespace Pathfinder
