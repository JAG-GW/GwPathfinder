; Exemple d'utilisation de la DLL Pathfinding avec AutoIt
; Assurez-vous que GWPathfinder.dll et maps.zip sont dans le même dossier

#include <Array.au3>
#include <Math.au3>

; ============================================
; Configuration
; ============================================
Global Const $DLL_PATH = @ScriptDir & "\GWPathfinder.dll"

; ============================================
; Structures
; ============================================
Global Const $tagPathPoint = "float x;float y"
Global Const $tagPathResult = "ptr points;int point_count;float total_cost;int error_code;char error_message[256]"
Global Const $tagMapStats = "int trapezoid_count;int point_count;int teleport_count;int travel_portal_count;int npc_travel_count;int enter_travel_count;int error_code;char error_message[256]"

; ============================================
; Fonctions principales
; ============================================

; Initialise la DLL et charge maps.zip
Func InitializePathfinder()
    Local $result = DllCall($DLL_PATH, "int:cdecl", "Initialize")
    If @error Then
        ConsoleWrite("Erreur DllCall Initialize: " & @error & @CRLF)
        Return False
    EndIf
    If $result[0] = 0 Then
        ConsoleWrite("Erreur: Initialisation échouée (maps.zip manquant?)" & @CRLF)
        Return False
    EndIf
    ConsoleWrite("Pathfinder initialisé avec succès!" & @CRLF)
    Return True
EndFunc

; Ferme la DLL
Func ShutdownPathfinder()
    DllCall($DLL_PATH, "none:cdecl", "Shutdown")
    ConsoleWrite("Pathfinder arrêté." & @CRLF)
EndFunc

; Trouve un chemin entre deux points
Func FindPathGW($mapID, $startX, $startY, $destX, $destY, $simplifyRange = 0)
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "FindPath", _
        "int", $mapID, _
        "float", $startX, _
        "float", $startY, _
        "float", $destX, _
        "float", $destY, _
        "float", $simplifyRange)

    If @error Then
        ConsoleWrite("Erreur DllCall FindPath: " & @error & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Return $result[0]
EndFunc

; Libère la mémoire d'un PathResult
Func FreePathResult($pResult)
    DllCall($DLL_PATH, "none:cdecl", "FreePathResult", "ptr", $pResult)
EndFunc

; Vérifie si une map est disponible
Func IsMapAvailable($mapID)
    Local $result = DllCall($DLL_PATH, "int:cdecl", "IsMapAvailable", "int", $mapID)
    If @error Then Return False
    Return $result[0] = 1
EndFunc

; Obtient la liste de toutes les maps disponibles
Func GetAvailableMaps()
    Local $count = 0
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "GetAvailableMaps", "int*", $count)
    If @error Or $result[0] = 0 Then
        ConsoleWrite("Aucune map disponible ou erreur." & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Local $pMapList = $result[0]
    $count = $result[1]

    ; Lire la liste des IDs
    Local $mapIds[$count]
    For $i = 0 To $count - 1
        $mapIds[$i] = DllStructGetData(DllStructCreate("int", $pMapList + $i * 4), 1)
    Next

    ; Libérer la mémoire
    DllCall($DLL_PATH, "none:cdecl", "FreeMapList", "ptr", $pMapList)

    Return $mapIds
EndFunc

; Obtient les statistiques d'une map
Func GetMapStats($mapID)
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "GetMapStats", "int", $mapID)
    If @error Or $result[0] = 0 Then
        ConsoleWrite("Erreur lors de la récupération des stats de la map " & $mapID & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Local $pStats = $result[0]
    Local $stats = DllStructCreate($tagMapStats, $pStats)

    ; Copier les données
    Local $statsArray[7]
    $statsArray[0] = DllStructGetData($stats, "trapezoid_count")
    $statsArray[1] = DllStructGetData($stats, "point_count")
    $statsArray[2] = DllStructGetData($stats, "teleport_count")
    $statsArray[3] = DllStructGetData($stats, "travel_portal_count")
    $statsArray[4] = DllStructGetData($stats, "npc_travel_count")
    $statsArray[5] = DllStructGetData($stats, "enter_travel_count")
    $statsArray[6] = DllStructGetData($stats, "error_code")

    ; Libérer la mémoire
    DllCall($DLL_PATH, "none:cdecl", "FreeMapStats", "ptr", $pStats)

    Return $statsArray
EndFunc

; Affiche un chemin trouvé
Func DisplayPath($pResult)
    If $pResult = 0 Then
        ConsoleWrite("PathResult invalide" & @CRLF)
        Return
    EndIf

    Local $result = DllStructCreate($tagPathResult, $pResult)
    Local $errorCode = DllStructGetData($result, "error_code")

    If $errorCode <> 0 Then
        Local $errorMsg = DllStructGetData($result, "error_message")
        ConsoleWrite("Erreur pathfinding: " & $errorMsg & @CRLF)
        Return
    EndIf

    Local $pointCount = DllStructGetData($result, "point_count")
    Local $totalCost = DllStructGetData($result, "total_cost")
    Local $pPoints = DllStructGetData($result, "points")

    ConsoleWrite("Chemin trouvé!" & @CRLF)
    ConsoleWrite("  Nombre de points: " & $pointCount & @CRLF)
    ConsoleWrite("  Coût total: " & $totalCost & @CRLF)
    ConsoleWrite("  Points:" & @CRLF)

    For $i = 0 To $pointCount - 1
        Local $point = DllStructCreate($tagPathPoint, $pPoints + $i * 8)
        Local $x = DllStructGetData($point, "x")
        Local $y = DllStructGetData($point, "y")
        ConsoleWrite("    " & $i & ": (" & $x & ", " & $y & ")" & @CRLF)
    Next
EndFunc

; ============================================
; Tests
; ============================================

ConsoleWrite("=== Test de la DLL Pathfinding ===" & @CRLF & @CRLF)

; 1. Initialisation
ConsoleWrite("[1] Initialisation..." & @CRLF)
If Not InitializePathfinder() Then
    ConsoleWrite("ÉCHEC: Impossible d'initialiser!" & @CRLF)
    Exit
EndIf
ConsoleWrite(@CRLF)

; 2. Lister les maps disponibles
ConsoleWrite("[2] Liste des maps disponibles..." & @CRLF)
Local $maps = GetAvailableMaps()
If Not @error Then
    ConsoleWrite("Nombre de maps: " & UBound($maps) & @CRLF)
    ConsoleWrite("Premières maps: ")
    For $i = 0 To _Min(9, UBound($maps) - 1)
        ConsoleWrite($maps[$i] & " ")
    Next
    ConsoleWrite("..." & @CRLF)
Else
    ConsoleWrite("Erreur lors de la récupération des maps" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 3. Vérifier qu'une map spécifique est disponible
Local $testMapID = 7 ; Ascalon City
ConsoleWrite("[3] Vérification de la map " & $testMapID & "..." & @CRLF)
If IsMapAvailable($testMapID) Then
    ConsoleWrite("Map " & $testMapID & " est disponible!" & @CRLF)
Else
    ConsoleWrite("Map " & $testMapID & " n'est pas disponible." & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 4. Obtenir les stats de la map
ConsoleWrite("[4] Statistiques de la map " & $testMapID & "..." & @CRLF)
Local $stats = GetMapStats($testMapID)
If Not @error Then
    ConsoleWrite("  Trapézoides: " & $stats[0] & @CRLF)
    ConsoleWrite("  Points: " & $stats[1] & @CRLF)
    ConsoleWrite("  Téléporteurs: " & $stats[2] & @CRLF)
    ConsoleWrite("  Portails de voyage: " & $stats[3] & @CRLF)
    ConsoleWrite("  Voyages NPC: " & $stats[4] & @CRLF)
    ConsoleWrite("  Voyages Enter: " & $stats[5] & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 5. Trouver un chemin
ConsoleWrite("[5] Recherche d'un chemin sur la map " & $testMapID & "..." & @CRLF)
ConsoleWrite("De (-4258.8976, -5018.4462) à ( 4177.5174,  9114.5833) avec simplification = 1250" & @CRLF)

Local $pPath = FindPathGW($testMapID, -4258.8976, -5018.4462, 4177.5174,  9114.5833, 1250)
If Not @error And $pPath <> 0 Then
    DisplayPath($pPath)
    FreePathResult($pPath)
Else
    ConsoleWrite("Erreur lors de la recherche du chemin" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 6. Test avec une map invalide
ConsoleWrite("[6] Test avec une map invalide (ID 99999)..." & @CRLF)
Local $pInvalidPath = FindPathGW(99999, 0, 0, 100, 100, 0)
If Not @error And $pInvalidPath <> 0 Then
    DisplayPath($pInvalidPath)
    FreePathResult($pInvalidPath)
Else
    ConsoleWrite("Erreur attendue (normal)" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 7. Arrêt
ConsoleWrite("[7] Arrêt..." & @CRLF)
ShutdownPathfinder()

ConsoleWrite(@CRLF & "=== Tests terminés ===" & @CRLF)
