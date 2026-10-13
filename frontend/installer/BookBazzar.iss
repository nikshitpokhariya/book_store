; =====================================================================
; Inno Setup Script for BookBazzar Qt 6 Desktop Client
; Application: BookBazzar
; Version: 0.1.0
; Architecture: Windows x64
; Output Target: frontend/dist/BookBazzar-Setup-0.1.0.exe
; =====================================================================

#define MyAppName "BookBazzar"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "BookBazzar Team"
#define MyAppURL "https://github.com/nikshitpokhariya/book_store"
#define MyAppExeName "BookBazzar.exe"
#define SourceDistDir "..\dist\BookBazzar"

[Setup]
; Unique application GUID
AppId={{A89C32C2-1138-4F79-880D-665A519967D8}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Installation target directory (resolves to C:\Program Files\BookBazzar)
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Target output executable in frontend/dist/
OutputDir=..\dist
OutputBaseFilename=BookBazzar-Setup-0.1.0

; Architecture restrictions (strict 64-bit Windows requirement)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Compression settings for high-efficiency distribution
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes

; UI & Modern Installer Styling
WizardStyle=modern
WizardSizePercent=100
DisableWelcomePage=no

; Permissions & Privilege handling
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog commandline

; Windows Uninstall Entry Configuration (Add/Remove Programs)
CreateUninstallRegKey=yes
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}

; Ensure running instances are closed before install/uninstall
CloseApplications=yes
CloseApplicationsFilter=*.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main executable placed first
Source: "{#SourceDistDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Packaged Qt 6 libraries, plugins, translation assets and dependencies
; Excludes any accidental development artifacts, sources, git metadata, or environment configs
Source: "{#SourceDistDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.env*,*.git*,*.pdb,*.obj,*.ilk,*.exp,*.lib"

[Icons]
; Start Menu Program Shortcuts
Name: "{autoprograms}\{#MyAppName}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Comment: "Launch BookBazzar"
Name: "{autoprograms}\{#MyAppName}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"; Comment: "Uninstall BookBazzar"

; Optional Desktop Shortcut
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; WorkingDir: "{app}"; Comment: "Launch BookBazzar"

[Run]
; Run bundled Microsoft Visual C++ 2015-2022 x64 Redistributable if missing on host system
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/passive /norestart"; Check: VCRedistNeedsInstall; Flags: waituntilterminated runascurrentuser; StatusMsg: "Verifying Microsoft Visual C++ 2015-2022 Runtime prerequisites..."

; Post-install launch option
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Clean up any dynamic cache directories created during execution
Type: filesandordirs; Name: "{app}\cache"
Type: filesandordirs; Name: "{app}\logs"

[Code]
// Helper function: determine if Visual C++ 2015-2022 (v14.x) 64-bit runtime is already present
function VCRedistNeedsInstall: Boolean;
var
  Installed: Cardinal;
begin
  Result := True;

  // If the redist installer is not packaged or not found, skip execution
  if not FileExists(ExpandConstant('{app}\vc_redist.x64.exe')) then
  begin
    Result := False;
    Exit;
  end;

  // Query standard HKLM key for 64-bit VC runtime v14.x
  if RegQueryDWordValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed', Installed) then
  begin
    if Installed = 1 then
      Result := False;
  end;
end;
