#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Pathfinder {

    /**
     * @brief Registre singleton pour le chargement des données de cartes
     *
     * Cette classe charge les données JSON des cartes à la demande depuis une archive ZIP.
     * Les données sont mises en cache automatiquement pour améliorer les performances.
     *
     * Configuration:
     * 1. Placer le fichier maps.zip dans le même répertoire que la DLL
     * 2. Le fichier doit contenir des fichiers nommés map_XXX.json (ex: map_7.json, map_123.json)
     */
    class MapDataRegistry {
    public:
        // Singleton
        static MapDataRegistry& GetInstance();

        /**
         * @brief Initialise le registre avec le chemin vers l'archive
         * @param archive_path Chemin vers maps.zip (optionnel, par défaut cherche dans le dossier de la DLL)
         * @return true si l'initialisation a réussi
         */
        bool Initialize(const std::string& archive_path = "");

        /**
         * @brief Retourne les données JSON d'une carte
         * @param map_id ID de la carte à charger
         * @return Les données JSON, ou "" si la carte n'existe pas
         */
        std::string GetMapData(int32_t map_id);

        /**
         * @brief Vérifie si une carte est disponible
         * @param map_id ID de la carte à vérifier
         * @return true si la carte existe
         */
        bool HasMap(int32_t map_id) const;

        /**
         * @brief Obtient la liste de tous les IDs de cartes disponibles
         * @return Vector contenant tous les IDs de cartes
         */
        std::vector<int32_t> GetAvailableMapIds() const;

        /**
         * @brief Vérifie si le registre est initialisé
         */
        bool IsInitialized() const;

        // Interdit la copie
        MapDataRegistry(const MapDataRegistry&) = delete;
        MapDataRegistry& operator=(const MapDataRegistry&) = delete;

    private:
        MapDataRegistry();
        ~MapDataRegistry() = default;

        /**
         * @brief Trouve le chemin par défaut de l'archive maps.zip
         * @return Le chemin vers maps.zip dans le dossier de la DLL
         */
        std::string GetDefaultArchivePath() const;
    };

} // namespace Pathfinder
