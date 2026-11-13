# Animator Choreographer (JUCE)

*English · [Deutsch](#deutsch)*

Native desktop app that generates scene XML for the Animator of the ICST AmbiEncoder_64 (ICST Ambisonics Plugins 3.2.0.9). Interface and workflow mirror the web app: five steps (Form, Groups, Gestures, Relations, Review & Export), top and side view, timeline with sections, transport (space bar = start/stop).

## Building on macOS

Requirements: Xcode (Command Line Tools), CMake ≥ 3.22.

```sh
# JUCE is fetched automatically (tag 8.0.12)
cmake -B build -G Xcode
cmake --build build --config Release --target Raumchoreographie

# alternatively with a local JUCE checkout
cmake -B build -G Xcode -DJUCE_DIR=/path/to/JUCE
```

The CMake target keeps its internal name `Raumchoreographie`; the app is built as `build/Raumchoreographie_artefacts/Release/Animator Choreographer.app`.

The app is not signed. On macOS 15 and later: try to open it, then System Settings > Privacy & Security > Open Anyway; or
`xattr -dr com.apple.quarantine "Animator Choreographer.app"`.

### GitHub Actions

`.github/workflows/macos.yml` builds a universal app (arm64 + x86_64), runs the tests and stores `AnimatorChoreographer-macOS.zip` as an artifact.

## Windows

Files for Windows 10/11 (x64): `Animator-Choreographer-<version>-Windows-Setup.exe` installs for the current user without administrator rights (`%LOCALAPPDATA%\Programs\Animator Choreographer`) and optionally copies the REAPER script to `%APPDATA%\REAPER\Scripts\Animator`. `Animator-Choreographer-<version>-Windows-portable.zip` contains the program, the script and LICENSE. Both are unsigned: Windows SmartScreen shows a warning on first start (More info › Run anyway). `.github/workflows/windows.yml` builds both, tests silent installation, start and uninstallation, and attaches them to a release.

## Linux

Packages for x86_64 (built on Ubuntu 22.04, glibc 2.35 or newer): `animator-choreographer_<version>_amd64.deb` and `animator-choreographer-<version>-linux-x86_64.tar.gz`.

```sh
sudo apt install ./animator-choreographer_0.1.0_amd64.deb   # installs /usr/bin/animator-choreographer
```

The REAPER script is installed to `/usr/share/animator-choreographer/`; copy it to `~/.config/REAPER/Scripts/Animator/`. The `.tar.gz` contains the program, script, icon, `.desktop` file and `INSTALL.txt`.

Building: `sudo apt install cmake ninja-build libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libfreetype-dev libfontconfig1-dev libasound2-dev libgl1-mesa-dev`, then `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build` and `installer/make-linux-packages.sh build dist`. `.github/workflows/linux.yml` does the same, checks installation and start without a display, and attaches the packages to a release.

## Usage

- Export XML: file dialog, suggested path `~/Documents/<title>.xml`. After saving, a confirmation with the path and "Show in Finder" appears. If the plan has errors, the app jumps to step 5.
- Save / open plan: `.choreo.json` (same format as the web app).
- Open XML: load an existing Animator export (view or continue editing).
- In the Animator: load the scene via XML import and play it.
- Import REAPER regions/markers: run `reaper/Animator_export_regions_markers.lua` in REAPER (Actions > Load ReaScript). It writes `Animator_<project>_regions_markers.csv` (seconds) next to the project. In the app, step Form > "Import regions (CSV)": regions become sections (gaps between regions become "(gap)" sections), or consecutive markers delimit sections if there are no regions. The app reports the start time of the first section; in the Animator use `Insert at Cursor` at that position.

## Plan from description (local AI)

"Describe" in the header bar opens a window where you describe the choreography in your own words. A local language model via [Ollama](https://ollama.com) drafts a plan from it, not the XML directly. The app's generator then creates the scene as usual.

- The model answers in listening terms (direction, distance, height, gesture, curve). A JSON schema in the `format` field restricts the answer to valid values.
- The app reviews the draft (plan review, group count, one cell per section, total duration) and sends findings back to the model for correction up to two times.
- The result (concept, sections, gestures per group, review findings) appears in the window. "Apply" loads it like an opened plan; afterwards everything can be edited in the five steps.
- "Without AI (rules)" builds a draft with the built-in rule planner instead (English and German keywords), without any language model.
- Options: group count and total duration (or automatic), model from the installed models, "Think" enables the model's thinking phase (more thorough, roughly five to ten times slower).
- The description never leaves your computer. Server: `127.0.0.1:11434`. Settings and the last description are stored in `~/Library/Application Support/Animator Choreographer/Animator Choreographer.settings`.

Setup: install and start Ollama, then `ollama pull gemma4:12b` (about 8 GB, suitable for 16 GB RAM; smaller: `gemma4:e4b`). Cloud models are not offered.

Without the GUI:

```sh
./build/ChoreoAI_artefacts/Release/ChoreoAI --list
./build/ChoreoAI_artefacts/Release/ChoreoAI --groups 6 --dur 35 \
    "Build-up of a threatening sound cluster that implodes at the end" cluster.choreo.json cluster.xml
```

More options: `--model NAME`, `--think`, `--rules`, `--rules-list`, `--print-prompt`, `--print-schema`.

## Tests

```sh
./build/ChoreoTests_artefacts/Release/ChoreoTests /tmp/choreo-out tests/data/test-gp.xml
```

Checked: export of all templates, plan JSON round trip, import/export round trip of an Animator file (byte-identical). The export was also compared with the web app (5 templates and 400 random plans, identical in content).

## Snapshot (hidden option, for tests without interaction)

```sh
Raumchoreographie --snapshot out.png [--step N] [--preset key] [--xml file] [--plan file] \
  [--t seconds] [--section i] [--open key] [--size WxH]
```

On Linux without a display: `xvfb-run -a -s "-screen 0 1600x1000x24" ./Raumchoreographie --snapshot out.png`.

## Limitations

- The direction of rotation of orbits is schematic in the preview; playback in the Animator is authoritative.
- The Animator import steps (`Import Scene…`, `Replace`, `Rescale`) were checked with the AmbiEncoder_64 in REAPER.
- Plan files keep their internal German keys (e.g. `ruhen`, `kreis`, `"format":"raumchoreographie"`) for compatibility with files from the web app.

## Author and authorship

Developed by Johannes Schütt, ICST – Institute for Computer Music and Sound Technology, Zurich University of the Arts (ZHdK).

Concept, design of the workflow, testing in REAPER and responsibility for the content: Johannes Schütt. Written with AI assistance: Claude (Anthropic) for code and documentation; Perplexity Computer for repository, CI (GitHub Actions), packaging and release notes. What has not been tested yet is listed under Limitations and in the release notes.

## Licenses

- Code: GPLv3 (`LICENSE`), because headers from the ICST Ambisonics Plugins (GPLv3) are included: `third_party/icst`, tag 3.2.0.9, commit 27fd2f35c41b2ddd0d40f0df937d1410b71e781b.
- Fonts Inter and JetBrains Mono: SIL Open Font License (`Resources/Fonts`).

---

<a id="deutsch"></a>

# Animator Choreographer (JUCE) — Deutsch

*[English](#animator-choreographer-juce) · Deutsch*

Native Desktop-App, die Szenen-XML für den Animator des ICST AmbiEncoder_64 (ICST Ambisonics Plugins 3.2.0.9) erzeugt. Oberfläche und Ablauf entsprechen der Web-App: fünf Schritte (Form, Gruppen, Gesten, Beziehungen, Prüfen & Export), Draufsicht und Seitenansicht, Zeitleiste mit Abschnitten, Transport (Leertaste = Start/Stopp).

## Bauen auf macOS

Voraussetzungen: Xcode (Command Line Tools), CMake ≥ 3.22.

```sh
# JUCE wird automatisch geladen (Tag 8.0.12)
cmake -B build -G Xcode
cmake --build build --config Release --target Raumchoreographie

# alternativ mit lokalem JUCE-Checkout
cmake -B build -G Xcode -DJUCE_DIR=/pfad/zu/JUCE
```

Der CMake-Target heisst intern weiter `Raumchoreographie`; die App liegt danach unter `build/Raumchoreographie_artefacts/Release/Animator Choreographer.app`.

Die App ist nicht signiert. Ab macOS 15: App öffnen, Meldung schliessen, dann Systemeinstellungen > Datenschutz & Sicherheit > Dennoch öffnen; oder
`xattr -dr com.apple.quarantine "Animator Choreographer.app"`.

### GitHub Actions

`.github/workflows/macos.yml` baut eine Universal-App (arm64 + x86_64), führt die Tests aus und legt `AnimatorChoreographer-macOS.zip` als Artefakt ab.

## Windows

Dateien für Windows 10/11 (x64): `Animator-Choreographer-<version>-Windows-Setup.exe` installiert für den aktuellen Benutzer ohne Administratorrechte (`%LOCALAPPDATA%\Programs\Animator Choreographer`) und kopiert auf Wunsch das REAPER-Skript nach `%APPDATA%\REAPER\Scripts\Animator`. `Animator-Choreographer-<version>-Windows-portable.zip` enthält Programm, Skript und LICENSE. Beide sind nicht signiert: Windows SmartScreen warnt beim ersten Start (Weitere Informationen › Trotzdem ausführen). `.github/workflows/windows.yml` baut beides, prüft stille Installation, Start und Deinstallation und hängt die Dateien an ein Release an.

## Linux

Pakete für x86_64 (gebaut auf Ubuntu 22.04, glibc 2.35 oder neuer): `animator-choreographer_<version>_amd64.deb` und `animator-choreographer-<version>-linux-x86_64.tar.gz`.

```sh
sudo apt install ./animator-choreographer_0.1.0_amd64.deb   # installiert /usr/bin/animator-choreographer
```

Das REAPER-Skript liegt danach unter `/usr/share/animator-choreographer/`; nach `~/.config/REAPER/Scripts/Animator/` kopieren. Das `.tar.gz` enthält Programm, Skript, Icon, `.desktop`-Datei und `INSTALL.txt`.

Bauen: `sudo apt install cmake ninja-build libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libfreetype-dev libfontconfig1-dev libasound2-dev libgl1-mesa-dev`, dann `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build` und `installer/make-linux-packages.sh build dist`. `.github/workflows/linux.yml` macht dasselbe, prüft Installation und Start ohne Bildschirm und hängt die Pakete an ein Release an.

## Bedienung

- XML exportieren: Dateidialog, Vorschlag `~/Documents/<titel>.xml`. Nach dem Speichern erscheint eine Bestätigung mit Pfad und „Im Finder zeigen“. Bei Fehlern im Plan springt die App zu Schritt 5.
- Plan sichern / öffnen: `.choreo.json` (gleiches Format wie die Web-App).
- XML öffnen: vorhandenen Animator-Export laden (Ansicht oder Weiterbearbeitung).
- Im Animator: Szene über den XML-Import laden und abspielen.
- REAPER-Regionen/-Marker importieren: `reaper/Animator_export_regions_markers.lua` in REAPER ausführen (Actions > Load ReaScript). Das Skript schreibt `Animator_<Projekt>_regions_markers.csv` (Sekunden) neben das Projekt. In der App Schritt Form > „Import regions (CSV)": Regionen werden zu Abschnitten (Lücken zwischen Regionen werden zu „(gap)“-Abschnitten), ohne Regionen begrenzen aufeinanderfolgende Marker die Abschnitte. Die App nennt die Startzeit des ersten Abschnitts; im Animator dort `Insert at Cursor` verwenden.

## Plan aus Beschreibung (lokale KI)

»Describe« in der Kopfleiste öffnet ein Fenster, in dem die Choreographie in eigenen Worten beschrieben wird. Ein lokales Sprachmodell über [Ollama](https://ollama.com) entwirft daraus einen Plan, nicht direkt das XML. Die Szene erzeugt danach wie gewohnt der Generator der App.

- Das Modell antwortet in Hörbegriffen (Richtung, Abstand, Höhe, Geste, Kurve). Ein JSON-Schema im Feld `format` beschränkt die Antwort auf gültige Werte.
- Die App prüft den Entwurf (Plan-Prüfung, Gruppenzahl, eine Zelle pro Abschnitt, Gesamtdauer) und schickt Befunde bis zu zweimal zur Korrektur an das Modell zurück.
- Das Ergebnis (Konzept, Abschnitte, Gesten je Gruppe, Prüfbefunde) erscheint im Fenster. »Apply« lädt es wie einen geöffneten Plan, danach lässt sich alles in den fünf Schritten weiter bearbeiten.
- »Without AI (rules)« erzeugt stattdessen einen Entwurf mit dem eingebauten Regelplaner (englische und deutsche Stichwörter), ganz ohne Sprachmodell.
- Vorgaben: Gruppenzahl und Gesamtdauer (oder automatisch), Modell aus den installierten Modellen, »Think« schaltet die Denkphase des Modells ein (gründlicher, etwa fünf- bis zehnmal langsamer).
- Die Beschreibung verlässt den Rechner nicht. Server: `127.0.0.1:11434`. Einstellungen und letzte Beschreibung liegen in `~/Library/Application Support/Animator Choreographer/Animator Choreographer.settings`.

Einrichtung: Ollama installieren und starten, dann `ollama pull gemma4:12b` (etwa 8 GB, für 16 GB RAM geeignet; kleiner: `gemma4:e4b`). Cloud-Modelle werden nicht angeboten.

Ohne Oberfläche:

```sh
./build/ChoreoAI_artefacts/Release/ChoreoAI --list
./build/ChoreoAI_artefacts/Release/ChoreoAI --groups 6 --dur 35 \
    "Aufbau eines bedrohlichen Klangclusters, der am Schluss implodiert" cluster.choreo.json cluster.xml
```

Weitere Optionen: `--model NAME`, `--think`, `--rules`, `--rules-list`, `--print-prompt`, `--print-schema`.

## Tests

```sh
./build/ChoreoTests_artefacts/Release/ChoreoTests /tmp/choreo-out tests/data/test-gp.xml
```

Geprüft werden: Export aller Vorlagen, Plan-JSON-Rundlauf, Import/Export-Rundlauf einer Animator-Datei (byte-identisch). Zusätzlich wurde der Export gegen die Web-App verglichen (5 Vorlagen und 400 Zufallspläne, inhaltlich identisch).

## Schnappschuss (versteckte Option, für Tests ohne Bedienung)

```sh
Raumchoreographie --snapshot out.png [--step N] [--preset key] [--xml datei] [--plan datei] \
  [--t sekunden] [--section i] [--open key] [--size BxH]
```

Unter Linux ohne Bildschirm: `xvfb-run -a -s "-screen 0 1600x1000x24" ./Raumchoreographie --snapshot out.png`.

## Grenzen

- Die Drehrichtung von Umläufen ist in der Vorschau schematisch; massgeblich ist die Wiedergabe im Animator.
- Die Animator-Importschritte (`Import Scene…`, `Replace`, `Rescale`) wurden mit dem AmbiEncoder_64 in REAPER geprüft.
- Plan-Dateien behalten ihre internen deutschen Schlüssel (z. B. `ruhen`, `kreis`, `"format":"raumchoreographie"`), damit Dateien der Web-App kompatibel bleiben.
- Die Oberfläche ist englisch; die Dokumentation gibt es auf Deutsch und Englisch.

## Autor und Entstehung

Entwicklung: Johannes Schütt, ICST – Institute for Computer Music and Sound Technology, Zürcher Hochschule der Künste (ZHdK).

Konzept, Gestaltung der Arbeitsschritte, Prüfung in REAPER und Verantwortung für den Inhalt: Johannes Schütt. Mit KI-Assistenz entstanden: Claude (Anthropic) bei Programmierung und Dokumentation; Perplexity Computer bei Repository, Build-Abläufen (GitHub Actions), Paketierung und Release-Text. Was noch nicht geprüft ist, steht unter „Grenzen“ und in den Release-Notizen.

## Lizenzen

- Code: GPLv3 (`LICENSE`), da Header aus den ICST Ambisonics Plugins (GPLv3) eingebunden sind: `third_party/icst`, Tag 3.2.0.9, Commit 27fd2f35c41b2ddd0d40f0df937d1410b71e781b.
- Schriften Inter und JetBrains Mono: SIL Open Font License (`Resources/Fonts`).
