; Inno Setup – Animator Choreographer (Windows). Aufruf / Usage: iscc /DAppVersion=0.1.0 installer\AnimatorChoreographer.iss
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{6B1D3F52-7C0E-4A4B-9E57-0A11C0DE0001}
AppName=Animator Choreographer
AppVersion={#AppVersion}
AppPublisher=ICST, ZHdK
DefaultDirName={autopf}\Animator Choreographer
DefaultGroupName=Animator Choreographer
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=AnimatorChoreographer-{#AppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\Animator Choreographer.exe
LicenseFile=..\LICENSE
SetupIconFile=..\Resources\Icon\AnimatorChoreographer.ico

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Flags: unchecked
Name: "reaperscript"; Description: "REAPER-Skript installieren / Install REAPER script (%APPDATA%\REAPER\Scripts\Animator)"

[Files]
Source: "..\build\Raumchoreographie_artefacts\Release\Animator Choreographer.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\reaper\Animator_export_regions_markers.lua"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\reaper\Animator_export_regions_markers.lua"; DestDir: "{userappdata}\REAPER\Scripts\Animator"; Tasks: reaperscript; Flags: ignoreversion

[Icons]
Name: "{group}\Animator Choreographer"; Filename: "{app}\Animator Choreographer.exe"
Name: "{group}\{cm:UninstallProgram,Animator Choreographer}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Animator Choreographer"; Filename: "{app}\Animator Choreographer.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Animator Choreographer.exe"; Description: "{cm:LaunchProgram,Animator Choreographer}"; Flags: nowait postinstall skipifsilent
