#include "MapArchiveLoader.h"
#include <zip.h>
#include <algorithm>
#include <sstream>
#include <cstring>

namespace Pathfinder {

    // ==================== MapCache Implementation ====================

    MapCache::MapCache(size_t max_size)
        : m_max_size(max_size) {
    }

    std::string MapCache::Get(int32_t map_id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(map_id);
        if (it == m_cache.end()) {
            return "";
        }

        // Move this element to the front of the LRU list
        m_lru_list.erase(it->second.lru_it);
        m_lru_list.push_front(map_id);
        it->second.lru_it = m_lru_list.begin();

        return it->second.data;
    }

    void MapCache::Put(int32_t map_id, const std::string& data) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(map_id);
        if (it != m_cache.end()) {
            // Update existing entry
            m_lru_list.erase(it->second.lru_it);
            m_lru_list.push_front(map_id);
            it->second.data = data;
            it->second.lru_it = m_lru_list.begin();
        }
        else {
            // New entry
            if (m_cache.size() >= m_max_size) {
                // Remove the least recently used element
                int32_t old_id = m_lru_list.back();
                m_lru_list.pop_back();
                m_cache.erase(old_id);
            }

            m_lru_list.push_front(map_id);
            CacheEntry entry;
            entry.data = data;
            entry.lru_it = m_lru_list.begin();
            m_cache[map_id] = entry;
        }
    }

    void MapCache::Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
        m_lru_list.clear();
    }

    // ==================== MapArchiveLoader Implementation ====================

    MapArchiveLoader::MapArchiveLoader()
        : m_initialized(false)
        , m_cache(std::make_unique<MapCache>(20)) {
    }

    MapArchiveLoader::~MapArchiveLoader() {
    }

    MapArchiveLoader& MapArchiveLoader::GetInstance() {
        static MapArchiveLoader instance;
        return instance;
    }

    bool MapArchiveLoader::Initialize(const std::string& archive_path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_initialized) {
            return true;
        }

        m_archive_path = archive_path;

        // Verify that the archive exists and is readable
        int error_code = 0;
        zip_t* archive = zip_open(archive_path.c_str(), ZIP_RDONLY, &error_code);
        if (!archive) {
            return false;
        }
        zip_close(archive);

        // Scan the archive to find all available maps
        ScanArchive();

        m_initialized = true;
        return true;
    }

    std::string MapArchiveLoader::LoadMapData(int32_t map_id) {
        if (!m_initialized) {
            return "";
        }

        // Check the cache first
        std::string cached = m_cache->Get(map_id);
        if (!cached.empty()) {
            return cached;
        }

        // Find the file corresponding to map_id in the archive
        // Files are named like: "100_Prophecies_Kryta_...json"
        std::string data = FindAndReadMapFile(map_id);
        if (!data.empty()) {
            // Add to cache
            m_cache->Put(map_id, data);
        }

        return data;
    }

    bool MapArchiveLoader::HasMap(int32_t map_id) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::find(m_available_maps.begin(), m_available_maps.end(), map_id) != m_available_maps.end();
    }

    std::vector<int32_t> MapArchiveLoader::GetAvailableMapIds() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available_maps;
    }

    void MapArchiveLoader::ClearCache() {
        m_cache->Clear();
    }

    std::string MapArchiveLoader::ReadFileFromZip(const std::string& filename) {
        std::lock_guard<std::mutex> lock(m_mutex);

        int error_code = 0;
        zip_t* archive = zip_open(m_archive_path.c_str(), ZIP_RDONLY, &error_code);
        if (!archive) {
            return "";
        }

        // Find the file in the archive
        zip_stat_t stat;
        if (zip_stat(archive, filename.c_str(), 0, &stat) != 0) {
            zip_close(archive);
            return "";
        }

        // Open the file
        zip_file_t* file = zip_fopen(archive, filename.c_str(), 0);
        if (!file) {
            zip_close(archive);
            return "";
        }

        // Read the content
        std::string content;
        content.resize(stat.size);

        zip_int64_t bytes_read = zip_fread(file, &content[0], stat.size);
        if (bytes_read < 0) {
            zip_fclose(file);
            zip_close(archive);
            return "";
        }

        content.resize(bytes_read);

        zip_fclose(file);
        zip_close(archive);

        return content;
    }

    std::string MapArchiveLoader::FindAndReadMapFile(int32_t map_id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        int error_code = 0;
        zip_t* archive = zip_open(m_archive_path.c_str(), ZIP_RDONLY, &error_code);
        if (!archive) {
            return "";
        }

        // Search for a file that starts with "map_id_"
        std::string prefix = std::to_string(map_id) + "_";
        zip_int64_t num_entries = zip_get_num_entries(archive, 0);

        for (zip_int64_t i = 0; i < num_entries; ++i) {
            const char* name = zip_get_name(archive, i, 0);
            if (!name) continue;

            std::string filename(name);
            // Check if the file starts with map_id and ends with .json
            if (filename.find(prefix) == 0 && filename.find(".json") != std::string::npos) {
                // File found, read it
                std::string content = ReadFileFromZipUnlocked(archive, filename);
                zip_close(archive);
                return content;
            }
        }

        zip_close(archive);
        return "";
    }

    std::string MapArchiveLoader::ReadFileFromZipUnlocked(zip_t* archive, const std::string& filename) {
        // Find the file in the archive
        zip_stat_t stat;
        if (zip_stat(archive, filename.c_str(), 0, &stat) != 0) {
            return "";
        }

        // Open the file
        zip_file_t* file = zip_fopen(archive, filename.c_str(), 0);
        if (!file) {
            return "";
        }

        // Read the content
        std::string content;
        content.resize(stat.size);

        zip_int64_t bytes_read = zip_fread(file, &content[0], stat.size);
        if (bytes_read < 0) {
            zip_fclose(file);
            return "";
        }

        content.resize(bytes_read);
        zip_fclose(file);

        return content;
    }

    void MapArchiveLoader::ScanArchive() {
        m_available_maps.clear();

        int error_code = 0;
        zip_t* archive = zip_open(m_archive_path.c_str(), ZIP_RDONLY, &error_code);
        if (!archive) {
            return;
        }

        zip_int64_t num_entries = zip_get_num_entries(archive, 0);
        for (zip_int64_t i = 0; i < num_entries; ++i) {
            const char* name = zip_get_name(archive, i, 0);
            if (!name) continue;

            // Files are named: "100_Prophecies_Kryta_...json"
            // Extract the ID from the beginning of the filename
            std::string filename(name);
            if (filename.find(".json") != std::string::npos) {
                // Find the first underscore
                size_t first_underscore = filename.find('_');
                if (first_underscore != std::string::npos && first_underscore > 0) {
                    std::string id_str = filename.substr(0, first_underscore);
                    try {
                        int32_t map_id = std::stoi(id_str);
                        m_available_maps.push_back(map_id);
                    }
                    catch (...) {
                        // Ignore files with invalid names
                    }
                }
            }
        }

        zip_close(archive);

        // Sort IDs for easier searching
        std::sort(m_available_maps.begin(), m_available_maps.end());
    }

} // namespace Pathfinder
