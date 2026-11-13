; Installer of calibly (Inno Setup 6). Built by tools/make-release.ps1:
;   ISCC.exe /DBuildDir=<release build folder> installer\calibly.iss
; It packs the folder that "cmake --install" wrote to <BuildDir>\deploy.
;
; - Installs for the current user only (no admin rights, updates without a
;   UAC prompt) to %LOCALAPPDATA%\Programs\calibly.
; - Uninstalling removes the program only: the library (Documents\
;   calibly-Bibliothek) and the settings stay.
; - Installing a newer version over an older one keeps everything.

#ifndef BuildDir
  #error Pass the release build folder: ISCC /DBuildDir=...\build\mingw-release calibly.iss
#endif
#include BuildDir + "\installer\version.iss"

[Setup]
; Never change the AppId: it identifies calibly for updates and uninstalling.
AppId={{4D884B1E-29F2-4DEA-8007-6A6B458B750C}
AppName=calibly
AppVersion={#AppVersion}
AppVerName=calibly {#AppVersion}
AppPublisher={#AppPublisher}
AppCopyright={#AppCopyright}
AppPublisherURL={#AppReleasesUrl}
AppSupportURL={#AppReleasesUrl}
AppUpdatesURL={#AppReleasesUrl}
VersionInfoVersion={#AppVersionNumeric}
VersionInfoProductVersion={#AppVersionNumeric}
VersionInfoDescription=calibly Setup
DefaultDirName={autopf}\calibly
DefaultGroupName=calibly
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
SetupIconFile=..\resources\icon\calibly.ico
UninstallDisplayIcon={app}\calibly.exe
UninstallDisplayName=calibly
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
OutputDir={#BuildDir}\installer
OutputBaseFilename=calibly-{#AppVersion}-setup
CloseApplications=yes
RestartApplications=no

[Languages]
; The license of calibly (GNU GPL 3, LICENSE.txt) is shown before installing.
; Only the English text is official, so both languages show it.
Name: "de"; MessagesFile: "compiler:Languages\German.isl"; LicenseFile: "..\LICENSE.txt"
Name: "en"; MessagesFile: "compiler:Default.isl"; LicenseFile: "..\LICENSE.txt"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[InstallDelete]
; Plugins and QML modules of an older version that the new one no longer
; ships; the new version brings them again where needed.
Type: filesandordirs; Name: "{app}\plugins"
Type: filesandordirs; Name: "{app}\qml"
; The German license terms of versions up to 0.2 (calibly is GPL 3 since then).
Type: files; Name: "{app}\LICENSE.de.txt"

[Files]
Source: "{#BuildDir}\deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\calibly"; Filename: "{app}\calibly.exe"
Name: "{autodesktop}\calibly"; Filename: "{app}\calibly.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\calibly.exe"; Description: "{cm:LaunchProgram,calibly}"; Flags: nowait postinstall skipifsilent
