#!/bin/bash
# Baut ein DMG mit App, REAPER-Skript und Installer. / Builds a DMG with app, REAPER script and installer.
# Aufruf / Usage:  ./installer/make-dmg.sh [version]
set -e
cd "$(dirname "$0")/.."
VERSION="${1:-0.1.0}"
APP="build/Raumchoreographie_artefacts/Release/Animator Choreographer.app"
[ -d "$APP" ] || { echo "App fehlt / app missing: $APP"; echo "Erst bauen / build first."; exit 1; }

STAGE="$(mktemp -d)/Animator Choreographer"
mkdir -p "$STAGE" dist
ditto "$APP" "$STAGE/Animator Choreographer.app"
codesign --force --deep -s - "$STAGE/Animator Choreographer.app"
cp reaper/Animator_export_regions_markers.lua "$STAGE/"
cp installer/install.command installer/uninstall.command "$STAGE/"
cp installer/INSTALL.txt "$STAGE/INSTALL.txt"
chmod +x "$STAGE/install.command" "$STAGE/uninstall.command"

OUT="dist/AnimatorChoreographer-$VERSION.dmg"
rm -f "$OUT"
hdiutil create -volname "Animator Choreographer $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$OUT"
echo "Fertig / Done: $OUT"
