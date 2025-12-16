#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>
#include <cstdint>

// Forward declare zip_t to avoid including zip.h in header
typedef struct zip zip_t;

namespace Pathfinder {

    /**
     * @brief LRU cache manager for map data
     *
     * Caches JSON data from recently used maps to avoid
     * re-reading the ZIP archive each time.
     */
    class MapCache {
    public:
        explicit MapCache(size_t max_size = 20);

        // Gets map data from cache (returns "" if not found)
        std::string Get(int32_t map_id);

        // Adds or updates an entry in the cache
        void Put(int32_t map_id, const std::string& data);

        // Clears the entire cache
        void Clear();

        // Returns the current cache size
        size_t Size() const { return m_cache.size(); }

    private:
        size_t m_max_size;
        std::mutex m_mutex;

        // List to maintain LRU order (most recent = front)
        std::list<int32_t> m_lru_list;

        // Map: map_id -> (data, iterator in lru_list)
        struct CacheEntry {
            std::string data;
            std::list<int32_t>::iterator lru_it;
        };
        std::unordered_map<int32_t, CacheEntry> m_cache;
    };

    /**
     * @brief Map data loader from ZIP archive
     *
     * This class handles lazy loading of JSON data from a ZIP archive.
     * Data is loaded on demand and cached to improve performance.
     */
    class MapArchiveLoader {
    public:
        MapArchiveLoader();
        ~MapArchiveLoader();

        // Singleton
        static MapArchiveLoader& GetInstance();

        /**
         * @brief Initializes the loader with the archive path
         * @param archive_path Path to the maps.zip file
         * @return true if initialization succeeded
         */
        bool Initialize(const std::string& archive_path);

        /**
         * @brief Loads JSON data for a map
         * @param map_id ID of the map to load
         * @return JSON data for the map, or "" if not found
         */
        std::string LoadMapData(int32_t map_id);

        /**
         * @brief Checks if a map exists in the archive
         * @param map_id ID of the map to check
         * @return true if the map exists
         */
        bool HasMap(int32_t map_id) const;

        /**
         * @brief Gets the list of all available map IDs
         * @return Vector containing all map IDs
         */
        std::vector<int32_t> GetAvailableMapIds() const;

        /**
         * @brief Checks if the loader is initialized
         */
        bool IsInitialized() const { return m_initialized; }

        /**
         * @brief Clears the map cache
         */
        void ClearCache();

        // Disallow copying
        MapArchiveLoader(const MapArchiveLoader&) = delete;
        MapArchiveLoader& operator=(const MapArchiveLoader&) = delete;

    private:
        bool m_initialized;
        std::string m_archive_path;
        mutable std::mutex m_mutex;

        // LRU cache for map data
        std::unique_ptr<MapCache> m_cache;

        // List of available map IDs in the archive
        std::vector<int32_t> m_available_maps;

        /**
         * @brief Reads a file from the ZIP archive
         * @param filename Name of the file in the archive (e.g., "map_123.json")
         * @return File content, or "" on error
         */
        std::string ReadFileFromZip(const std::string& filename);

        /**
         * @brief Finds and reads a map file by its ID
         * @param map_id ID of the map to load
         * @return File content, or "" if not found
         */
        std::string FindAndReadMapFile(int32_t map_id);

        /**
         * @brief Reads a file from an already opened archive (without lock)
         * @param archive Already opened ZIP archive
         * @param filename Name of the file to read
         * @return File content, or "" on error
         */
        std::string ReadFileFromZipUnlocked(zip_t* archive, const std::string& filename);

        /**
         * @brief Scans the archive to find all available maps
         */
        void ScanArchive();
    };

} // namespace Pathfinder
