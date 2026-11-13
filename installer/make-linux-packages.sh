#!/bin/sh
# Linux-Pakete fuer den Animator Choreographer: .tar.gz und .deb (x86_64 / amd64).
# Aufruf / Usage:  installer/make-linux-packages.sh [BUILD_DIR] [OUT_DIR]
#   BUILD_DIR  CMake-Build-Ordner (Standard: build)
#   OUT_DIR    Zielordner (Standard: dist)
# Voraussetzung: cmake --build BUILD_DIR --config Release --target Raumchoreographie
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-$ROOT/build}"
OUT="${2:-$ROOT/dist}"
NAME=animator-choreographer
VER="$(sed -n 's/^project(Raumchoreographie VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
[ -n "$VER" ] || { echo "Version nicht gefunden in CMakeLists.txt" >&2; exit 1; }

BIN="$(find "$BUILD" -type f -path '*Raumchoreographie_artefacts*' -name 'Animator Choreographer' -perm -u+x | head -1)"
[ -n "$BIN" ] || { echo "App nicht gefunden unter $BUILD (zuerst bauen)" >&2; exit 1; }

mkdir -p "$OUT"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

DESKTOP="[Desktop Entry]
Type=Application
Name=Animator Choreographer
Comment=Scene XML for the ICST Animator (AmbiEncoder_64)
Comment[de]=Szenen-XML für den ICST Animator (AmbiEncoder_64)
Exec=$NAME %f
Icon=$NAME
Terminal=false
Categories=AudioVideo;Audio;
"

# ---------- .tar.gz ----------
T="$WORK/$NAME-$VER-linux-x86_64"
mkdir -p "$T"
install -m 755 "$BIN" "$T/$NAME"
strip --strip-unneeded "$T/$NAME"
install -m 644 "$ROOT/reaper/Animator_export_regions_markers.lua" "$T/"
install -m 644 "$ROOT/Resources/Icon/AnimatorChoreographer-icon.png" "$T/$NAME.png"
install -m 644 "$ROOT/LICENSE" "$T/LICENSE"
printf '%s' "$DESKTOP" > "$T/$NAME.desktop"
cat > "$T/INSTALL.txt" <<TXT
ANIMATOR CHOREOGRAPHER $VER – Linux (x86_64)

DE
1. Starten: ./$NAME
2. Optional ins Menü: $NAME.desktop nach ~/.local/share/applications/ kopieren,
   Exec= auf den vollen Pfad von $NAME setzen, $NAME.png nach
   ~/.local/share/icons/hicolor/1024x1024/apps/ kopieren.
3. REAPER-Skript: Animator_export_regions_markers.lua nach
   ~/.config/REAPER/Scripts/Animator/ kopieren, dann in REAPER
   Actions > Show action list > New action > Load ReaScript…
Benötigt: X11 (oder XWayland), OpenGL, FreeType, Fontconfig.

EN
1. Run: ./$NAME
2. Optional menu entry: copy $NAME.desktop to ~/.local/share/applications/,
   set Exec= to the full path of $NAME, copy $NAME.png to
   ~/.local/share/icons/hicolor/1024x1024/apps/.
3. REAPER script: copy Animator_export_regions_markers.lua to
   ~/.config/REAPER/Scripts/Animator/, then in REAPER
   Actions > Show action list > New action > Load ReaScript…
Requires: X11 (or XWayland), OpenGL, FreeType, Fontconfig.
TXT
tar -C "$WORK" --owner=0 --group=0 -czf "$OUT/$NAME-$VER-linux-x86_64.tar.gz" "$NAME-$VER-linux-x86_64"

# ---------- .deb ----------
D="$WORK/deb"
mkdir -p "$D/DEBIAN" "$D/usr/bin" "$D/usr/share/applications" \
         "$D/usr/share/icons/hicolor/1024x1024/apps" \
         "$D/usr/share/$NAME" "$D/usr/share/doc/$NAME"
install -m 755 "$BIN" "$D/usr/bin/$NAME"
strip --strip-unneeded "$D/usr/bin/$NAME"
printf '%s' "$DESKTOP" > "$D/usr/share/applications/$NAME.desktop"
install -m 644 "$ROOT/Resources/Icon/AnimatorChoreographer-icon.png" "$D/usr/share/icons/hicolor/1024x1024/apps/$NAME.png"
install -m 644 "$ROOT/reaper/Animator_export_regions_markers.lua" "$D/usr/share/$NAME/"
install -m 644 "$ROOT/LICENSE" "$D/usr/share/doc/$NAME/copyright"
SIZE="$(du -sk "$D/usr" | cut -f1)"
# Mindestversion von glibc aus den gebundenen Symbolen (abhaengig vom Build-System)
GLIBC="$(objdump -T "$BIN" | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -1)"
# JUCE laedt die X11-Erweiterungen (Xrandr, Xinerama, Xcursor, Xext) zur Laufzeit per dlopen.
cat > "$D/DEBIAN/control" <<CTL
Package: $NAME
Version: $VER
Section: sound
Priority: optional
Architecture: amd64
Installed-Size: $SIZE
Depends: libc6 (>= $GLIBC), libstdc++6, libgcc-s1, libgl1, libfreetype6, libfontconfig1, libx11-6, libxext6, libxrandr2, libxinerama1, libxcursor1
Maintainer: Johannes Schütt <johannes.schuett@zhdk.ch>
Homepage: https://ambisonics.ch/icst-ambisonics-plugins/20_animator_choreographer/
Description: Scene XML for the ICST Animator (AmbiEncoder_64)
 Plans how groups of sources move through space over time and writes the
 scene XML that the Animator in the ICST AmbiEncoder_64 imports and plays.
 The REAPER script Animator_export_regions_markers.lua is installed to
 /usr/share/$NAME/; copy it to ~/.config/REAPER/Scripts/Animator/.
CTL
dpkg-deb --build --root-owner-group "$D" "$OUT/${NAME}_${VER}_amd64.deb" >/dev/null

( cd "$OUT" && sha256sum "$NAME-$VER-linux-x86_64.tar.gz" "${NAME}_${VER}_amd64.deb" )
