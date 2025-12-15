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
     * @brief Gestionnaire de cache LRU pour les données de maps
     *
     * Cache les données JSON des maps récemment utilisées pour éviter
     * de relire l'archive ZIP à chaque fois.
     */
    class MapCache {
    public:
        explicit MapCache(size_t max_size = 20);

        // Récupère les données d'une map depuis le cache (retourne "" si absent)
        std::string Get(int32_t map_id);

        // Ajoute ou met à jour une entrée dans le cache
        void Put(int32_t map_id, const std::string& data);

        // Vide complètement le cache
        void Clear();

        // Retourne la taille actuelle du cache
        size_t Size() const { return m_cache.size(); }

    private:
        size_t m_max_size;
        std::mutex m_mutex;

        // Liste pour maintenir l'ordre LRU (most recent = front)
        std::list<int32_t> m_lru_list;

        // Map: map_id -> (data, iterator dans lru_list)
        struct CacheEntry {
            std::string data;
            std::list<int32_t>::iterator lru_it;
        };
        std::unordered_map<int32_t, CacheEntry> m_cache;
    };

    /**
     * @brief Chargeur de données de maps depuis une archive ZIP
     *
     * Cette classe gère le chargement lazy des données JSON depuis une archive ZIP.
     * Les données sont chargées à la demande et mises en cache pour améliorer les performances.
     */
    class MapArchiveLoader {
    public:
        MapArchiveLoader();
        ~MapArchiveLoader();

        // Singleton
        static MapArchiveLoader& GetInstance();

        /**
         * @brief Initialise le chargeur avec le chemin vers l'archive
         * @param archive_path Chemin vers le fichier maps.zip
         * @return true si l'initialisation a réussi
         */
        bool Initialize(const std::string& archive_path);

        /**
         * @brief Charge les données JSON d'une map
         * @param map_id ID de la map à charger
         * @return Les données JSON de la map, ou "" si non trouvée
         */
        std::string LoadMapData(int32_t map_id);

        /**
         * @brief Vérifie si une map existe dans l'archive
         * @param map_id ID de la map à vérifier
         * @return true si la map existe
         */
        bool HasMap(int32_t map_id) const;

        /**
         * @brief Obtient la liste de tous les IDs de maps disponibles
         * @return Vector contenant tous les IDs de maps
         */
        std::vector<int32_t> GetAvailableMapIds() const;

        /**
         * @brief Vérifie si le chargeur est initialisé
         */
        bool IsInitialized() const { return m_initialized; }

        /**
         * @brief Vide le cache de maps
         */
        void ClearCache();

        // Interdit la copie
        MapArchiveLoader(const MapArchiveLoader&) = delete;
        MapArchiveLoader& operator=(const MapArchiveLoader&) = delete;

    private:
        bool m_initialized;
        std::string m_archive_path;
        mutable std::mutex m_mutex;

        // Cache LRU pour les données de maps
        std::unique_ptr<MapCache> m_cache;

        // Liste des IDs de maps disponibles dans l'archive
        std::vector<int32_t> m_available_maps;

        /**
         * @brief Lit un fichier depuis l'archive ZIP
         * @param filename Nom du fichier dans l'archive (ex: "map_123.json")
         * @return Le contenu du fichier, ou "" si erreur
         */
        std::string ReadFileFromZip(const std::string& filename);

        /**
         * @brief Cherche et lit un fichier de map par son ID
         * @param map_id ID de la map à charger
         * @return Le contenu du fichier, ou "" si non trouvé
         */
        std::string FindAndReadMapFile(int32_t map_id);

        /**
         * @brief Lit un fichier depuis une archive déjà ouverte (sans lock)
         * @param archive Archive ZIP déjà ouverte
         * @param filename Nom du fichier à lire
         * @return Le contenu du fichier, ou "" si erreur
         */
        std::string ReadFileFromZipUnlocked(zip_t* archive, const std::string& filename);

        /**
         * @brief Scanne l'archive pour trouver toutes les maps disponibles
         */
        void ScanArchive();
    };

} // namespace Pathfinder
