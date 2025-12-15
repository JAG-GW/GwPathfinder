cmake_minimum_required(VERSION 3.16)
project(GWPathfinder VERSION 1.0.0 LANGUAGES CXX)

# Forcer le linkage statique avec vcpkg
set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Configuration C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources de la DLL (nouveau système avec archive ZIP)
set(PATHFINDER_SOURCES
    PathfinderAPI.cpp
    PathfinderCore.cpp
    MapDataRegistry.cpp
    MapArchiveLoader.cpp
)

set(PATHFINDER_HEADERS
    PathfinderAPI.h
    PathfinderCore.h
    MapDataRegistry.h
    MapArchiveLoader.h
)

# Créer la DLL
add_library(GWPathfinder SHARED
    ${PATHFINDER_SOURCES}
    ${PATHFINDER_HEADERS}
)

# Définir le symbole d'export
target_compile_definitions(GWPathfinder PRIVATE PATHFINDER_EXPORTS)

# Trouver et lier les dépendances
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)

target_link_libraries(GWPathfinder PRIVATE
    nlohmann_json::nlohmann_json
    libzip::zip
)

# Lier shlwapi.lib pour PathRemoveFileSpec (Windows)
if(WIN32)
    target_link_libraries(GWPathfinder PRIVATE shlwapi)
endif()

# Options de compilation
if(MSVC)
    target_compile_options(GWPathfinder PRIVATE
        /W4                # Warning level 4
        /WX-               # Don't treat warnings as errors
        /permissive-       # Standards conformance
        /MP                # Multi-processor compilation
    )

    # Optimisations pour Release
    target_compile_options(GWPathfinder PRIVATE
        $<$<CONFIG:Release>:/O2>    # Maximum optimization
        $<$<CONFIG:Release>:/Oi>    # Enable intrinsics
        $<$<CONFIG:Release>:/GL>    # Whole program optimization
    )

    # Link-time optimizations pour Release
    set_target_properties(GWPathfinder PROPERTIES
        LINK_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF"
    )

    # Options de linker
    target_link_options(GWPathfinder PRIVATE
        /LARGEADDRESSAWARE      # Permet d'utiliser plus de 2GB en Win32
        /INCREMENTAL:NO         # Désactive le lien incrémental
    )
else()
    target_compile_options(GWPathfinder PRIVATE
        -Wall
        -Wextra
        -pedantic
    )
endif()

# Installer la DLL
install(TARGETS GWPathfinder
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Installer les headers (pour les utilisateurs C++)
install(FILES PathfinderAPI.h
    DESTINATION include/GWPathfinder
)

# Copier maps.zip dans le dossier de sortie après compilation
add_custom_command(TARGET GWPathfinder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Checking for maps.zip..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/maps.zip
        $<TARGET_FILE_DIR:GWPathfinder>/maps.zip
    COMMENT "Copying maps.zip to output directory"
)

# Générer un fichier .def pour les exports (optionnel)
if(MSVC)
    set_target_properties(GWPathfinder PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF  # Utiliser __declspec(dllexport)
    )
endif()

# Information de build
message(STATUS "========================================")
message(STATUS "GWPathfinder Configuration (ZIP Archive System)")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Archive Loading: ENABLED")
message(STATUS "  Source files: ${PATHFINDER_SOURCES}")
message(STATUS "========================================")
message(STATUS "NOTE: Make sure maps.zip exists in this directory!")
message(STATUS "      Run ConvertRarToZip.ps1 to create it from maps.rar")
message(STATUS "========================================")
