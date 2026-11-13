#!/bin/bash
# Animator Choreographer – Installer / Installer (DE/EN)
# Doppelklick genügt. Kopiert die App nach /Applications und das REAPER-Skript in den REAPER-Scripts-Ordner.
# Double-click to run. Copies the app to /Applications and the REAPER script to REAPER's Scripts folder.

cd "$(dirname "$0")" || exit 1
HERE="$PWD"

APP_NAME="Animator Choreographer.app"
LUA_NAME="Animator_export_regions_markers.lua"

APP_SRC="$HERE/$APP_NAME"
[ -d "$APP_SRC" ] || APP_SRC="$HERE/../build/Raumchoreographie_artefacts/Release/$APP_NAME"
LUA_SRC="$HERE/$LUA_NAME"
[ -f "$LUA_SRC" ] || LUA_SRC="$HERE/../reaper/$LUA_NAME"

REAPER_DIR="$HOME/Library/Application Support/REAPER"
LUA_DST_DIR="$REAPER_DIR/Scripts/Animator"

echo "=== Animator Choreographer: Installation / Installation ==="
echo

if [ ! -d "$APP_SRC" ]; then
  echo "FEHLER / ERROR: App nicht gefunden / app not found:"
  echo "  $APP_SRC"
  echo "Bitte zuerst bauen / build first:  cmake --build build --config Release --target Raumchoreographie"
  read -n 1 -s -r -p "Taste drücken / press any key"; exit 1
fi

# 1) App
echo "1) App nach /Applications / App to /Applications"
if [ -d "/Applications/$APP_NAME" ]; then
  osascript -e 'tell application "Animator Choreographer" to quit' >/dev/null 2>&1
  rm -rf "/Applications/$APP_NAME" 2>/dev/null || sudo rm -rf "/Applications/$APP_NAME"
fi
ditto "$APP_SRC" "/Applications/$APP_NAME" 2>/dev/null || sudo ditto "$APP_SRC" "/Applications/$APP_NAME"
xattr -dr com.apple.quarantine "/Applications/$APP_NAME" 2>/dev/null
codesign --force --deep -s - "/Applications/$APP_NAME" >/dev/null 2>&1
echo "   OK"

# 2) REAPER-Skript
echo "2) REAPER-Skript / REAPER script"
if [ -f "$LUA_SRC" ]; then
  mkdir -p "$LUA_DST_DIR"
  cp "$LUA_SRC" "$LUA_DST_DIR/$LUA_NAME"
  echo "   OK  ->  $LUA_DST_DIR/$LUA_NAME"
else
  echo "   Übersprungen / skipped (Skript nicht gefunden / script not found)"
fi

echo
echo "Fertig / Done."
echo
echo "Nächster Schritt in REAPER / Next step in REAPER:"
echo "  Actions > Show action list > New action > Load ReaScript…"
echo "  und / and choose:  $LUA_DST_DIR/$LUA_NAME"
echo
echo "REAPER portable? Dann die .lua-Datei von Hand in den Scripts-Ordner dieser Installation kopieren."
echo "REAPER portable? Copy the .lua file by hand into that install's Scripts folder."
echo
read -r -p "App jetzt starten? / Open the app now? [j/y/N] " A
case "$A" in j|J|y|Y) open "/Applications/$APP_NAME";; esac
