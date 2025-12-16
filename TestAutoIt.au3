; GWPathfinder DLL usage example with AutoIt
; Make sure GWPathfinder.dll and maps.zip are in the same folder

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
Global Const $tagObstacleZone = "float x;float y;float radius"

; ============================================
; Main Functions
; ============================================

; Initialize the DLL and load maps.zip
Func InitializePathfinder()
    Local $result = DllCall($DLL_PATH, "int:cdecl", "Initialize")
    If @error Then
        ConsoleWrite("DllCall Initialize error: " & @error & @CRLF)
        Return False
    EndIf
    If $result[0] = 0 Then
        ConsoleWrite("Error: Initialization failed (maps.zip missing?)" & @CRLF)
        Return False
    EndIf
    ConsoleWrite("Pathfinder initialized successfully!" & @CRLF)
    Return True
EndFunc

; Shutdown the DLL
Func ShutdownPathfinder()
    DllCall($DLL_PATH, "none:cdecl", "Shutdown")
    ConsoleWrite("Pathfinder shut down." & @CRLF)
EndFunc

; Get DLL version string
Func GetPathfinderVersion()
    Local $result = DllCall($DLL_PATH, "str:cdecl", "GetPathfinderVersion")
    If @error Then
        ConsoleWrite("DllCall GetPathfinderVersion error: " & @error & @CRLF)
        Return ""
    EndIf
    Return $result[0]
EndFunc

; Load a map from an external JSON file
Func LoadMapFromFile($mapID, $filePath)
    Local $result = DllCall($DLL_PATH, "int:cdecl", "LoadMapFromFile", "int", $mapID, "str", $filePath)
    If @error Then
        ConsoleWrite("DllCall LoadMapFromFile error: " & @error & @CRLF)
        Return False
    EndIf
    Return $result[0] = 1
EndFunc

; Find a path between two points
; $obstacles can be:
;   - 0 or Default: no obstacles
;   - 2D array: [[x1, y1, radius1], [x2, y2, radius2], ...]
Func FindPathGW($mapID, $startX, $startY, $destX, $destY, $obstacles = 0, $simplifyRange = 0)
    ; Check if obstacles are provided
    If IsArray($obstacles) And UBound($obstacles) > 0 Then
        ; Use FindPathWithObstacles
        Return _FindPathWithObstacles($mapID, $startX, $startY, $destX, $destY, $obstacles, $simplifyRange)
    EndIf

    ; No obstacles, use standard FindPath
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "FindPath", _
        "int", $mapID, _
        "float", $startX, _
        "float", $startY, _
        "float", $destX, _
        "float", $destY, _
        "float", $simplifyRange)

    If @error Then
        ConsoleWrite("DllCall FindPath error: " & @error & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Return $result[0]
EndFunc

; Internal function to find path with obstacles
Func _FindPathWithObstacles($mapID, $startX, $startY, $destX, $destY, $obstacles, $simplifyRange)
    Local $obstacleCount = UBound($obstacles)

    ; Create a contiguous array of ObstacleZone structures in memory
    ; Each ObstacleZone is 12 bytes (3 floats: x, y, radius)
    Local $obstacleStructSize = 12
    Local $obstacleBuffer = DllStructCreate("byte[" & ($obstacleCount * $obstacleStructSize) & "]")
    Local $pObstacles = DllStructGetPtr($obstacleBuffer)

    ; Fill the obstacle buffer
    For $i = 0 To $obstacleCount - 1
        Local $obstacle = DllStructCreate($tagObstacleZone, $pObstacles + $i * $obstacleStructSize)
        DllStructSetData($obstacle, "x", $obstacles[$i][0])
        DllStructSetData($obstacle, "y", $obstacles[$i][1])
        DllStructSetData($obstacle, "radius", $obstacles[$i][2])
    Next

    ; Call FindPathWithObstacles
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "FindPathWithObstacles", _
        "int", $mapID, _
        "float", $startX, _
        "float", $startY, _
        "float", $destX, _
        "float", $destY, _
        "ptr", $pObstacles, _
        "int", $obstacleCount, _
        "float", $simplifyRange)

    If @error Then
        ConsoleWrite("DllCall FindPathWithObstacles error: " & @error & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Return $result[0]
EndFunc

; Free PathResult memory
Func FreePathResult($pResult)
    DllCall($DLL_PATH, "none:cdecl", "FreePathResult", "ptr", $pResult)
EndFunc

; Check if a map is available
Func IsMapAvailable($mapID)
    Local $result = DllCall($DLL_PATH, "int:cdecl", "IsMapAvailable", "int", $mapID)
    If @error Then Return False
    Return $result[0] = 1
EndFunc

; Get list of all available maps
Func GetAvailableMaps()
    Local $count = 0
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "GetAvailableMaps", "int*", $count)
    If @error Or $result[0] = 0 Then
        ConsoleWrite("No maps available or error." & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Local $pMapList = $result[0]
    $count = $result[1]

    ; Read the list of IDs
    Local $mapIds[$count]
    For $i = 0 To $count - 1
        $mapIds[$i] = DllStructGetData(DllStructCreate("int", $pMapList + $i * 4), 1)
    Next

    ; Free memory
    DllCall($DLL_PATH, "none:cdecl", "FreeMapList", "ptr", $pMapList)

    Return $mapIds
EndFunc

; Get map statistics
Func GetMapStats($mapID)
    Local $result = DllCall($DLL_PATH, "ptr:cdecl", "GetMapStats", "int", $mapID)
    If @error Or $result[0] = 0 Then
        ConsoleWrite("Error retrieving stats for map " & $mapID & @CRLF)
        Return SetError(1, 0, 0)
    EndIf

    Local $pStats = $result[0]
    Local $stats = DllStructCreate($tagMapStats, $pStats)

    ; Copy data
    Local $statsArray[7]
    $statsArray[0] = DllStructGetData($stats, "trapezoid_count")
    $statsArray[1] = DllStructGetData($stats, "point_count")
    $statsArray[2] = DllStructGetData($stats, "teleport_count")
    $statsArray[3] = DllStructGetData($stats, "travel_portal_count")
    $statsArray[4] = DllStructGetData($stats, "npc_travel_count")
    $statsArray[5] = DllStructGetData($stats, "enter_travel_count")
    $statsArray[6] = DllStructGetData($stats, "error_code")

    ; Free memory
    DllCall($DLL_PATH, "none:cdecl", "FreeMapStats", "ptr", $pStats)

    Return $statsArray
EndFunc

; Display a found path
Func DisplayPath($pResult)
    If $pResult = 0 Then
        ConsoleWrite("Invalid PathResult" & @CRLF)
        Return
    EndIf

    Local $result = DllStructCreate($tagPathResult, $pResult)
    Local $errorCode = DllStructGetData($result, "error_code")

    If $errorCode <> 0 Then
        Local $errorMsg = DllStructGetData($result, "error_message")
        ConsoleWrite("Pathfinding error: " & $errorMsg & @CRLF)
        Return
    EndIf

    Local $pointCount = DllStructGetData($result, "point_count")
    Local $totalCost = DllStructGetData($result, "total_cost")
    Local $pPoints = DllStructGetData($result, "points")

    ConsoleWrite("Path found!" & @CRLF)
    ConsoleWrite("  Point count: " & $pointCount & @CRLF)
    ConsoleWrite("  Total cost: " & $totalCost & @CRLF)
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

ConsoleWrite("=== GWPathfinder DLL Test ===" & @CRLF & @CRLF)

; 1. Get DLL version
ConsoleWrite("[1] Getting DLL version..." & @CRLF)
Local $version = GetPathfinderVersion()
If $version <> "" Then
    ConsoleWrite("DLL Version: " & $version & @CRLF)
Else
    ConsoleWrite("Could not get version" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 2. Initialize
ConsoleWrite("[2] Initializing..." & @CRLF)
If Not InitializePathfinder() Then
    ConsoleWrite("FAILED: Could not initialize!" & @CRLF)
    Exit
EndIf
ConsoleWrite(@CRLF)

; 3. List available maps
ConsoleWrite("[3] Listing available maps..." & @CRLF)
Local $maps = GetAvailableMaps()
If Not @error Then
    ConsoleWrite("Number of maps: " & UBound($maps) & @CRLF)
    ConsoleWrite("First maps: ")
    For $i = 0 To _Min(9, UBound($maps) - 1)
        ConsoleWrite($maps[$i] & " ")
    Next
    ConsoleWrite("..." & @CRLF)
Else
    ConsoleWrite("Error retrieving maps" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 4. Check if a specific map is available
Local $testMapID = 7 ; Ascalon City
ConsoleWrite("[4] Checking map " & $testMapID & "..." & @CRLF)
If IsMapAvailable($testMapID) Then
    ConsoleWrite("Map " & $testMapID & " is available!" & @CRLF)
Else
    ConsoleWrite("Map " & $testMapID & " is not available." & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 5. Get map stats
ConsoleWrite("[5] Getting stats for map " & $testMapID & "..." & @CRLF)
Local $stats = GetMapStats($testMapID)
If Not @error Then
    ConsoleWrite("  Trapezoids: " & $stats[0] & @CRLF)
    ConsoleWrite("  Points: " & $stats[1] & @CRLF)
    ConsoleWrite("  Teleporters: " & $stats[2] & @CRLF)
    ConsoleWrite("  Travel portals: " & $stats[3] & @CRLF)
    ConsoleWrite("  NPC travels: " & $stats[4] & @CRLF)
    ConsoleWrite("  Enter travels: " & $stats[5] & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 6. Find a path (without obstacles)
ConsoleWrite("[6] Finding path on map " & $testMapID & " (no obstacles)..." & @CRLF)
ConsoleWrite("From (-4258.8976, -5018.4462) to (4177.5174, 9114.5833) with simplification = 1250" & @CRLF)

Local $pPath = FindPathGW($testMapID, -4258.8976, -5018.4462, 4177.5174, 9114.5833, 0, 1250)
If Not @error And $pPath <> 0 Then
    DisplayPath($pPath)
    FreePathResult($pPath)
Else
    ConsoleWrite("Error finding path" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 6b. Find a path WITH obstacles
ConsoleWrite("[6b] Finding path on map " & $testMapID & " (WITH obstacles)..." & @CRLF)
ConsoleWrite("From (-4258.8976, -5018.4462) to (4177.5174, 9114.5833) with simplification = 1250" & @CRLF)
ConsoleWrite("Obstacles: [125, 978, 50], [-1578, 9875, 75]" & @CRLF)

; Define obstacles as 2D array: [[x, y, radius], ...]
Local $aObstacles[2][3] = [ _
    [125, 978, 50], _
    [-1578, 9875, 75] _
]

Local $pPathWithObstacles = FindPathGW($testMapID, -4258.8976, -5018.4462, 4177.5174, 9114.5833, $aObstacles, 1250)
If Not @error And $pPathWithObstacles <> 0 Then
    DisplayPath($pPathWithObstacles)
    FreePathResult($pPathWithObstacles)
Else
    ConsoleWrite("Error finding path with obstacles" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 7. Test with invalid map
ConsoleWrite("[7] Testing with invalid map (ID 99999)..." & @CRLF)
Local $pInvalidPath = FindPathGW(99999, 0, 0, 100, 100, 0)
If Not @error And $pInvalidPath <> 0 Then
    DisplayPath($pInvalidPath)
    FreePathResult($pInvalidPath)
Else
    ConsoleWrite("Expected error (normal)" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 8. Test LoadMapFromFile (example - file must exist)
ConsoleWrite("[8] Testing LoadMapFromFile..." & @CRLF)
Local $testJsonPath = @ScriptDir & "\test_map.json"
If FileExists($testJsonPath) Then
    If LoadMapFromFile(99998, $testJsonPath) Then
        ConsoleWrite("Successfully loaded map from external file!" & @CRLF)
    Else
        ConsoleWrite("Failed to load map from file" & @CRLF)
    EndIf
Else
    ConsoleWrite("Skipped (test_map.json not found - this is normal)" & @CRLF)
EndIf
ConsoleWrite(@CRLF)

; 9. Shutdown
ConsoleWrite("[9] Shutting down..." & @CRLF)
ShutdownPathfinder()

ConsoleWrite(@CRLF & "=== Tests completed ===" & @CRLF)
