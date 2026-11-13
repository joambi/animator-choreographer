#!/bin/bash
# Animator Choreographer – Deinstallation / Uninstall (DE/EN)
APP_NAME="Animator Choreographer.app"
LUA_DST_DIR="$HOME/Library/Application Support/REAPER/Scripts/Animator"
echo "=== Animator Choreographer entfernen / remove ==="
osascript -e 'tell application "Animator Choreographer" to quit' >/dev/null 2>&1
[ -d "/Applications/$APP_NAME" ] && { rm -rf "/Applications/$APP_NAME" 2>/dev/null || sudo rm -rf "/Applications/$APP_NAME"; echo "App entfernt / app removed"; }
[ -f "$LUA_DST_DIR/Animator_export_regions_markers.lua" ] && { rm -f "$LUA_DST_DIR/Animator_export_regions_markers.lua"; rmdir "$LUA_DST_DIR" 2>/dev/null; echo "Skript entfernt / script removed"; }
echo "Hinweis / Note: In REAPER bleibt ein Eintrag in der Action List, bis du ihn dort löschst. / A leftover entry stays in REAPER's action list until you remove it there."
read -n 1 -s -r -p "Taste drücken / press any key"
